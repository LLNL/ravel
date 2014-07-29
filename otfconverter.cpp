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

    // May be used later to do partition by function
    QList<QList<CommEvent *> *> * allcomms = new QList<QList<CommEvent *> *>();

    // Used for waitall merging
    // We're now making groups of sends that must end in a Waitall/Testall
    // and not be interrupted by a Collective or a Receive.
    // This is backwards from the previous algorithm. We will end up collecting
    // lots of these sends but not adding them to the waitall since they
    // don't end in a waitall/testall. Instead we'll just clear them and
    // keep looking.
    QList<QList<Partition *> *> * waitallgroups = new QList<QList<Partition *> *>();
    QList<Partition *> * sendgroup = new QList<Partition *>();


    emit(matchingUpdate(1, "Constructing events..."));
    int progressPortion = std::max(round(rawtrace->num_processes / 1.0
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
    std::cout << "Num collectives " << rawtrace->collectives->size() << std::endl;
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

        QList<CommEvent *> * commevents = new QList<CommEvent *>();
        allcomms->append(commevents);

        QVector<CommRecord *> * sendlist = rawtrace->messages->at(i);
        QVector<CommRecord *> * recvlist = rawtrace->messages_r->at(i);
        std::cout << "Total comm records: " << sendlist->size() << std::endl;
        QList<P2PEvent *> * isends = new QList<P2PEvent *>();
        int sindex = 0, rindex = 0;
        CommEvent * prev = NULL;
        for (QVector<EventRecord *>::Iterator evt = event_list->begin();
             evt != event_list->end(); ++evt)
        {
            if ((*evt)->value == 0) // End of a subroutine
            {
                EventRecord * bgn = stack->pop();

                if (options->isendCoalescing && bgn->value != isend_index && isends->size() > 0)
                {
                    P2PEvent * isend = new P2PEvent(isends);
                    isend->comm_prev = isends->first()->comm_prev;
                    isends->first()->comm_prev->comm_next = isend;
                    makeSingletonPartition(isend);
                    prev = isend;
                    isends = new QList<P2PEvent *>();
                    if (options->waitallMerge && !options->partitionByFunction)
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
                    if (rawtrace->collectiveMap->at((*evt)->process)->contains(bgn->time))
                    {
                        cr = (*(rawtrace->collectiveMap->at((*evt)->process)))[bgn->time];
                    }

                    if (sindex < sendlist->size())
                    {
                        if (bgn->time == sendlist->at(sindex)->send_time)
                        {
                            sflag = true;
                            if (bgn->value == isend_index)
                                isendflag = true;
                        }
                        else if (bgn->time > sendlist->at(sindex)->send_time)
                        {
                            sindex++;
                            std::cout << "Error, skipping message (by send) at ";
                            std::cout << sendlist->at(sindex)->send_time << " on ";
                            std::cout << (*evt)->process << std::endl;
                        }
                    }

                    if (rindex < recvlist->size())
                    {
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
                }


                Event * e = NULL;
                if (cr)
                {
                    cr->events->append(new CollectiveEvent(bgn->time, (*evt)->time,
                                            bgn->value, bgn->process,
                                            phase, cr));
                    cr->events->last()->comm_prev = prev;
                    if (prev)
                        prev->comm_next = cr->events->last();
                    prev = cr->events->last();
                    e = cr->events->last();
                    if (options->partitionByFunction)
                        commevents->append(cr->events->last());
                    else
                        makeSingletonPartition(cr->events->last());

                    cpartcounter++;
                    // Any sends beforehand not end in a waitall.
                    if (options->waitallMerge && !options->partitionByFunction)
                    {
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
                    msgs->append(crec->message);
                    crec->message->sender = new P2PEvent(bgn->time, (*evt)->time,
                                                         bgn->value,
                                                         bgn->process, phase,
                                                         msgs);
                    if (isendflag)
                        isends->append(crec->message->sender);
                    crec->message->sender->comm_prev = prev;
                    if (prev)
                        prev->comm_next = crec->message->sender;
                    prev = crec->message->sender;
                    e = crec->message->sender;
                    if (options->partitionByFunction)
                        commevents->append(crec->message->sender);
                    else if (!options->isendCoalescing || !isendflag)
                        makeSingletonPartition(crec->message->sender);
                    sindex++;

                    spartcounter++;
                    // Collect the send for possible waitall merge
                    if (options->waitallMerge && !(options->isendCoalescing && isendflag)
                            && !options->partitionByFunction)
                    {
                        sendgroup->append(trace->partitions->last());
                    }
                }
                else if (rflag)
                {
                    QVector<Message *> * msgs = new QVector<Message *>();
                    CommRecord * crec = NULL;
                    while (rindex < recvlist->size() && (*evt)->time == recvlist->at(rindex)->recv_time)
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
                    if (prev)
                        prev->comm_next = msgs->at(0)->receiver;
                    prev = msgs->at(0)->receiver;

                    if (options->partitionByFunction)
                        commevents->append(msgs->at(0)->receiver);
                    else
                        makeSingletonPartition(msgs->at(0)->receiver);

                    rpartcounter++;

                    e = msgs->at(0)->receiver;

                    if (options->waitallMerge && !options->partitionByFunction)
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
                else
                {
                    e = new Event(bgn->time, (*evt)->time, bgn->value,
                                  bgn->process);

                    // Stop by Waitall/Testall
                    if (options->waitallMerge && !options->partitionByFunction
                        && (bgn->value == waitall_index || bgn->value == testall_index)
                        && sendgroup->size() > 0)
                    {
                        waitallgroups->append(sendgroup);
                        sendgroup = new QList<Partition *>();
                    }
                }

                depth--;
                e->depth = depth;
                if (depth == 0)
                    (*(trace->roots))[(*evt)->process]->append(e);

                if (e->exit > endtime)
                    endtime = e->exit;
                if (!stack->isEmpty())
                {
                    stack->top()->children.append(e);
                }
                for (QList<Event *>::Iterator child = (*evt)->children.begin();
                     child != (*evt)->children.end(); ++child)
                {
                    e->callees->append(*child);
                    (*child)->caller = e;
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
        // We assume these events are not communication
        while (!stack->isEmpty())
        {
            EventRecord * bgn = stack->pop();
            endtime = std::max(endtime, bgn->time);
            Event * e = new Event(bgn->time, endtime, bgn->value,
                          bgn->process);
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
            (*(trace->events))[bgn->process]->append(e);
            depth--;
        }

        // Finish off last isend list
        if (options->isendCoalescing && isends->size() > 0)
        {
            P2PEvent * isend = new P2PEvent(isends);
            isend->comm_prev = isends->first()->comm_prev;
            isends->first()->comm_prev->comm_next = isend;
            prev = isend;
        }
        else
        {
            delete isends;
        }

        // Prepare for next process
        stack->clear();
        sendgroup->clear();
    }
    delete stack;
    delete sendgroup;

    if (options->waitallMerge && !options->partitionByFunction)
    {
        // mergePartitions cleans up the extra structures
        // No we can't use this here because we don't have all
        // partitions, this will delete things not covered by a Waitall.
        // FIXME
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

    std::cout << "Number of partitions: " << trace->partitions->size() << std::endl;
    std::cout << "Part counters: c = " << cpartcounter << ", s = " << spartcounter << ", r = " << rpartcounter << std::endl;
}

// This is a lighter weight merge since we know at this stage everything stays
// within its own process and the partitions that must be merged are contiguous
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
