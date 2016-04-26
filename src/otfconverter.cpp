#include "otfconverter.h"
#include <QElapsedTimer>
#include <QStack>
#include <QSet>
#include <cmath>
#include <climits>
#include <iostream>
#include "ravelutils.h"

#ifdef OTF1LIB
#include "otfimporter.h"
#endif

#include "otf2importer.h"
#include "importoptions.h"
#include "rawtrace.h"
#include "trace.h"
#include "counter.h"
#include "function.h"
#include "collectiverecord.h"
#include "eventrecord.h"
#include "commrecord.h"
#include "counterrecord.h"
#include "rpartition.h"
#include "event.h"
#include "commevent.h"
#include "p2pevent.h"
#include "message.h"
#include "collectiveevent.h"
#include "primaryentitygroup.h"
#include "metrics.h"


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


Trace * OTFConverter::importOTF(QString filename, ImportOptions *_options)
{
    #ifdef OTF1LIB
    // Keep track of options
    options = _options;

    // Start with the rawtrace similar to what we got from PARAVER
    OTFImporter * importer = new OTFImporter();
    rawtrace = importer->importOTF(filename.toStdString().c_str(),
                                   options->enforceMessageSizes);
    emit(finishRead());

    convert();

    delete importer;
    trace->fullpath = filename;
    #endif
    return trace;
}


Trace * OTFConverter::importOTF2(QString filename, ImportOptions *_options)
{
    // Keep track of options
    options = _options;

    // Start with the rawtrace similar to what we got from PARAVER
    OTF2Importer * importer = new OTF2Importer();
    rawtrace = importer->importOTF2(filename.toStdString().c_str(),
                                    options->enforceMessageSizes);
    emit(finishRead());

    convert();

    delete importer;
    trace->fullpath = filename;
    return trace;
}

Trace * OTFConverter::importCharm(RawTrace * rt, ImportOptions *_options)
{
    options = _options;

    rawtrace = rt;
    emit(finishRead());

    convert();

    return trace;
}

void OTFConverter::convert()
{
    // Time the rest of this
    QElapsedTimer traceTimer;
    qint64 traceElapsed;
    traceTimer.start();
    trace = new Trace(rawtrace->num_entities, rawtrace->num_entities);
    trace->units = rawtrace->second_magnitude;

    // Start setting up new Trace
    delete trace->functions;
    trace->functions = rawtrace->functions;
    delete trace->functionGroups;
    trace->primaries = rawtrace->primaries;
    trace->functionGroups = rawtrace->functionGroups;
    trace->collectives = rawtrace->collectives;
    trace->collectiveMap = rawtrace->collectiveMap;

    trace->entitygroups = rawtrace->entitygroups;
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


    if (rawtrace->options->origin == ImportOptions::OF_SAVE_OTF2)
    {
        trace->options = *(rawtrace->options);
        options = rawtrace->options;

        // Setup metrics
        for (QList<QString>::Iterator metric = rawtrace->metric_names->begin();
             metric != rawtrace->metric_names->end(); ++metric)
        {
            trace->metrics->append(*metric);
            trace->metric_units->insert(*metric,
                                        rawtrace->metric_units->value(*metric));
        }

        matchEventsSaved();
    }
    else
    {

        // Set up collective metrics
        for (QMap<unsigned int, Counter *>::Iterator counter = rawtrace->counters->begin();
             counter != rawtrace->counters->end(); ++counter)
        {
            trace->metrics->append((counter.value())->name);
            trace->metric_units->insert((counter.value())->name,
                                        (counter.value())->name + " / time");
        }

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

        // Convert the events into matching enter and exit
        matchEvents();
    }

    // Sort all the collective records
    for (QMap<unsigned long long, CollectiveRecord *>::Iterator cr
         = trace->collectives->begin();
         cr != trace->collectives->end(); ++cr)
    {
        qSort((*cr)->events->begin(), (*cr)->events->end(), Event::eventEntityLessThan);
    }

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Event/Message Matching: ");

    delete rawtrace;
}

void OTFConverter::makeSingletonPartition(CommEvent * evt)
{
    Partition * p = new Partition();
    p->addEvent(evt);
    evt->partition = p;
    p->new_partition = p;
    trace->partitions->append(p);
}

// Determine events as blocks of matching enter and exit,
// link them into a call tree
void OTFConverter::matchEvents()
{
    // We can handle each set of events separately
    QStack<EventRecord *> * stack = new QStack<EventRecord *>();

    // Keep track of how many commsbelow we have at each depth
    QMap<int, int> commsbelow = QMap<int, int>();

    // Keep track of the counters at that time
    QStack<CounterRecord *> * counterstack = new QStack<CounterRecord *>();
    QMap<unsigned int, CounterRecord *> * lastcounters = new QMap<unsigned int, CounterRecord *>();

    // May be used later to do partition by function
    QList<QList<CommEvent *> *> * allcomms = new QList<QList<CommEvent *> *>();

    // Used for heuristic waitall merging
    // We're now making groups of sends that must end in a Waitall/Testall
    // and not be interrupted by a Collective or a Receive.
    // This is backwards from the previous algorithm. We will end up collecting
    // lots of these sends but not adding them to the waitall since they
    // don't end in a waitall/testall. Instead we'll just clear them and
    // keep looking.
    QList<QList<Partition *> *> * waitallgroups = new QList<QList<Partition *> *>();
    QList<Partition *> * sendgroup = new QList<Partition *>();

    // In the case of true waitall merging, we still use the waitallgroups and the
    // sendgroups but with a different algorithm.
    // max_complete is 0 if we're not in the midst of request-waitall events
    // and is the max found complete time within if we are
    unsigned long long int max_complete = 0;

    emit(matchingUpdate(1, "Constructing events..."));
    int progressPortion = std::max(round(rawtrace->num_entities / 1.0
                                         / event_match_portion), 1.0);
    int currentPortion = 0;
    int currentIter = 0;
    bool sflag, rflag, isendflag;

    // Find needed indices for merge options
    int isend_index = -1, waitall_index = -1, testall_index = -1;
    if ((options->isendCoalescing || options->waitallMerge) && !options->partitionByFunction)
    {
        for (QMap<int, Function * >::Iterator function = trace->functions->begin();
             function != trace->functions->end(); ++function)
        {
            if (function.value()->group == trace->mpi_group)
            {
                if (function.value()->name == "MPI_Isend")
                    isend_index = function.key();
                if (function.value()->name == "MPI_Waitall")
                    waitall_index = function.key();
                if (function.value()->name == "MPI_Testall")
                    testall_index = function.key();
            }
        }
    }

    int spartcounter = 0, rpartcounter = 0, cpartcounter = 0;
    for (int i = 0; i < rawtrace->events->size(); i++)
    {
        QVector<EventRecord *> * event_list = rawtrace->events->at(i);
        int depth = 0;
        int phase = 0;
        unsigned long long endtime = 0;
        max_complete = 0;
        if (round(currentIter / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(matchingUpdate(1 + currentPortion, "Constructing events..."));
        }
        ++currentIter;

        QVector<CounterRecord *> * counters = rawtrace->counter_records->at(i);
        lastcounters->clear();
        int counter_index = 0;

        QVector<RawTrace::CollectiveBit *> * collective_bits = rawtrace->collectiveBits->at(i);
        int collective_index = 0;

        QList<CommEvent *> * commevents = new QList<CommEvent *>();
        allcomms->append(commevents);

        QVector<CommRecord *> * sendlist = rawtrace->messages->at(i);
        QVector<CommRecord *> * recvlist = rawtrace->messages_r->at(i);
        QList<P2PEvent *> * isends = new QList<P2PEvent *>();
        int sindex = 0, rindex = 0;
        CommEvent * prev = NULL;
        for (QVector<EventRecord *>::Iterator evt = event_list->begin();
             evt != event_list->end(); ++evt)
        {
            if (!((*evt)->enter)) // End of a subroutine
            {
                EventRecord * bgn = stack->pop();

                // This is definitely not an isend, so finish coalescing any pending isends
                if (options->isendCoalescing && bgn->value != isend_index && isends->size() > 0)
                {
                    P2PEvent * isend = new P2PEvent(isends);
                    isend->comm_prev = isends->first()->comm_prev;
                    if (isend->comm_prev)
                        isend->comm_prev->comm_next = isend;
                    makeSingletonPartition(isend);
                    prev = isend;
                    isends = new QList<P2PEvent *>();

                    if (!options->partitionByFunction
                        && (max_complete > 0 || options->waitallMerge))
                    {
                        sendgroup->append(trace->partitions->last());
                    }

                    if (stack->isEmpty())
                    {
                        (*(trace->roots))[isend->entity]->append(isend);
                    }

                }

                // Partition/handle comm events
                CollectiveRecord * cr = NULL;
                sflag = false, rflag = false, isendflag = false;
                if (((*(trace->functions))[bgn->value])->group
                        == trace->mpi_group)
                {
                    // Check for possible collective
                    if (collective_index < collective_bits->size()
                        && bgn->time <= collective_bits->at(collective_index)->time
                            && (*evt)->time >= collective_bits->at(collective_index)->time)
                    {
                        cr = collective_bits->at(collective_index)->cr;
                        collective_index++;
                    }

                    // Check/advance sends, including if isend
                    if (sindex < sendlist->size())
                    {
                        if (bgn->time <= sendlist->at(sindex)->send_time
                                && (*evt)->time >= sendlist->at(sindex)->send_time)
                        {
                            sflag = true;
                            if (bgn->value == isend_index && options->isendCoalescing)
                                isendflag = true;
                        }
                        else if (bgn->time > sendlist->at(sindex)->send_time)
                        {
                            std::cout << "Error, skipping message (by send) at ";
                            std::cout << sendlist->at(sindex)->send_time << " on ";
                            std::cout << (*evt)->entity << std::endl;
                            sindex++;
                        }
                    }

                    // Check/advance receives
                    if (rindex < recvlist->size())
                    {
                        if (!sflag && (*evt)->time >= recvlist->at(rindex)->recv_time
                                && bgn->time <= recvlist->at(rindex)->recv_time)
                        {
                            rflag = true;
                        }
                        else if (!sflag && (*evt)->time > recvlist->at(rindex)->recv_time)
                        {
                            std::cout << "Error, skipping message (by recv) at ";
                            std::cout << recvlist->at(rindex)->send_time << " on ";
                            std::cout << (*evt)->entity << std::endl;
                            rindex++;
                        }
                    }
                }

                Event * e = NULL;
                if (cr)
                {
                    cr->events->append(new CollectiveEvent(bgn->time, (*evt)->time,
                                            bgn->value, bgn->entity, bgn->entity,
                                            phase, cr));
                    cr->events->last()->comm_prev = prev;
                    if (prev)
                        prev->comm_next = cr->events->last();
                    prev = cr->events->last();

                    counter_index = advanceCounters(cr->events->last(),
                                                    counterstack,
                                                    counters, counter_index,
                                                    lastcounters);

                    e = cr->events->last();
                    if (options->partitionByFunction)
                        commevents->append(cr->events->last());
                    else
                        makeSingletonPartition(cr->events->last());

                    cpartcounter++;

                    // Collective gets counted as both send and receive so 2
                    commsbelow.insert(depth, commsbelow.value(depth) + 2);

                    if (!options->partitionByFunction)
                    {
                        // We are still collecting
                        if (max_complete > 0)
                            sendgroup->append(trace->partitions->last());

                        // Any sends beforehand not end in a waitall.
                        else if (options->waitallMerge)
                            sendgroup->clear();

                    }
                }
                else if (sflag)
                {
                    QVector<Message *> * msgs = new QVector<Message *>();
                    CommRecord * crec = sendlist->at(sindex);
                    if (!(crec->message))
                    {
                        crec->message = new Message(crec->send_time,
                                                    crec->recv_time,
                                                    crec->group);
                        crec->message->tag = crec->tag;
                        crec->message->size = crec->size;
                    }
                    if (crec->send_complete > max_complete)
                        max_complete = crec->send_complete;
                    msgs->append(crec->message);
                    crec->message->sender = new P2PEvent(bgn->time, (*evt)->time,
                                                         bgn->value,
                                                         bgn->entity, bgn->entity, phase,
                                                         msgs);

                    if (isendflag)
                        isends->append(crec->message->sender);


                    crec->message->sender->comm_prev = prev;
                    if (prev)
                        prev->comm_next = crec->message->sender;
                    prev = crec->message->sender;

                    counter_index = advanceCounters(crec->message->sender,
                                                    counterstack,
                                                    counters, counter_index,
                                                    lastcounters);

                    e = crec->message->sender;
                    if (options->partitionByFunction)
                        commevents->append(crec->message->sender);
                    else if (!(options->isendCoalescing && isendflag))
                        makeSingletonPartition(crec->message->sender);
                    sindex++;

                    spartcounter++;

                    commsbelow.insert(depth, commsbelow.value(depth) + 1);

                    // Collect the send for possible waitall merge
                    if ((max_complete > 0 || options->waitallMerge)
                        && !(options->isendCoalescing && isendflag)
                        && !options->partitionByFunction)
                    {
                        sendgroup->append(trace->partitions->last());
                    }
                }
                else if (rflag)
                {
                    QVector<Message *> * msgs = new QVector<Message *>();
                    CommRecord * crec = NULL;
                    while (rindex < recvlist->size() && (*evt)->time >= recvlist->at(rindex)->recv_time
                           && bgn->time <= recvlist->at(rindex)->recv_time)
                    {
                        crec = recvlist->at(rindex);
                        if (!(crec->message))
                        {
                            crec->message = new Message(crec->send_time,
                                                        crec->recv_time,
                                                        crec->group);
                            crec->message->tag = crec->tag;
                            crec->message->size = crec->size;
                        }
                        msgs->append(crec->message);
                        rindex++;
                    }
                    msgs->at(0)->receiver = new P2PEvent(bgn->time, (*evt)->time,
                                                         bgn->value,
                                                         bgn->entity, bgn->entity, phase,
                                                         msgs);
                    for (int i = 1; i < msgs->size(); i++)
                    {
                        msgs->at(i)->receiver = msgs->at(0)->receiver;
                    }
                    msgs->at(0)->receiver->is_recv = true;

                    msgs->at(0)->receiver->comm_prev = prev;
                    if (prev)
                        prev->comm_next = msgs->at(0)->receiver;
                    prev = msgs->at(0)->receiver;

                    if (options->partitionByFunction)
                        commevents->append(msgs->at(0)->receiver);
                    else
                        makeSingletonPartition(msgs->at(0)->receiver);

                    rpartcounter++;

                    commsbelow.insert(depth, commsbelow.value(depth) + 1); // + msgs->size() ?

                    counter_index = advanceCounters(msgs->at(0)->receiver,
                                                    counterstack,
                                                    counters, counter_index,
                                                    lastcounters);

                    e = msgs->at(0)->receiver;

                    if (!options->partitionByFunction)
                    {
                        if (max_complete > 0)
                        {
                            // This contains the max complete time, end the group
                            if (e->enter <= max_complete && e->exit >= max_complete
                                    && sendgroup->size() > 0)
                            {
                                waitallgroups->append(sendgroup);
                                sendgroup = new QList<Partition *>();
                                max_complete = 0;
                            }
                            else
                            {
                                sendgroup->append(trace->partitions->last());
                            }
                        }

                        else if (options->waitallMerge)
                        {
                            // Is this a wait/test all, end the group
                            if ((bgn->value == waitall_index || bgn->value == testall_index)
                                    && sendgroup->size() > 0)
                            {
                                waitallgroups->append(sendgroup);
                                sendgroup = new QList<Partition *>();
                            }
                            else // Break the send group, not a waitall
                            {
                                sendgroup->clear();
                            }
                        }
                    }
                }
                else // Non-com event
                {
                    e = new Event(bgn->time, (*evt)->time, bgn->value,
                                  bgn->entity, bgn->entity);

                    // Stop by Waitall/Testall
                    if (!options->partitionByFunction)
                    {
                        // true waitall
                        if (max_complete > 0)
                        {
                            // This contains the max complete time, end the group
                            if (e->enter <= max_complete && e->exit >= max_complete
                                    && sendgroup->size() > 0)
                            {
                                waitallgroups->append(sendgroup);
                                sendgroup = new QList<Partition *>();
                                max_complete = 0;
                            }
                        }

                        // waitall heuristic
                        else if (options->waitallMerge && sendgroup->size() > 0
                            && (bgn->value == waitall_index || bgn->value == testall_index))
                        {
                            waitallgroups->append(sendgroup);
                            sendgroup = new QList<Partition *>();
                        }
                    }

                    // Squelch counter values that we're not keeping track of here (for now)
                    while (!counterstack->isEmpty() && counterstack->top()->time == bgn->time)
                    {
                        counterstack->pop();
                    }
                    while (counters->size() > counter_index
                           && counters->at(counter_index)->time == (*evt)->time)
                    {
                        counter_index++;
                    }

                    // Keep track of the largest number of comms in each function name
                    // Then add the value for the current depth and clear the children
                    // for the sibling function at this depth.
                    if (trace->functions->value(bgn->value)->comms < commsbelow.value(depth+1))
                        trace->functions->value(bgn->value)->comms = commsbelow.value(depth+1);
                    commsbelow.insert(depth, commsbelow.value(depth) + commsbelow.value(depth+1)); // Add for parent
                    commsbelow.insert(depth+1, 0); // Clear children
                }

                depth--;
                e->depth = depth;
                if (depth == 0 && !isendflag)
                    (*(trace->roots))[(*evt)->entity]->append(e);

                if (e->exit > endtime)
                    endtime = e->exit;
                if (!stack->isEmpty())
                {
                    stack->top()->children.append(e);
                }
                for (QList<Event *>::Iterator child = bgn->children.begin();
                     child != bgn->children.end(); ++child)
                {
                    // If the child already has a caller, it was coalesced.
                    // In that case, we want to make that caller the child
                    // rather than this reality direct one... but only for
                    // the first one
                    if ((*child)->caller)
                    {
                        if (e->callees->isEmpty()
                            || e->callees->last() != (*child)->caller)
                            e->callees->append((*child)->caller);
                    }
                    else
                    {
                        e->callees->append(*child);
                        (*child)->caller = e;
                    }
                }

                (*(trace->events))[(*evt)->entity]->append(e);
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
                while (counters->size() > counter_index
                       && counters->at(counter_index)->time == (*evt)->time)
                {
                    counterstack->push(counters->at(counter_index));
                    counter_index++;

                    // Set the first one to the beginning of the trace
                    if (lastcounters->value(counters->at(counter_index)->counter) == NULL)
                    {
                        lastcounters->insert(counters->at(counter_index)->counter,
                                             counters->at(counter_index));
                    }
                }

            }
        }

        // Finish off last isend list
        // This really shouldn't be needed because we expect
        // something handling their request to come after them
        if (options->isendCoalescing && isends->size() > 0)
        {
            P2PEvent * isend = new P2PEvent(isends);
            isend->comm_prev = isends->first()->comm_prev;
            if (isend->comm_prev)
                isend->comm_prev->comm_next = isend;
            prev = isend;

            if (stack->isEmpty())
                (*(trace->roots))[isend->entity]->append(isend);
        }
        else // Only do this if it is empty
        {
            delete isends;
        }

        // Deal with unclosed trace issues
        // We assume these events are not communication
        while (!stack->isEmpty())
        {
            EventRecord * bgn = stack->pop();
            endtime = std::max(endtime, bgn->time);
            Event * e = new Event(bgn->time, endtime, bgn->value,
                          bgn->entity, bgn->entity);
            if (!stack->isEmpty())
            {
                stack->top()->children.append(e);
            }
            for (QList<Event *>::Iterator child = bgn->children.begin();
                 child != bgn->children.end(); ++child)
            {
                e->callees->append(*child);
                (*child)->caller = e;
            }
            (*(trace->events))[bgn->entity]->append(e);
            depth--;
        }

        // Prepare for next entity
        stack->clear();
        sendgroup->clear();
    }
    delete stack;
    delete sendgroup;

    if (!options->partitionByFunction
            && (options->waitallMerge
                || options->origin == ImportOptions::OF_OTF2))
    {
        mergeContiguous(waitallgroups);
    }

    if (!options->partitionByFunction
            && options->callerMerge)
    {
        mergeByMultiCaller();
    }


    // Fix phases and create partitions if partitioning by function
    // We have to pay attention here as this partitioning might break our
    // ordering constraints (send & receive in same partition, collective in
    // single partition) -- in which case we need to fix it. Here we fix
    // everything to its last possible phase based on these constraints.
    if (options->partitionByFunction)
    {
        std::cout << "Partitioning by phase..." << std::endl;
        QMap<int, Partition *> * partition_dict = new QMap<int, Partition *>();
        for (QList<QList<CommEvent *> *>::Iterator event_list = allcomms->begin();
            event_list != allcomms->end(); ++event_list)
        {
            for (int i = 0; i < (*event_list)->size(); i++)
            {
                CommEvent * evt = (*event_list)->at(i);
                if ((evt)->comm_prev
                    && (evt)->comm_prev->phase > (evt)->phase)
                {
                    (evt)->phase = (evt)->comm_prev->phase;
                }

                // Fix phases based on whether they have a message or not
                evt->fixPhases();

                if (!partition_dict->contains((evt)->phase))
                    (*partition_dict)[(evt)->phase] = new Partition();
                ((*partition_dict)[(evt)->phase])->addEvent(evt);
                (evt)->partition = (*partition_dict)[(evt)->phase];
            }
        }

        for (QMap<int, Partition *>::Iterator partition
             = partition_dict->begin();
             partition != partition_dict->end(); ++partition)
        {
            (*partition)->sortEvents();
            trace->partitions->append(*partition);
        }

        delete partition_dict;
    }

    // Clean up allcoms
    for (QList<QList<CommEvent *> *>::Iterator ac = allcomms->begin();
         ac != allcomms->end(); ++ac)
    {
        delete *ac;
    }
    delete allcomms;
}

// We only do this with comm events right now, so we know we won't have nesting
int OTFConverter::advanceCounters(CommEvent * evt, QStack<CounterRecord *> * counterstack,
                                   QVector<CounterRecord *> * counters, int index,
                                   QMap<unsigned int, CounterRecord *> * lastcounters)
{
    CounterRecord * begin, * last, * end;
    int tmpIndex;

    // We know we can't have too many counters recorded, so we can search in them
    while (!counterstack->isEmpty() && counterstack->top()->time == evt->enter)
    {
        begin = counterstack->pop();
        last = lastcounters->value(begin->counter);
        end = NULL;

        tmpIndex = index;
        while(!end && tmpIndex < counters->size())
        {
            if (counters->at(tmpIndex)->time == evt->exit
                    && counters->at(tmpIndex)->counter == begin->counter)
            {
                end = counters->at(tmpIndex);
            }
            tmpIndex++;
        }

        if (end)
        {
            // Add metric
            evt->metrics->addMetric(rawtrace->counters->value(begin->counter)->name,
                                    end->value - begin->value,
                                    begin->value - last->value);

            // Update last counters
            lastcounters->insert(end->counter, end);

        }
        else
        {
            // Error in some way since matching counter not found
            std::cout << "Matching counter not found!" << std::endl;
        }

    }

    // Advance counters index
    while(index < counters->size()
          && counters->at(index)->time <= evt->exit)
    {
        index++;
    }
    return index;
}

// We have our initial partitions set up and we counted the undercomms along the way
// Now we use this to figure out which ones should be merged if we want to merge
// by the ones that have multiple comms under them.
void OTFConverter::mergeByMultiCaller()
{
    QList<QList<Partition *> *> * groups = new QList<QList<Partition *> *>();
    QList<Partition *> * current_group = new QList<Partition *>();
    Event * multicaller = NULL;

    // We assume partitions are in order by entity
    for (QList<Partition *>::Iterator part = trace->partitions->begin();
         part != trace->partitions->end(); ++part)
    {
        // There should be only one event_list at this point, have not
        // merged across entities
        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = (*part)->events->begin();
                 event_list != (*part)->events->end(); ++event_list)
        {
            // If our first value equals the previous' last value -- add
            // Or if this group is empty, add (initial condition)
            if (current_group->isEmpty()
                || (multicaller
                    && event_list.value()->first()->least_multiple_function_caller(trace->functions) == multicaller))
            {
                current_group->append(*part);
            }
            // Otherwise we don't match, close previous group and
            // start a new group
            else
            {
                groups->append(current_group);
                current_group = new QList<Partition *>();
                current_group->append(*part);
            }

            // We only stitch at the end so look at the last value
            multicaller = event_list.value()->last()->least_multiple_function_caller(trace->functions);
        }
    }

    // Close off
    if (!current_group->isEmpty())
        groups->append(current_group);

    mergeContiguous(groups);
}

// This is a lighter weight merge since we know at this stage everything stays
// within its own entity and the partitions that must be merged are contiguous
// Therefore we only need to merge the groups and remove the previous ones
// from the partition list -- so we create the new ones and point the old
// ones toward it so we can make a single pass to remove those that point to
// a new partition.
void OTFConverter::mergeContiguous(QList<QList<Partition * > *> * groups)
{
    // Create the new partitions and store them in new parts
    QList<Partition *> * newparts = new QList<Partition *>();

    for (QList<QList<Partition *> *>::Iterator group = groups->begin();
         group != groups->end(); ++group)
    {
        Partition * p = new Partition();
        for (QList<Partition *>::Iterator part = (*group)->begin();
             part != (*group)->end(); ++part)
        {
            (*part)->new_partition = p;
            for (QMap<unsigned long, QList<CommEvent *> *>::Iterator proc
                 = (*part)->events->begin();
                 proc != (*part)->events->end(); ++proc)
            {
                for (QList<CommEvent *>::Iterator evt
                     = (proc.value())->begin();
                     evt != (proc.value())->end(); ++evt)
                {
                    (*evt)->partition = p;
                    p->addEvent(*evt);
                }
            }
        }
        p->new_partition = p;
        //newparts->append(p);
    }

    // Add in the partitions in order -- those that didn't get changed
    // just added in normally. Add the first instance of the others so
    // that order can be maintained and delete the ones that represent
    // changes.
    QSet<Partition *> sortSet = QSet<Partition *>();
    QList<Partition *> toDelete = QList<Partition *>();
    for (QList<Partition *>::Iterator part = trace->partitions->begin();
         part != trace->partitions->end(); ++part)
    {
        if ((*part)->new_partition == *part)
        {
            newparts->append(*part);
        }
        else
        {
            if (!sortSet.contains((*part)->new_partition))
            {
                newparts->append((*part)->new_partition);
                sortSet.insert((*part)->new_partition);
            }
            toDelete.append(*part);
        }
    }

    // Delete old parts
    for (QList<Partition *>::Iterator oldpart = toDelete.begin();
         oldpart != toDelete.end(); ++oldpart)
    {
        delete *oldpart;
        *oldpart = NULL;
    }

    // Add the new partitions to trace->partitions
    delete trace->partitions;
    trace->partitions = newparts;
}

// Determine events as blocks of matching enter and exit,
// link them into a call tree - from saved Ravel OTF2 to re-read faster
void OTFConverter::matchEventsSaved()
{
    // We can handle each set of events separately
    QStack<EventRecord *> * stack = new QStack<EventRecord *>();

    // Find needed indices for merge options
    int isend_index = -1;
    if ((options->isendCoalescing || options->waitallMerge) && !options->partitionByFunction)
    {
        for (QMap<int, Function * >::Iterator function = trace->functions->begin();
             function != trace->functions->end(); ++function)
        {
            if (function.value()->group == trace->mpi_group)
            {
                if (function.value()->name == "MPI_Isend")
                    isend_index = function.key();
            }
        }
    }

    QList<Partition *> * sendgroup = new QList<Partition *>();

    emit(matchingUpdate(1, "Constructing events..."));
    int progressPortion = std::max(round(rawtrace->num_entities / 1.0
                                         / event_match_portion), 1.0);
    int currentPortion = 0;
    int currentIter = 0;
    bool sflag, rflag, isendflag;

    // If isend index, we put this at the depth of the isend. Then we will
    // collect subevents until the depth matches this value.
    int coalesceflag;
    bool coalesced_event;

    for (int i = 0; i < rawtrace->events->size(); i++)
    {
        QVector<EventRecord *> * event_list = rawtrace->events->at(i);
        int depth = 0;
        int phase = 0;
        unsigned long long endtime = 0;
        if (round(currentIter / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(matchingUpdate(1 + currentPortion, "Constructing events..."));
        }
        ++currentIter;

        QVector<RawTrace::CollectiveBit *> * collective_bits = rawtrace->collectiveBits->at(i);
        int collective_index = 0;

        QVector<CommRecord *> * sendlist = rawtrace->messages->at(i);
        QVector<CommRecord *> * recvlist = rawtrace->messages_r->at(i);
        QList<P2PEvent *> * isends = new QList<P2PEvent *>();
        int sindex = 0, rindex = 0;
        CommEvent * prev = NULL;
        coalesceflag = -1;
        for (QVector<EventRecord *>::Iterator evt = event_list->begin();
             evt != event_list->end(); ++evt)
        {
            coalesced_event = false;
            if (!((*evt)->enter)) // End of a subroutine
            {
                EventRecord * bgn = stack->pop();

                // Partition/handle comm events
                CollectiveRecord * cr = NULL;
                sflag = false, rflag = false, isendflag = false;
                if (((*(trace->functions))[bgn->value])->group
                        == trace->mpi_group)
                {
                    // Check for possible collective
                    if (collective_index < collective_bits->size()
                        && bgn->time <= collective_bits->at(collective_index)->time
                            && (*evt)->time >= collective_bits->at(collective_index)->time)
                    {
                        cr = collective_bits->at(collective_index)->cr;
                        collective_index++;
                    }

                    // Check/advance sends, including if isend but if we did coalesce isends,
                    // we need to check for that event first to switch the coalescing on.
                    if (sindex < sendlist->size() && depth > coalesceflag)
                    {
                        if (bgn->time <= sendlist->at(sindex)->send_time
                                && (*evt)->time >= sendlist->at(sindex)->send_time)
                        {
                            sflag = true;
                            if (bgn->value == isend_index && options->isendCoalescing)
                                isendflag = true;
                        }
                        else if (bgn->time > sendlist->at(sindex)->send_time)
                        {
                            std::cout << "Error, skipping message (by send) at ";
                            std::cout << sendlist->at(sindex)->send_time << " on ";
                            std::cout << (*evt)->entity << std::endl;
                            sindex++;
                        }
                    }

                    // Check/advance receives
                    if (rindex < recvlist->size())
                    {
                        if (!sflag && (*evt)->time >= recvlist->at(rindex)->recv_time
                                && bgn->time <= recvlist->at(rindex)->recv_time)
                        {
                            rflag = true;
                        }
                        else if (!sflag && (*evt)->time > recvlist->at(rindex)->recv_time)
                        {
                            std::cout << "Error, skipping message (by recv) at ";
                            std::cout << recvlist->at(rindex)->send_time << " on ";
                            std::cout << (*evt)->entity << std::endl;
                            rindex++;
                        }
                    }
                }

                Event * e = NULL;
                if (cr)
                {
                    cr->events->append(new CollectiveEvent(bgn->time, (*evt)->time,
                                            bgn->value, bgn->entity, bgn->entity,
                                            phase, cr));
                    cr->events->last()->comm_prev = prev;
                    if (prev)
                        prev->comm_next = cr->events->last();
                    prev = cr->events->last();

                    handleSavedAttributes(cr->events->last(), *evt);
                    addToSavedPartition(cr->events->last(), cr->events->last()->phase);
                    e = cr->events->last();
                }
                else if (coalesceflag == depth)
                {
                    coalesceflag = -1; // Return to not coalescing
                    P2PEvent * isend = new P2PEvent(isends);
                    isend->comm_prev = isends->first()->comm_prev;
                    if (isend->comm_prev)
                        isend->comm_prev->comm_next = isend;
                    addToSavedPartition(isend, isend->phase);
                    handleSavedAttributes(isend, *evt);
                    prev = isend;
                    e = isend;
                    isends = new QList<P2PEvent *>();
                    coalesced_event = true;
                }
                else if (sflag)
                {
                    QVector<Message *> * msgs = new QVector<Message *>();
                    CommRecord * crec = sendlist->at(sindex);
                    if (!(crec->message))
                    {
                        crec->message = new Message(crec->send_time,
                                                    crec->recv_time,
                                                    crec->group);
                        crec->message->tag = crec->tag;
                        crec->message->size = crec->size;
                    }
                    msgs->append(crec->message);
                    crec->message->sender = new P2PEvent(bgn->time, (*evt)->time,
                                                         bgn->value,
                                                         bgn->entity, bgn->entity, phase,
                                                         msgs);

                    crec->message->sender->comm_prev = prev;
                    if (prev)
                        prev->comm_next = crec->message->sender;
                    prev = crec->message->sender;
                    sindex++;

                    if (isendflag)
                    {
                        isends->append(crec->message->sender);
                    }
                    else
                    {
                        handleSavedAttributes(crec->message->sender, *evt);
                        addToSavedPartition(crec->message->sender,
                                            crec->message->sender->phase);
                    }
                    e = crec->message->sender;

                }
                else if (rflag)
                {
                    QVector<Message *> * msgs = new QVector<Message *>();
                    CommRecord * crec = NULL;
                    while (rindex < recvlist->size() && (*evt)->time >= recvlist->at(rindex)->recv_time
                           && bgn->time <= recvlist->at(rindex)->recv_time)
                    {
                        crec = recvlist->at(rindex);
                        if (!(crec->message))
                        {
                            crec->message = new Message(crec->send_time,
                                                        crec->recv_time,
                                                        crec->group);
                            crec->message->tag = crec->tag;
                            crec->message->size = crec->size;
                        }
                        msgs->append(crec->message);
                        rindex++;
                    }
                    msgs->at(0)->receiver = new P2PEvent(bgn->time, (*evt)->time,
                                                         bgn->value,
                                                         bgn->entity, bgn->entity, phase,
                                                         msgs);
                    for (int i = 1; i < msgs->size(); i++)
                    {
                        msgs->at(i)->receiver = msgs->at(0)->receiver;
                    }
                    msgs->at(0)->receiver->is_recv = true;

                    msgs->at(0)->receiver->comm_prev = prev;
                    if (prev)
                        prev->comm_next = msgs->at(0)->receiver;
                    prev = msgs->at(0)->receiver;

                    handleSavedAttributes(msgs->at(0)->receiver, *evt);
                    addToSavedPartition(msgs->at(0)->receiver,
                                        msgs->at(0)->receiver->phase);

                    e = msgs->at(0)->receiver;
                }
                else // Non-com event
                {
                    e = new Event(bgn->time, (*evt)->time, bgn->value,
                                  bgn->entity, bgn->entity);
                }

                depth--;
                e->depth = depth;
                if (depth == 0)
                    (*(trace->roots))[(*evt)->entity]->append(e);

                if (!coalesced_event)
                {
                    if (e->exit > endtime)
                        endtime = e->exit;
                    if (!stack->isEmpty())
                    {
                        stack->top()->children.append(e);
                    }
                    for (QList<Event *>::Iterator child = bgn->children.begin();
                         child != bgn->children.end(); ++child)
                    {
                        e->callees->append(*child);
                        (*child)->caller = e;
                    }

                    (*(trace->events))[(*evt)->entity]->append(e);
                }

            }
            else // Begin a subroutine
            {
                depth++;

                if (options->isendCoalescing && (*evt)->value == isend_index && coalesceflag <= 0)
                {
                    coalesceflag = depth;
                }

                stack->push(*evt);
            }
        }

        // Deal with unclosed trace issues
        // We assume these events are not communication
        while (!stack->isEmpty())
        {
            EventRecord * bgn = stack->pop();
            endtime = std::max(endtime, bgn->time);
            Event * e = new Event(bgn->time, endtime, bgn->value,
                          bgn->entity, bgn->entity);
            if (!stack->isEmpty())
            {
                stack->top()->children.append(e);
            }
            for (QList<Event *>::Iterator child = bgn->children.begin();
                 child != bgn->children.end(); ++child)
            {
                e->callees->append(*child);
                (*child)->caller = e;
            }
            (*(trace->events))[bgn->entity]->append(e);
            depth--;
        }

        // Prepare for next entity
        stack->clear();
        sendgroup->clear();
        delete isends;
    }
    delete stack;
    delete sendgroup;
}

void OTFConverter::addToSavedPartition(CommEvent * evt, int partition)
{
    while (trace->partitions->size() <= partition)
    {
        trace->partitions->append(new Partition());
        trace->partitions->last()->new_partition = trace->partitions->last();
    }
    Partition * p = trace->partitions->at(partition);
    p->addEvent(evt);
    evt->partition = p;
}

void OTFConverter::handleSavedAttributes(CommEvent * evt, EventRecord * er)
{
    evt->phase = er->ravel_info->value("phase");
    evt->step = er->ravel_info->value("step");

    for (QList<QString>::Iterator attr = rawtrace->metric_names->begin();
         attr != rawtrace->metric_names->end(); ++attr)
    {
        evt->metrics->addMetric(*attr, er->metrics->value(*attr),
                                er->metrics->value(*attr + "_agg"));
    }
}
