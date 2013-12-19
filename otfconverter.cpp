#include <QElapsedTimer>
#include "otfconverter.h"
#include <climits>
#include <iostream>
#include "general_util.h"

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

    // Time the rest of this
    QElapsedTimer traceTimer;
    qint64 traceElapsed;
    traceTimer.start();
    trace = new Trace(rawtrace->num_processes, false);

    // Start setting up new Trace
    delete trace->functions;
    trace->functions = rawtrace->functions;

    if (options->partitionByFunction && options->partitionFunction.length() > 0)
    {
        int char_count = INT_MAX; // Want the smallest containing function in case name is part of some bigger name
        // TODO in future: wildcard set
        for (QMap<int, Function *>::Iterator fxn = trace->functions->begin(); fxn != trace->functions->end(); ++fxn)
            if ((fxn.value())->name.contains(options->partitionFunction)
                    && (fxn.value())->name.length() < char_count)
            {
                phaseFunction = fxn.key();
                char_count = (fxn.value())->name.length();
            }
    }

    delete trace->functionGroups;
    trace->functionGroups = rawtrace->functionGroups;

    // Find the MPI Group key
    for (QMap<int, QString>::Iterator fxnGroup = trace->functionGroups->begin(); fxnGroup != trace->functionGroups->end(); ++fxnGroup)
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

    // Match messages to previously connected events
    matchMessages();

    delete importer;
    delete rawtrace;

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Event & Message Matching: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;

    return trace;
}

// Determine events as blocks of matching enter and exit
void OTFConverter::matchEvents()
{
    // We can handle each set of events separately
    QStack<Event *> * stack = new QStack<Event *>();
    for (QVector<QVector<EventRecord *> *>::Iterator event_list = rawtrace->events->begin(); event_list != rawtrace->events->end(); ++event_list) {
        int depth = 0;
        int phase = 0;
        unsigned long long endtime = 0;

        for (QVector<EventRecord *>::Iterator evt = (*event_list)->begin(); evt != (*event_list)->end(); ++evt)
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
                Event * e = new Event((*evt)->time, 0, (*evt)->value, (*evt)->process, -1);

                if (options->partitionByFunction && (*evt)->value == phaseFunction)
                    ++phase;
                e->phase = phase;

                e->depth = depth;
                if (depth == 0)
                    (*(trace->roots))[(*evt)->process]->append(e);
                depth++;

                // Keep track of the mpi_events for partitioning
                if (((*(trace->functions))[e->function])->group == trace->mpi_group)
                    ((*(trace->mpi_events))[e->process])->prepend(e);

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

        /*std::cout << "Process " << process << " has " << stack->size() << " left on the stack out of ";
        std::cout << ((*event_list)->size()/2) << " total functions." << std::endl;
        for (QStack<Event *>::Iterator itr = stack->begin(); itr != stack->end(); ++itr)
        {
            std::cout << "   Left function " << (((*(trace->functions))[(*itr)->function])->name).toStdString().c_str() << " at " << (*itr)->enter << std::endl;
        }*/
        stack->clear();
    }
    delete stack;
}


// Match the messages to the events.
void OTFConverter::matchMessages()
{
    int messages = 0;
    int unmatched_recvs = 0;
    int unmatched_sends = 0;
    for (QVector<QVector<CommRecord *> *>::Iterator commlist = rawtrace->messages->begin(); commlist != rawtrace->messages->end(); ++commlist)
    {
        for (QVector<CommRecord *>::Iterator comm = (*commlist)->begin(); comm != (*commlist)->end(); ++comm)
        {
            messages++;
            Message * m = new Message((*comm)->send_time, (*comm)->recv_time);

            Event * recv_evt = find_comm_event(search_child_ranges( (*(trace->roots))[(*comm)->receiver],
                                                                    (*comm)->recv_time),
                                               (*comm)->recv_time);
            if (recv_evt) {
                recv_evt->messages->append(m);
                m->receiver = recv_evt;
            } else {
                std::cout << "Error finding recv event for " << (*comm)->sender << "->" << (*comm)->receiver
                         << " (" << (*comm)->send_time << ", " << (*comm)->recv_time << ")" << std::endl;
                unmatched_recvs++;
            }

            Event * send_evt = find_comm_event(search_child_ranges( (*(trace->roots))[(*comm)->sender],
                                                                    (*comm)->send_time),
                                              (*comm)->send_time);
            if (send_evt) {
                send_evt->messages->append(m);
                m->sender = send_evt;
                trace->send_events->append(send_evt); // Keep track of the send events for merging later
            } else {
                std::cout << "Error finding send event for " << (*comm)->sender << "->" << (*comm)->receiver
                          << " (" << (*comm)->send_time << ", " << (*comm)->recv_time << ")" << std::endl;
                unmatched_sends++;
            }

        }
    }
    std::cout << "Total messages: " << messages << " with " << unmatched_sends << " unmatched sends and " << unmatched_recvs << " unmatched_recvs." << std::endl;
}

// Binary search for event containing time
Event * OTFConverter::search_child_ranges(QVector<Event *> * children, unsigned long long int time)
{
    int imid, imin = 0;
    int imax = children->size() - 1;
    while (imax >= imin)
    {
        imid = (imin + imax) / 2;
        //std::cout << "child loop for " << time << " looking at " << ((*(children))[imid])->enter << " to " << ((*(children))[imid])->exit << std::endl;
        if (((*(children))[imid])->exit < time)
        {
            imin = imid + 1;
            //std::cout << "      increase min " << std::endl;
        }
        else if (((*(children))[imid])->enter > time)
        {
            imax = imid - 1;
            //std::cout << "      decrease max " << std::endl;
        }
        else
            return (*children)[imid];
    }

    //std::cout << "Returning null in child ranges" << std::endl;
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
