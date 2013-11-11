#include "otfconverter.h"

OTFConverter::OTFConverter()
{
}

OTFConverter::~OTFConverter()
{
}

Trace * OTFConverter::importOTF(QString filename)
{
    // Start with the rawtrace similar to what we got from PARAVER
    OTFImporter * importer = new OTFImporter();
    rawtrace = importer->importOTF(filename.toStdString().c_str());
    trace = new Trace(rawtrace->num_processes);

    // Start setting up new Trace
    delete trace->functions;
    trace->functions = rawtrace->functions;

    delete trace->functionGroups;
    trace->functionGroups = rawtrace->functionGroups;

    // Find the MPI Group key
    for (QMap<int, QString>::Iterator itr = trace->functionGroups->begin(); itr != trace->functionGroups->end(); ++itr)
    {
        if (itr.value().contains("MPI")) {
            mpi_group = itr.key();
            break;
        }
    }

    // Convert the events into matching enter and exit
    mpi_events = new QVector<QList<Event *> * >(rawtrace->num_processes);
    for (int i = 0; i < rawtrace->num_processes; ++i)
        (*mpi_events)[i] = new QList<Event *>();
    matchEvents();

    // Match messages to previously connected events
    send_events = new QList<Event *>();
    matchMessages();
    chainCommEvents(); // Set comm_prev/comm_next

    partitions = new QList<Partition *>();
    dag_entries = new QList<Partition *>();
    dag_step_dict = new QMap<int, QSet<Partition *> *>();

    // Partition - default
    if (true)
    {
          // Partition by Process w or w/o Waitall
        if (true)
            initializePartitionsWaitall();
        else
            initializePartitions();

          // Merge communication
        mergeForMessages();
          // Tarjan
        mergeCycles();

          // Merge by rank level [ later ]
        if (false)
        {
            set_dag_steps();
            mergeByLeap();
        }
    }
    // Partition - given
      // Form partitions given in some way -- need to write options for this [ later ]

    // Step

    // Calculate Step metrics

    delete importer;
    delete rawtrace;
    delete partitions; // Need not delete contained partitions
    delete dag_entries;
    for (QMap<int, QSet<Partition *> *>::Iterator itr = dag_step_dict->begin(); itr != dag_step_dict->end(); ++itr)
    {
        delete *itr;
        *itr = NULL;
    }
    delete dag_step_dict;

    // Delete only container, not Events
    for (QVector<QList<Event * > * >::Iterator itr = mpi_events->begin(); itr != mpi_events->end(); ++itr)
    {
        delete *itr;
        *itr = NULL;
    }
    delete mpi_events;
    delete send_events;

    return trace;
}

// Merge the two given partitions to create a new partition
// We need not handle prev/next of each Event because those should never change.
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

void OTFConverter::mergeByLeap()
{

}

// Merges partitions that are connected by a message
// We go through all send events and merge them with all recvs
// connected to the event (there should only be 1)
void OTFConverter::mergeForMessages()
{
    Partition * p, p1, p2;
    for (QList<Event *>::Iterator eitr = send_events->begin(); eitr != send_events->end(); ++eitr)
    {
        p1 = (*eitr)->partition;
        for (QVector<Message *>::Iterator itr = (*eitr)->messages->begin(); itr != (*eitr)->messages->end(); ++itr)
        {
            p2 = (*itr)->receiver->partition;
            if (p1 != p2)
            {
                p = mergePartitions(p1, p2);
                partitions->removeAll(p1);
                partitions->removeAll(p2);
                partitions->append(p);
                delete p1;
                delete p2;
                p1 = p; // For the rest of the messages connected to this send. [ Should only be one, shouldn't matter. ]
            }
        }
    }
}

// Looping section of Tarjan algorithm
void OTFConverter::strong_connect_loop(Partition * part, QStack<Partition *> * stack,
                                      QList<Partition *> * children, int cIndex,
                                      QStack<RecurseInfo *> * recurse,
                                      QList<QList<Partition *> *> * components)
{
    while (cIndex < children->size())
    {
        Partition * child = (*children)[cIndex];
        if (child->tindex < 0)
        {
            // Push onto recursion stack, return so we can deal with it
            recurse->push(new RecurseInfo(part, child, children, cIndex));
            recurse->push(new RecurseInfo(child, NULL, NULL, -1));
            return;
        }
        else if (stack->contains(child))
        {
            // Child already marked, set lowlink and proceed to next child
            part->lowlink = std::min(part->lowlink, child->tindex);
        }
        ++cIndex;
    }

    // After all children have been handled, process component
    if (part->lowlink == part->tindex)
    {
        component = new QList<Partition *>();
        Partition * vert = stack->pop();
        while (vert != part)
        {
            component->append(vert);
            vert = stack->pop();
        }
        component->append(part);
        components->append(component);
    }
}

// Iteration portion of Tarjan algorithm
int OTFConverter::strong_connect_iter(Partition * partition, QStack<Partition *> * stack,
                                      QList<QList<Partition *> > * components, int index)
{
    QStack<RecurseInfo *> * recurse = new QStack<RecurseInfo *>();
    recurse->append(new RecurseInfo(partition, NULL, NULL, -1));
    while (recurse->size() > 0)
    {
        RecurseInfo * ri = recurse->pop();

        if (ri->cIndex >= 0)
        {
            ri->part->tindex = index;
            ri->part->lowlink = index;
            ++index;
            stack->push(ri->part);
            ri->children = new QList<Partition *>();
            for (QSet<Partition *>::Iterator itr = ri->part->children->begin(); itr != ri->part->children->end(); ++itr)
                ri->children->append(*itr);

            strong_connect_loop(ri->part, stack, ri->part->children, 0, recurse, components);
        }
        else
        {
            ri->part->lowlink = std::min(ri->part->lowlink, ri->child->lowlink);
            strong_connect_loop(ri->part, stack, ri->children, ++(ri->cIndex), recurse, components);
        }

        delete ri->children;
        delete ri;
    }

    delete recurse;
    return index;
}

// Main outer loop of Tarjan algorithm
QList<QList<Partition *> *> * OTFConverter::tarjan()
{
    // Initialize
    for (QList<Partition *>::Iterator itr = partitions->begin(); itr != partitions->end(); ++itr)
    {
        (*itr)->tindex = -1;
        (*itr)->lowlink = -1;
    }

    int index = 0;
    QStack<Partition *> * stack = new QStack<Partition *>();
    QList<QList<Partition *> *> * components = new QList<QList<Partition *> *>();
    for (QList<Partition *>::Iterator itr = partitions->begin(); itr != partitions->end(); ++itr)
        if ((*itr)->tindex < 0)
            index = strong_connect_iter((*itr), stack, components, index);

    delete stack;
    return components;
}

// Goes through current partitions and merges cycles
void OTFConverter::mergeCycles()
{
    // Determine partition parents/children through dag
    // and then determine strongly connected components (SCCs) with tarjan.
    set_partition_dag();
    QList<QList<Partition *> *> * components = tarjan();

    // Go through the SCCs and merge them into single partitions
    QList<Partition *> * merged = QList<Partition *>();
    for (QList<QList<Partition *> *>::Iterator citr = components->begin(); citr != components->end(); ++citr)
    {
        // If SCC is single partition, keep it
        if ((*citr)->size() == 1)
        {
            Partition * p = (*citr)[0];
            p->old_parents = p->parents;
            p->old_children = p->children;
            p->new_partition = p;
            p->parents = QSet<Partition *>();
            p->children = QSet<Partition *>();
            merged->append(p);
            continue;
        }

        // Otherwise, iterate through the SCC and merge into new partition
        Partition * p = new Partition();
        for (QList<Partition *>::Iterator itr = (*citr)->begin(); itr != (*citr)->end(); ++itr)
        {
            (*itr)->new_partition = p;

            // Merge all the events into the new partition
            QList<int> keys = (*itr)->events->keys();
            for (QList<int>::Iterator k = keys.begin(); k != keys.end(); ++k)
            {
                if (p->events->contains(*k))
                {
                    (*(p->events))[*k] += (*((*itr)->events))[*k];
                }
                else
                {
                    (*(p->events))[*k] = new QList<Event *>();
                    (*(p->events))[*k] += (*((*itr)->events))[*k];
                }
            }

            // Set old_children and old_parents from the children and parents of the partition to merge
            for (QSet<Partition *>::Iterator pitr = (*itr)->children->begin(); pitr != (*itr)->children->end(); ++pitr)
                if (!((*citr)->contains(*itr))) // but only if parent/child not already in SCC
                    p->old_children->insert(*itr);
            for (QSet<Partition *>::Iterator pitr = (*itr)->parents->begin(); pitr != (*itr)->parents->end(); ++pitr)
                if (!((*citr)->contains(*itr)))
                    p->old_parents->insert(*itr);
        }

        merged->append(p);
    }

    // Now that we have all the merged partitions, figure out parents/children between them,
    // and sort the event lists and such
    // Note we could just set the event partition adn then use set_partition_dag... in theory
    bool parent_flag;
    dag_entries->removeAll();
    for (QList<Partition *>::Iterator pitr = merged->begin(); pitr != merged->end(); ++pitr)
    {
        parent_flag = false;

        // Update parents/children by taking taking the old parents/children and the new partition they belong to
        for (QSet<Partition *>::Iterator itr = (*pitr)->old_children->begin(); itr != (*pitr)->old_children->end(); ++itr)
            if ((*itr)->new_partition)
                (*pitr)->children->insert((*itr)->new_partition);
            else
                std::cout << "Error, no new partition set on child" << std::endl;
        for (QSet<Partition *>::Iterator itr = (*pitr)->old_parents->begin(); itr != (*pitr)->old_parents->end(); ++itr)
            if ((*itr)->new_partition) {
                (*pitr)->parents->insert((*itr)->new_partition);
                parent_flag = true;
            }
            else
                std::cout << "Error, no new partition set on parent" << std::endl;

        if (!parent_flag)
            dag_entries->append(*pitr);

        // Delete/reset unneeded old stuff
        delete (*pitr)->old_children;
        delete (*pitr)->old_parents;
        (*pitr)->old_children = QSet<Partition *>();
        (*pitr)->old_parents = QSet<Partition *>();

        // Sort Events
        (*pitr)->sortEvents();

        // Set Event partition for all of the events.
        for (QMap<int, QVector<Event *> *>::Iterator eitr = (*pitr)->events->begin(); eitr != (*pitr)->events->end(); ++eitr) {
            for (QVector<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
                (*itr)->partition = (*pitr);
            }
        }
    }

    delete partitions;
    partitions = merged;

    // Clean up by deleting all of the old partitions through the components
    for (QList<QList<Partition *> *>::Iterator citr = components->begin(); citr != components->end(); ++citr)
    {
        for (QList<Partition *>::Iterator itr = (*citr)->begin(); itr != (*citr)->end(); ++itr)
        {
            if ((*itr)->new_partition != (*itr)) // Don't delete the singleton SCCs as we keep those
            {
                delete *itr;
                *itr = NULL;
            }
        }
        delete *citr;
        *citr = NULL;
    }
    delete components;

}

// Set all the parents/children in the partition by looking at the partitions of the events in them.
void OTFConverter::set_partition_dag()
{
    dag_entries->removeAll();
    bool parent_flag;
    for (QList<Partition *>::Iterator pitr = partitions->begin(); pitr != partitions->end(); ++pitr)
    {
        parent_flag = false;
        for (QMap<int, QVector<Event *> *>::Iterator eitr = (*pitr)->events->begin(); eitr != (*pitr)->events->end(); ++eitr) {
            if ((*eitr)->first()->comm_prev->partition)
            {
                ((*pitr)->parents)->insert((*eitr)->first()->comm_prev->partition);
                parent_flag = true;
            }
            if ((*eitr)->last()->comm_next->partition)
                ((*pitr)->children)->insert((*eitr)->last()->comm_next->partition);
        }

        if (!parent_flag)
            dag_entries->append(*pitr);
    }
}

void OTFConverter::set_dag_steps()
{
    // Clear current dag steps
    for (QList<Partition *>::Iterator itr = partitions->begin(); itr != partitions->end(); ++itr)
        (*itr)->dag_leap = -1;

    dag_step_dict->clear();
    QSet<Partition *> current_level = QSet::fromList(&(*dag_entries));
    int accumulated_leap;
    bool allParentsFlag;
    while (!current_level.isEmpty())
    {
        QSet<Partition *> next_level();
        for (QSet<Partition *>::Iterator itr = current_level.begin(); itr != current_level.end(); ++itr)
        {
            accumulated_leap = 0;
            allParentsFlag = true;
            if ((*itr)->dag_leap >= 0) // Already handled
                continue;

            // Deal with parents. If there are any unhandled, we set them to be
            // dealt with next and mark allParentsFlag false so we can put off this one.
            for (QSet<Partition *>::Iterator pitr = (*itr)->parents->begin(); pitr != (*itr)->parents->end(); ++pitr)
            {
                if ((*pitr)->dag_leap < 0)
                {
                    next_level.insert(*pitr);
                    allParentsFlag = false;
                }
                accumulated_step = std::max(accumulated_leap, (*pitr)->dag_leap);
            }

            // Still need to handle parents
            if (!allParentsFlag)
                continue;

            // All parents were handled, so we can set our steps
            (*itr)->dag_leap = accumulated_leap;
            if (!dag_step_dict->contains((*itr)->dag_leap))
                (*dag_step_dict)[(*itr)->dag_leap] = new QSet<Partition *>();
            ((*dag_step_dict)[(*itr)->dag_leap])->insert(*itr);

            for (QSet<Partition *>::Iterator citr = (*itr)->children->begin(); citr != (*itr)->children->end(); ++citr)
                next_level.insert(*citr);
        }
        current_level = next_level;
    }
}

// Every send/recv event becomes its own partition
void OTFConverter::initializePartitions()
{
    for (QVector<QList<Event *> *>::Iterator pitr = mpi_events->begin(); pitr != mpi_events->end(); ++pitr) {
        for (QList<Event *>::Iterator itr = (*pitr)->begin(); itr != (*pitr)->end(); ++itr)
        {
            // Every event with messages becomes its own partition
            if ((*itr)->messages->size() > 0)
            {
                Partition * p = new Partition();
                p->addEvent(*itr);
                (*itr)->partition = p;
                (*partitions)->append(p);
            }
        }
    }
}

// Waitalls determien if send/recv events are grouped along a process
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
                        (*partitions)->append(p);

                        // Do partition for this recv
                        Partition * r = new Partition();
                        r->addEvent(*itr);
                        (*itr)->partition = r;
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
                        (*partitions)->append(p);
                    }
                }
            } // Not aggregating
        }
    }
}

// Determine events as blocks of matching enter and exit
void OTFConverter::matchEvents()
{
    // We can handle each set of events separately
    QStack<Event *> * stack = new QStack<Event *>();
    for (QVector<QVector<EventRecord *> *>::Iterator pitr = rawtrace->events->begin(); pitr != rawtrace->events->end(); ++pitr) {
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
                if (depth == 0)
                    (*(trace->roots))[(*itr)->process]->append(e);
                depth++;

                // Keep track of the mpi_events for partitioning
                if ((((*trace)->functions)[e->process])->group == mpi_group)
                    ((*mpi_events)[e->process])->prepend(e);

                stack->push(e);
                (*(trace->events))[(*itr)->process]->append(e);
            }
        }
        stack->clear();
    }
}

// Create connectors of prev/next between send/recv events.
void OTFConverter::chainCommEvents()
{
    // Note that mpi_events go backwards in time, so as we iterate through, the event we just processed
    // is the next event of the one we are processing.
    for (QVector<QList<Event *> *>::Iterator pitr = mpi_events->begin(); pitr != mpi_events->end(); ++pitr) {
        Event * next = NULL;
        for (QList<Event *>::Iterator itr = (*pitr)->begin(); itr != (*pitr)->end(); ++itr) {
            if ((*itr)->messages->size() > 0) {
                (*itr)->comm_next = next;
                if (next)
                    next->comm_prev = (*itr);
                next = (*itr);
            }
        }
    }
}

// Match the messages to the events.
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
            send_events->append(send_evt); // Keep track of the send events for merging later
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
