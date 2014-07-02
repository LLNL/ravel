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
    std::cout << "Event/Message Matching: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;

    delete importer;
    delete rawtrace;

    return trace;
}

// Determine events as blocks of matching enter and exit,
// link them into a call tree
void OTFConverter::matchEvents()
{
    // We can handle each set of events separately
    QStack<EventRecord *> * stack = new QStack<EventRecord *>();
    emit(matchingUpdate(1, "Constructing events..."));
    int progressPortion = std::max(round(rawtrace->num_processes / 1.0
                                         / event_match_portion), 1.0);
    int currentPortion = 0;
    int currentIter = 0;
    bool mpi, sendflag, recvflag;
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

        QVector<CommRecord *> * sendlist = rawtrace->messages->at(i);
        QVector<CommRecord *> * recvlist = rawtrace->messages_r->at(i);
        int sindex = 0, rindex = 0;
        CommEvent * prev = NULL;
        for (QVector<EventRecord *>::Iterator evt = (*event_list)->begin();
             evt != (*event_list)->end(); ++evt)
        {
            if ((*evt)->value == 0) // End of a subroutine
            {
                EventRecord * bgn = stack->pop();

                // Keep track of the mpi_events for partitioning
                CollectiveRecord * cr = NULL;
                mpi = false, sendflag = false, recvflag = false;
                if (((*(trace->functions))[e->function])->group
                        == trace->mpi_group)
                {
                    mpi = true;
                    if (rawtrace->collectiveMap->at((*evt)->process)->contains(bgn->time))
                    {
                        cr = (*(rawtrace->collectiveMap->at((*evt)->process)))[bgn->time];
                    }

                    if (bgn->time == sendlist->at(sindex)->send_time)
                    {
                        sflag = true;
                    }
                    else if (bgn->time > sendlist->at(sindex)->send_time)
                    {
                        sindex++;
                        std::cout << "Error, skipping message (by send) at ";
                        std::cout << sendlist->at(sindex)->send_time << " on ";
                        std::cout << (*evt)->process << std::endl;
                    }

                    if (!sflag && (*evt)->time == recvlist->at(rindex)->recv_time)
                    {
                        rflag = true;
                    }
                    else if (!sflag && (*evt)->time > recvlist->at(rindex)->recv_time)
                    {
                        rindex++;
                        std::cout << "Error, skipping message (by recv) at ";
                        std::cout << recvlist->at(rindex)->send_time << " on ";
                        std::cout << (*evt)->process << std::endl;
                    }
                }


                Event * e = NULL;
                if (cr)
                {
                    cr->events->append(CollectiveEvent(bgn->time, (*evt)->time,
                                            bgn->value, bgn->process,
                                            phase, cr));
                    cr->events->last()->comm_prev = prev;
                    prev->comm_next = cr->events->last();
                    prev = cr->events->last();
                    e = cr->events->last();
                }
                else if (sflag)
                {
                    QVector<Message *> * msgs = new QVector<Message *>();
                    CommRecord * crec = sendlist->at(sindex);
                    if (!(crec->message))
                    {
                        crec->message = new Message(crec->send_time, crec->recv_time);
                    }
                    msgs->append(crec->message);
                    crec->message->sender = new P2PEvent(bgn->time, (*evt)->time,
                                                         bgn->value,
                                                         bgn->process, phase,
                                                         msgs);
                    crec->message->sender->comm_prev = prev;
                    prev->comm_next = crec->message->sender;
                    prev = crec->message->sender;
                    e = crec->message->sender;
                    sindex++;
                }
                else if (rflag)
                {
                    QVector<Message *> * msgs = new QVector<Message *>();
                    CommRecord * crec = NULL;
                    while ((*evt)->time == recvlist->at(rindex)->recv_time)
                    {
                        crec = recvlist->at(rindex);
                        if (!(crec->message))
                        {
                            crec->message = new Message(crec->send_time, crec->recv_time);
                        }
                        msgs->append(crec->message);
                        rindex++;
                    }
                    msgs->at(0)->receiver = new P2PEvent(bgn->time, (*evt)->time,
                                                         bgn->value,
                                                         bgn->process, phase,
                                                         msgs);
                    for (int i = 1; i < msgs->size(); i++)
                    {
                        msgs->at(i)->receiver = msgs->at(0)->receiver;
                    }
                    msgs->at(0)->receiver->is_recv = true;

                    msgs->at(0)->receiver->comm_prev = prev;
                    prev->comm_next = msgs->at(0)->receiver;
                    prev = msgs->at(0)->receiver;

                    e = msgs->at(0)->receiver;
                }
                else
                {
                    e = new Event(bgn->time, (*evt)->time, bgn->value,
                                  bgn->process);
                }

                if (mpi)
                {
                    ((*(trace->mpi_events))[e->process])->prepend(e);
                }

                depth--;
                e->depth = depth;
                if (depth == 0)
                    (*(trace->roots))[(*evt)->process]->append(e);

                if (e->exit > endtime)
                    endtime = e->exit;
                if (!stack->isEmpty())
                {
                    e->caller = stack->top();
                    stack->top()->callees->append(e);
                }

                (*(trace->events))[(*evt)->process]->append(e);
            }
            else // Begin a subroutine
            {
                if (options->partitionByFunction
                    && (*evt)->value == phaseFunction)
                {
                    ++phase;
                }
                depth++;
                stack->push(*evt);
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
