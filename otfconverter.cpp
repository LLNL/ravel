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
    rawtrace = importer->importOTF(filename.toStdString().c_str(),
                                   options->enforceMessageSizes);
    emit(finishRead());

    convert();

    delete importer;
    return trace;
}

Trace * OTFConverter::importOTF2(QString filename, OTFImportOptions *_options)
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
    return trace;
}

void OTFConverter::convert()
{
    // Time the rest of this
    QElapsedTimer traceTimer;
    qint64 traceElapsed;
    traceTimer.start();
    trace = new Trace(rawtrace->num_tasks);
    trace->units = rawtrace->second_magnitude;
    std::cout << "Trace units are " << trace->units << std::endl;

    // Set up collective metrics
    for (QMap<unsigned int, Counter *>::Iterator counter = rawtrace->counters->begin();
         counter != rawtrace->counters->end(); ++counter)
    {
        trace->metrics->append((counter.value())->name);
        //trace->metric_units->insert((counter.value())->name, (counter.value())->unit);
        trace->metric_units->insert((counter.value())->name,
                                    (counter.value())->name + " / time");
    }

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
    trace->tasks = rawtrace->tasks;
    trace->functionGroups = rawtrace->functionGroups;
    trace->collectives = rawtrace->collectives;
    trace->collectiveMap = rawtrace->collectiveMap;

    trace->taskgroups = rawtrace->taskgroups;
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
    int progressPortion = std::max(round(rawtrace->num_tasks / 1.0
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
                }

                // Partition/handle comm events
                CollectiveRecord * cr = NULL;
                sflag = false, rflag = false, isendflag = false;
                if (((*(trace->functions))[bgn->value])->group
                        == trace->mpi_group)
                {
                    // Since OTF2 is the present/future, perhaps we should do this by
                    // end time instead of begin time so we don't have to match on the OTF2 side.
                    //if (options->origin == OTFImportOptions::OF_OTF)
                    if (rawtrace->collectiveMap->at((*evt)->task)->contains(bgn->time))
                    {
                        cr = (*(rawtrace->collectiveMap->at((*evt)->task)))[bgn->time];
                    }

                    if (sindex < sendlist->size())
                    {
                        if (bgn->time <= sendlist->at(sindex)->send_time
                                && (*evt)->time >= sendlist->at(sindex)->send_time)
                        {
                            sflag = true;
                            if (bgn->value == isend_index)
                                isendflag = true;
                        }
                        else if (bgn->time > sendlist->at(sindex)->send_time)
                        {
                            std::cout << "Error, skipping message (by send) at ";
                            std::cout << sendlist->at(sindex)->send_time << " on ";
                            std::cout << (*evt)->task << std::endl;
                            sindex++;
                        }
                    }

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
                            std::cout << (*evt)->task << std::endl;
                            rindex++;
                        }
                    }
                }


                Event * e = NULL;
                if (cr)
                {
                    cr->events->append(new CollectiveEvent(bgn->time, (*evt)->time,
                                            bgn->value, bgn->task,
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
                        crec->message = new Message(crec->send_time, crec->recv_time);
                    }
                    if (crec->send_complete > max_complete)
                        max_complete = crec->send_complete;
                    msgs->append(crec->message);
                    crec->message->sender = new P2PEvent(bgn->time, (*evt)->time,
                                                         bgn->value,
                                                         bgn->task, phase,
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
                    else if (!options->isendCoalescing || !isendflag)
                        makeSingletonPartition(crec->message->sender);
                    sindex++;

                    spartcounter++;
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
                            crec->message = new Message(crec->send_time, crec->recv_time);
                        }
                        msgs->append(crec->message);
                        rindex++;
                    }
                    msgs->at(0)->receiver = new P2PEvent(bgn->time, (*evt)->time,
                                                         bgn->value,
                                                         bgn->task, phase,
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
                                  bgn->task);

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
                }

                depth--;
                e->depth = depth;
                if (depth == 0)
                    (*(trace->roots))[(*evt)->task]->append(e);

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

                (*(trace->events))[(*evt)->task]->append(e);
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

        // Deal with unclosed trace issues
        // We assume these events are not communication
        while (!stack->isEmpty())
        {
            EventRecord * bgn = stack->pop();
            endtime = std::max(endtime, bgn->time);
            Event * e = new Event(bgn->time, endtime, bgn->value,
                          bgn->task);
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
            (*(trace->events))[bgn->task]->append(e);
            depth--;
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
        }
        else
        {
            delete isends;
        }

        // Prepare for next task
        stack->clear();
        sendgroup->clear();
    }
    delete stack;
    delete sendgroup;

    if (!options->partitionByFunction
            && (options->waitallMerge
                || options->origin == OTFImportOptions::OF_OTF2))
    {
        mergeForWaitall(waitallgroups);
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

    // Check partitions
    /*int part_counter = 0;
    for (QList<Partition *>::Iterator part = trace->partitions->begin();
         part != trace->partitions->end(); ++part)
    {
        if ((*part)->events->size() < 1)
            std::cout << "Empty partition!  number: " << part_counter << std::endl;
        part_counter++;
    }*/
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
            /*double evt_time = evt->exit - evt->enter;
            double agg_time = evt->enter;
            if (evt->comm_prev)
                agg_time = evt->enter - evt->comm_prev->exit;
            evt->addMetric(rawtrace->counters->value(begin->counter)->name,
                           (end->value - begin->value) / 1.0 / evt_time,
                           (begin->value - last->value) / 1.0 / agg_time);
                           */
            evt->addMetric(rawtrace->counters->value(begin->counter)->name,
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

// This is a lighter weight merge since we know at this stage everything stays
// within its own task and the partitions that must be merged are contiguous
// Therefore we only need to merge the groups and remove the previous ones
// from the partition list -- so we create the new ones and point the old
// ones toward it so we can make a single pass to remove those that point to
// a new partition.
void OTFConverter::mergeForWaitall(QList<QList<Partition * > *> * groups)
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
            for (QMap<int, QList<CommEvent *> *>::Iterator proc
                 = (*part)->events->begin();
                 proc != (*part)->events->end(); ++proc)
            {
                for (QList<CommEvent *>::Iterator evt
                     = (proc.value())->begin();
                     evt != (proc.value())->end(); ++evt)
                {
                    (*evt)->partition = p;
                    p->addEvent(*evt);
                    p->new_partition = p;
                }
            }
        }
        newparts->append(p);
    }

    // Add in the partitions that did not change to new parts, save
    // others for deletion.
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
