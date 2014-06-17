#include <QElapsedTimer>
#include "otfconverter.h"
#include <climits>
#include <iostream>
#include "general_util.h"

const QString OTFConverter::collectives_string
    = QString("MPI_BarrierMPI_BcastMPI_ReduceMPI_GatherMPI_Scatter")
      + QString("MPI_AllgatherMPI_AllreduceMPI_AlltoallMPI_Scan")
      + QString("MPI_Reduce_scatterMPI_Op_createMPI_Op_freeMPIMPI_Alltoallv")
      + QString("MPI_AllgathervMPI_GathervMPI_Scatterv");

OTFConverter::OTFConverter()
    : rawtrace(NULL), trace(NULL), options(NULL), phaseFunction(-1)
{
}

OTFConverter::~OTFConverter()
{
}

Trace * OTFConverter::importOTF(QString filename, OTFImportOptions *_options)
{
    // Keep track of options
    options = _options;

    // Start with the rawtrace similar to what we got from PARAVER
    OTFImporter * importer = new OTFImporter();
    rawtrace = importer->importOTF(filename.toStdString().c_str());
    emit(finishRead());

    // Time the rest of this
    QElapsedTimer traceTimer;
    qint64 traceElapsed;
    traceTimer.start();
    trace = new Trace(rawtrace->num_processes);

    // Start setting up new Trace
    delete trace->functions;
    trace->functions = rawtrace->functions;

    if (options->partitionByFunction && options->partitionFunction.length() > 0)
    {
        // // Want the smallest containing function in case the name
        // is part of some bigger name
        int char_count = INT_MAX;
        // TODO in future: wildcard set
        for (QMap<int, Function *>::Iterator fxn = trace->functions->begin();
             fxn != trace->functions->end(); ++fxn)
        {
            if ((fxn.value())->name.contains(options->partitionFunction)
                    && (fxn.value())->name.length() < char_count)
            {
                phaseFunction = fxn.key();
                char_count = (fxn.value())->name.length();
            }
        }
    }

    delete trace->functionGroups;
    trace->functionGroups = rawtrace->functionGroups;
    trace->collectives = rawtrace->collectives;
    trace->collectiveMap = rawtrace->collectiveMap;

    // Not sure we need the communicators yet but it would be nice for
    // extra user information later
    trace->communicators = rawtrace->communicators;
    trace->collective_definitions = rawtrace->collective_definitions;

    // Find the MPI Group key
    for (QMap<int, QString>::Iterator fxnGroup = trace->functionGroups->begin();
         fxnGroup != trace->functionGroups->end(); ++fxnGroup)
    {
        if (fxnGroup.value().contains("MPI")) {
            trace->mpi_group = fxnGroup.key();
            break;
        }
    }

    // Convert the events into matching enter and exit
    trace->mpi_events = new QVector<QList<Event *> * >(rawtrace->num_processes);
    for (int i = 0; i < rawtrace->num_processes; ++i)
        (*(trace->mpi_events))[i] = new QList<Event *>();
    matchEvents();

    // Sort all the collective records
    for (QMap<unsigned long long, CollectiveRecord *>::Iterator cr
         = trace->collectives->begin();
         cr != trace->collectives->end(); ++cr)
    {
        qSort((*cr)->events->begin(), (*cr)->events->end(), eventProcessLessThan);
    }

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Event Matching: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;

    traceTimer.start();

    // First sort messages - not necessary
    /*for (QVector<QVector<CommRecord *> *>::Iterator msglist
         = rawtrace->messages->begin();
         msglist != rawtrace->messages->end(); ++msglist)
    {
        qSort((*msglist)->begin(), (*msglist)->end(), dereferencedLessThan<CommRecord>);
    }*/

    // Match messages to previously connected events
    matchMessages();

    delete importer;
    delete rawtrace;

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Message Matching: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;

    return trace;
}

// Determine events as blocks of matching enter and exit,
// link them into a call tree
void OTFConverter::matchEvents()
{
    // We can handle each set of events separately
    QStack<Event *> * stack = new QStack<Event *>();
    emit(matchingUpdate(1, "Constructing events..."));
    int progressPortion = std::max(round(rawtrace->num_processes / 1.0
                                         / event_match_portion), 1.0);
    int currentPortion = 0;
    int currentIter = 0;

    for (QVector<QVector<EventRecord *> *>::Iterator event_list
         = rawtrace->events->begin();
         event_list != rawtrace->events->end(); ++event_list)
    {
        int depth = 0;
        int phase = 0;
        unsigned long long endtime = 0;
        if (round(currentIter / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(matchingUpdate(1 + currentPortion, "Constructing events..."));
        }
        ++currentIter;
        for (QVector<EventRecord *>::Iterator evt = (*event_list)->begin();
             evt != (*event_list)->end(); ++evt)
        {
            if ((*evt)->value == 0) // End of a subroutine
            {
                Event * e = stack->pop();
                e->exit = (*evt)->time;
                if (e->exit > endtime)
                    endtime = e->exit;
                if (!stack->isEmpty()) {
                    e->caller = stack->top();
                    stack->top()->callees->append(e);
                }
                depth--;
            }
            else // Begin a subroutine
            {
                Event * e = new Event((*evt)->time, 0, (*evt)->value,
                                      (*evt)->process, -1);

                if (options->partitionByFunction
                    && (*evt)->value == phaseFunction)
                {
                    ++phase;
                }
                e->phase = phase;

                e->depth = depth;
                if (depth == 0)
                    (*(trace->roots))[(*evt)->process]->append(e);
                depth++;

                // Keep track of the mpi_events for partitioning
                if (((*(trace->functions))[e->function])->group
                        == trace->mpi_group)
                {
                    ((*(trace->mpi_events))[e->process])->prepend(e);
                    if (rawtrace->collectiveMap->at((*evt)->process)->contains((*evt)->time))
                    {
                        CollectiveRecord * cr = (*(rawtrace->collectiveMap->at((*evt)->process)))[(*evt)->time];
                        e->collective = cr;
                        cr->events->append(e);
                    }
                }

                stack->push(e);
                (*(trace->events))[(*evt)->process]->append(e);
            }
        }

        // Deal with unclosed trace issues

        while (!stack->isEmpty())
        {
            Event * e = stack->pop();
            endtime = std::max(endtime, e->enter);
            e->exit = endtime;
            if (!stack->isEmpty()) {
                e->caller = stack->top();
                stack->top()->callees->append(e);
            }
            depth--;
        }

        stack->clear();
    }
    delete stack;
}

void OTFConverter::matchMessages()
{
    int messages = 0;
    int unmatched_recvs = 0;
    int unmatched_sends = 0;
    Event * tmp;
    int index;
    int progressPortion = std::max(round(rawtrace->messages->size() * 0.5
                                         / message_match_portion), 1.0);
    int currentPortion = 0;
    int currentIter = 0;

    // First do all the sends, so the receives can wait for them
    for (int i = 0; i < rawtrace->num_processes; i++)
    {
        if (round(currentIter / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(matchingUpdate(1 + event_match_portion + currentPortion,
                                "Event/Message matching..."));
        }
        ++currentIter;
        QVector<CommRecord *> * commlist = rawtrace->messages->at(i);

        // Match the sends
        QList<Event *> * mpi_events = trace->mpi_events->at(i);
        index = mpi_events->size() - 1;
        for (QVector<CommRecord *>::Iterator comm = commlist->begin();
             comm != commlist->end(); ++comm)
        {
            messages++;
            Message * m = new Message((*comm)->send_time, (*comm)->recv_time);

            Event * send_evt = NULL;

            // Find matching send from last index back
            while (!send_evt && index >= 0)
            {
                tmp = mpi_events->at(index);
                if (tmp->enter <= (*comm)->send_time
                    && tmp->exit >= (*comm)->send_time)
                {
                    send_evt = tmp;
                }
                index--;
            }

            if (send_evt)
            {
                m->sender = send_evt;
                (*comm)->message = m;
            }
            else
            {
                std::cout << "Error finding send event for "
                          << (*comm)->sender << "->" << (*comm)->receiver
                          << " (" << (*comm)->send_time << ", "
                          << (*comm)->recv_time << ")"
                          << " -- dropping message." << std::endl;
                unmatched_sends++;
                delete m;
            }
        }
    }

    // Now do all the receives since sends have been finished
    for (int i = 0; i < rawtrace->num_processes; i++)
    {
        if (round(currentIter / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(matchingUpdate(1 + event_match_portion + currentPortion,
                                "Event/Message matching..."));
        }
        ++currentIter;
        QVector<CommRecord *> * commlist = rawtrace->messages_r->at(i);

        // Match the sends
        QList<Event *> * mpi_events = trace->mpi_events->at(i);
        index = mpi_events->size() - 1;
        for (QVector<CommRecord *>::Iterator comm = commlist->begin();
             comm != commlist->end(); ++comm)
        {
            // Find matching recv - move back/forth from index to find match
            Message * m = (*comm)->message;

            // Don't bother if we didn't find the matching send
            if (!m)
                continue;

            Event * recv_evt = NULL;

            // Find matching send from last index back
            while (!recv_evt && index >= 0)
            {
                tmp = mpi_events->at(index);
                if (tmp->enter <= (*comm)->recv_time
                    && tmp->exit >= (*comm)->recv_time)
                {
                    recv_evt = tmp;
                }
                index--;
            }


            if (recv_evt)
            {
                m->sender->messages->append(m);
                recv_evt->messages->append(m);
                m->receiver = recv_evt;
            }
            else
            {
                std::cout << "Error finding recv event for "
                          << (*comm)->sender << "->" << (*comm)->receiver
                          << " (" << (*comm)->send_time << ", "
                          << (*comm)->recv_time << ")"
                          << " -- dropping message." <<  std::endl;
                unmatched_recvs++;
                delete m;
            }

        }
    }

    // Report
    std::cout << "Total messages: " << messages << " with " << unmatched_sends
              << " unmatched sends and " << unmatched_recvs
              << " unmatched_recvs." << std::endl;
}


// Match the messages to the events.
void OTFConverter::matchMessagesOld()
{
    int messages = 0;
    int unmatched_recvs = 0;
    int unmatched_sends = 0;
    int progressPortion = std::max(round(rawtrace->messages->size() / 1.0
                                         / message_match_portion), 1.0);
    int currentPortion = 0;
    int currentIter = 0;
    for (QVector<QVector<CommRecord *> *>::Iterator commlist
         = rawtrace->messages->begin();
         commlist != rawtrace->messages->end(); ++commlist)
    {
        if (round(currentIter / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(matchingUpdate(1 + event_match_portion + currentPortion,
                                "Event/Message matching..."));
        }
        ++currentIter;
        for (QVector<CommRecord *>::Iterator comm = (*commlist)->begin();
             comm != (*commlist)->end(); ++comm)
        {
            messages++;
            Message * m = new Message((*comm)->send_time, (*comm)->recv_time);

            Event * recv_evt = find_comm_event(search_child_ranges((*(trace->roots))[(*comm)->receiver],
                                                                   (*comm)->recv_time),
                                                                   (*comm)->recv_time);

            Event * send_evt = find_comm_event(search_child_ranges((*(trace->roots))[(*comm)->sender],
                                                                   (*comm)->send_time),
                                                                   (*comm)->send_time);
            if (recv_evt && send_evt)
            {
                recv_evt->messages->append(m);
                m->receiver = recv_evt;
                send_evt->messages->append(m);
                m->sender = send_evt;
            } else {
                if (!recv_evt)
                {
                    std::cout << "Error finding recv event for "
                              << (*comm)->sender << "->" << (*comm)->receiver
                              << " (" << (*comm)->send_time << ", "
                              << (*comm)->recv_time << ")"
                              << " -- dropping message." <<  std::endl;
                    unmatched_recvs++;
                } else {
                    std::cout << "Error finding send event for "
                              << (*comm)->sender << "->" << (*comm)->receiver
                              << " (" << (*comm)->send_time << ", "
                              << (*comm)->recv_time << ")"
                              << " -- dropping message." << std::endl;
                    unmatched_sends++;
                }
            }

        }
    }
    // Report
    std::cout << "Total messages: " << messages << " with " << unmatched_sends
              << " unmatched sends and " << unmatched_recvs
              << " unmatched_recvs." << std::endl;
}

// Binary search for event containing time
Event * OTFConverter::search_child_ranges(QVector<Event *> * children,
                                          unsigned long long int time)
{
    int imid, imin = 0;
    int imax = children->size() - 1;
    while (imax >= imin)
    {
        imid = (imin + imax) / 2;
        if (((*(children))[imid])->exit < time)
        {
            imin = imid + 1;
        }
        else if (((*(children))[imid])->enter > time)
        {
            imax = imid - 1;
        }
        else
            return (*children)[imid];
    }

    return NULL;
}


// Find event containing this comm
Event * OTFConverter::find_comm_event(Event * evt, unsigned long long int time)
{
    // Pass back up null if something went wrong
    if (!evt)
        return evt;


    // No children, this must be it
    if (evt->callees->size() == 0)
        return evt;

    // Otherwise, continue search
    return find_comm_event(search_child_ranges(evt->callees, time), time);
}
