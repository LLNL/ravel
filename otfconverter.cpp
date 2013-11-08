#include "otfconverter.h"

OTFConverter::OTFConverter()
{
}

OTFConverter::~OTFConverter()
{
}

Trace * OTFConverter::importOTF(QString filename)
{
    OTFImporter * importer = new OTFImporter();
    rawtrace = importer->importOTF(filename.toStdString().c_str());
    trace = new Trace(rawtrace->num_processes);

    delete trace->functions;
    trace->functions = rawtrace->functions;

    delete trace->functionGroups;
    trace->functionGroups = rawtrace->functionGroups;
    for (QMap<int, QString>::Iterator itr = trace->functionGroups->begin(); itr != trace->functionGroups->end(); ++itr)
    {
        if (itr.value().contains("MPI")) {
            mpi_group = itr.key();
            break;
        }
    }

    // Convert the events
    mpi_events = new QVector<QList<Event *> * >(rawtrace->num_processes);
    for (int i = 0; i < rawtrace->num_processes; ++i)
        (*mpi_events)[i] = new QList<Event *>();
    matchEvents();

    // Match comms to events
    matchMessages();

    partitions = new QVector<Partition *>();
    // Partition - default
      // Partition by Process w or w/o Waitall
    if (true)
        initializePartitionsWaitall();
    else
        initializePartitions();

      // Merge communication

      // Tarjan
      // Merge by rank level

    // Partition - given
      // Form partitions given in some way -- need to write options for this [ later ]

    // Step

    // Calculate Step metrics

    delete importer;
    delete rawtrace;
    delete partitions; // Need not delete contained partitions

    // Delete only container, not Events
    for (QVector<QList<Event * > * >::Iterator itr = mpi_events->begin(); itr != mpi_events->end(); ++itr)
    {
        delete *itr;
        *itr = NULL;
    }
    delete mpi_events;

    return trace;
}

Partition * OTFConverter::mergePartitions(Partition * p1, Partition * p2)
{
    Partition * p = new Partition();

    // Copy events
    for (QMap<QVector<Partition *> *>::Iterator pitr = p1->events->begin(); pitr != p1->events->end(); ++pitr)
    {
        for (QVector<Partition *>::Iterator itr = (*pitr)->begin(); itr != (*pitr)->end(); ++itr)
        {
            p->addEvent(*itr);
        }
    }
    for (QMap<QVector<Partition *> *>::Iterator pitr = p2->events->begin(); pitr != p2->events->end(); ++pitr)
    {
        for (QVector<Partition *>::Iterator itr = (*pitr)->begin(); itr != (*pitr)->end(); ++itr)
        {
            p->addEvent(*itr);
        }
    }

    // Sort events
    p->sortEvents();
    return p;
}

void OTFConverter::initializePartitions()
{
    Partition * prev = NULL;
    for (QVector<QList<Event *> *>::Iterator pitr = mpi_events->begin(); pitr != mpi_events->end(); ++pitr) {
        for (QList<Event *>::Iterator itr = (*pitr)->begin(); itr != (*pitr)->end(); ++itr)
        {
            // Every event with messages becomes its own partition
            if ((*itr)->messages->size() > 0)
            {
                Partition * p = new Partition();
                p->addEvent(*itr);
                (*itr)->partition = p;
                (*(p->prev))[(*itr)->process] = prev;
                if (prev)
                    (*(prev->next))[(*itr->process)] = p;
                prev = p;
                (*partitions)->append(p);
            }
        }
    }
}

void OTFConverter::initializePartitionsWaitall()
{
    QList collective_ids(16);
    int collective_index = 0;
    int waitall_index = -1;
    QString collectives("MPI_BarrierMPI_BcastMPI_ReduceMPI_GatherMPI_ScatterMPI_AllgatherMPI_AllreduceMPI_AlltoallMPI_ScanMPI_Reduce_scatterMPI_Op_createMPI_Op_freeMPIMPI_AlltoallvMPI_AllgathervMPI_GathervMPI_Scatterv");
    for (QMap<int, Function * >::Iterator itr = trace->functions->begin(); itr != trace->functions->end(); ++itr)
    {
        if (itr.value()->group == mpi_group)
        {
            if ( collectives.contains(itr.value()->name) )
            {
                collective_ids[collective_index] = itr.key();
                ++collective_index;
            }
            if (itr.value()->name == "MPI_Waitall")
                waitall_index = itr.key();
        }
    }


    bool aggregating;
    QList<Event *> aggregation;
    Partition * prev = NULL;
    for (QVector<QList<Event *> *>::Iterator pitr = mpi_events->begin(); pitr != mpi_events->end(); ++pitr) {
        // We note these should be in reverse order because of the way they were added
        aggregating = false;
        for (QList<Event *>::Iterator itr = (*pitr)->begin(); itr != (*pitr)->end(); ++itr)
        {
            if (!aggregating)
            {
                // Is this an event that can stop aggregating?
                // 1. Due to a recv (including waitall recv)
                if ((*itr)->messages-size() > 0)
                {
                    bool recv_found = false;
                    for (QVector<Message *>::Iterator mitr = (*itr)->messages->begin(); mitr != (*itr)->end(); ++mitr)
                        if ((*mitr)->receiver == (*itr)->process) // Receive found
                        {
                            recv_found = true;
                            break;
                        }

                    if (recv_found)
                    {
                        // Do partition for aggregated stuff
                        Partition * p = new Partition();
                        for (QList<Event *>::Iterator eitr = aggregation.begin(); eitr != aggregation.end(); ++eitr)
                        {
                            p->addEvent(*eitr);
                            (*eitr)->partition = p;
                        }
                        (*(p->prev))[(*itr)->process] = prev;
                        if (prev)
                            (*(prev->next))[(*itr->process)] = p;
                        prev = p;
                        (*partitions)->append(p);

                        // Do partition for this recv
                        Partition * r = new Partition();
                        r->addEvent(*itr);
                        (*itr)->partition = r;
                        (*(r->prev))[(*itr)->process] = prev;
                        (*(prev->next))[(*itr->process)] = r;
                        prev = r;
                        (*partitions)->append(r);

                        aggregating = false;
                    }
                }

                // 2. Due to non-recv waitall or collective
                else if ((*itr)->function == waitall_index || collective_ids.contains((*itr)->function))
                {
                    // Do partition for aggregated stuff
                    Partition * p = new Partition();
                    for (QList<Event *>::Iterator eitr = aggregation.begin(); eitr != aggregation.end(); ++eitr)
                    {
                        p->addEvent(*eitr);
                        (*eitr)->partition = p;
                    }
                    (*(p->prev))[(*itr)->process] = prev;
                    if (prev)
                        (*(prev->next))[(*itr->process)] = p;
                    prev = p;
                    (*partitions)->append(p);

                    aggregating = false;
                }

                // Is this an event that should be added?
                else
                {
                    if ((*itr)->messages-size() > 0)
                        aggregation.prepend(*itr); // prepend since we're walking backwarsd
                }
            }
            else
            {
                // Is this an event that should cause aggregation?
                if ((*itr)->function == waitall_index)
                    aggregating = true;

                // Is this an event that should be added?
                if ((*itr)->messages->size() > 0)
                {
                    if (aggregating)
                    {
                        aggregation = QList<Event *>();
                        aggregation.prepend(*itr); // prepend since we're walking backwards
                    }
                    else
                    {
                        Partition * p = new Partition();
                        p->addEvent(*itr);
                        (*itr)->partition = p;
                        (*(p->prev))[(*itr)->process] = prev;
                        if (prev)
                            (*(prev->next))[(*itr->process)] = p;
                        prev = p;
                        (*partitions)->append(p);
                    }
                }
            } // Not aggregating
        }
    }
}

void OTFConverter::matchEvents()
{
    // We can handle each set of events separately
    for (QVector<QVector<EventRecord *> *>::Iterator pitr = rawtrace->events->begin(); pitr != rawtrace->events->end(); ++pitr) {
        QStack<Event *> * stack = new QStack<Event *>();
        int depth = 0;

        for (QVector<EventRecord *>::Iterator itr = (*pitr)->begin(); itr != (*pitr)->end(); ++itr)
        {
            if ((*itr)->value == 0) // End of a subroutine
            {
                Event * e = stack->pop();
                e->exit = (*itr)->time;
                if (!stack->isEmpty()) {
                    e->caller = stack->top();
                    (*(stack->top))->callees->append(e);
                }
                depth--;
            }
            else // Begin a subroutine
            {
                Event * e = new  Event((*itr)->time, 0, (*itr)->value, (*itr)->process -1, -1);

                e->depth = depth;
                if (depth == 0) {
                    (*(trace->roots))[(*itr)->process]->append(e);
                }
                depth++;

                if ((((*trace)->functions)[e->process])->group == mpi_group)
                    ((*mpi_events)[e->process])->prepend(e);

                stack->push(e);
                (*(trace->events))[(*itr)->process]->append(e);
            }
        }
        delete stack;
    }
}

// Let's determine which we need to step here rather than based on anything
// in matchEvents (though note this doesn't do the WaitAll yet), hrm.
void OTFConverter::matchMessages()
{
    for (QVector<CommRecord *>::Iterator itr = rawtrace->messages->begin(); itr != rawtrace->messages->end(); ++itr)
    {
        Message * m = new Message((*itr)->send_time, (*itr)->recv_time);

        Event * recv_evt = find_comm_event(search_child_ranges( (*((*(trace->roots))[(*itr)->receiver]))->children,
                                           (*itr->recv_time)), (*itr->recv_time));
        if (recv_evt) {
            recv_evt->messages->append(m);
            m->receiver = recv_evt;
        } else {
            std::cout << "Error finding recv event for " << (*itr)->sender << "->" << (*itr)->receiver
                      << " (" << (*itr)->send_time << ", " << (*itr)->recv_time << std::endl;
        }

        Event * send_evt = find_comm_event(search_child_ranges( (*((*(trace->roots))[(*itr)->sender]))->children,
                                           (*itr->send_time)), (*itr->send_time));
        if (send_evt) {
            send_evt->messages->append(m);
            m->sender = send_evt;
        } else {
            std::cout << "Error finding send event for " << (*itr)->sender << "->" << (*itr)->receiver
                      << " (" << (*itr)->send_time << ", " << (*itr)->recv_time << std::endl;
        }

    }
}

// Binary search for event containing time
Event * OTFConverter::search_child_ranges(QVector<Event *> * children, unsigned long long int time)
{
    int imin = 0;
    int imax = children->size() - 1;
    while (imax >= imin):
        imid = (imin + imax) / 2;
        if (((*(children))[imid])->exit < time)
            imin = imid + 1;
        else if (((*(children))[imid])->enter > time)
            imax = imid - 1;
        else
            return (*children)[imid];

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
