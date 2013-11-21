#include "trace.h"
#include <iostream>

Trace::Trace(int np, bool legacy) : num_processes(np), isLegacy(legacy)
{
    functionGroups = new QMap<int, QString>();
    functions = new QMap<int, Function *>();
    partitions = new QList<Partition *>();

    events = new QVector<QVector<Event *> *>(np);
    roots = new QVector<QVector<Event *> *>(np);
    for (int i = 0; i < np; i++) {
        (*events)[i] = new QVector<Event *>();
        (*roots)[i] = new QVector<Event *>();
    }

    send_events = new QList<Event *>();

    isProcessed = false;
}

Trace::Trace(int np)
{
    Trace(np, false);
}

Trace::~Trace()
{
    delete functionGroups;

    for (QMap<int, Function *>::Iterator itr = functions->begin(); itr != functions->end(); ++itr)
    {
        delete (itr.value());
        itr.value() = NULL;
    }
    delete functions;

    for (QList<Partition *>::Iterator itr = partitions->begin(); itr != partitions->end(); ++itr) {
        (*itr)->deleteEvents();
        delete *itr;
        *itr = NULL;
    }
    delete partitions;

    for (QVector<QVector<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        for (QVector<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete events;
    delete send_events;

    // Don't need to delete Events, they were deleted above
    for (QVector<QVector<Event *> *>::Iterator eitr = roots->begin(); eitr != roots->end(); ++eitr) {
        delete *eitr;
        *eitr = NULL;
    }
    delete roots;
}

void Trace::preprocess()
{
    partition();
    assignSteps();

    isProcessed = true;
}

void Trace::partition()
{

    chainCommEvents(); // Set comm_prev/comm_next

    partitions = new QList<Partition *>();
    dag_entries = new QList<Partition *>();
    dag_step_dict = new QMap<int, QSet<Partition *> *>();

    // Partition - default
    if (true)
    {
          // Partition by Process w or w/o Waitall
        if (false)
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

}

void Trace::calculate_lateness()
{
    QSet<Partition *> * current_leap = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin(); part != dag_entries->end(); ++part)
        current_leap->insert(*part);

    int accumulated_step;
    global_max_step = 0;

    while (!current_leap->isEmpty())
    {
        QSet<Partition *> * next_leap = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator part = current_leap->begin(); part != current_leap->end(); ++part)
        {
            accumulated_step = 0;
            bool allParents = true;
            if ((*part)->max_global_step >= 0) // We already handled this parent
                continue;
            for (QSet<Partition *>::Iterator parent = (*part)->parents->begin(); parent != (*part)->parents->end(); ++parent)
            {   // Check all parents to make sure they have all been handled
                if ((*parent)->max_global_step < 0)
                {
                    next_leap->insert(*parent); // So next time we must handle parent first
                    allParents = false;
                }
                // Find maximum step of all predecessors
                // We +1 because individual steps start at 0, so when we add 0, we want
                // it to be offset from teh parent
                accumulated_step = std::max(accumulated_step, (*parent)->max_global_step + 1);
            }

            // Skip since all parents haven't been handled
            if (!allParents)
                continue;

            // Set steps for the partition
            (*part)->max_global_step = (*part)->max_step + accumulated_step;
            (*part)->min_global_step = accumulated_step;

            // Set steps for partition events
            for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list)
                for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
                    (*evt)->step += accumulated_step;

            // Add children for handling
            for (QSet<Partition *>::Iterator child = (*part)->children->begin(); child != (*part)->children->end(); ++child)
                next_leap->insert(*child);

            // Keep track of global max step
            global_max_step = std::max(global_max_step, (*part)->max_global_step);
        }

        delete current_leap;
        current_leap = next_leap;
    }
    delete current_leap;
}

void Trace::set_global_steps()
{
    // Go through dag, generating steps
    QSet<Partition *> * active_partitions = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin(); part != dag_entries->end(); ++part)
        active_partitions->insert(*part);

    int active_step = 0;
    unsigned long long int mintime, aggmintime;

    // For each step, find all the events in the active partitions with that step
    // and calculate lateness
    // Active partition group may change every time the step is changed.
    for (int i = 0; i <= global_max_step; ++i)
    {
        QList<Event *> * i_list = new QList<Event *>();
        for (QSet<Partition *>::Iterator part = active_partitions->begin(); part != active_partitions->end(); ++part)
            for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list)
                for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
                    if ((*evt)->step == i)
                        i_list->append(*evt);

        // Find min leave time
        mintime = ULLONG_MAX;
        aggmintime = ULLONG_MAX;
        for (QList<Event *>::Iterator evt = i_list->begin(); evt != i_list->end(); ++evt)
        {
            if ((*evt)->exit < mintime)
                mintime = (*evt)->exit;
            if ((*evt)->enter < aggmintime)
                aggmintime = (*evt)->enter;
        }

        // Set lateness
        for (QList<Event *>::Iterator evt = i_list->begin(); evt != i_list->end(); ++evt)
            (*evt)->addMetric("Lateness", (*evt)->exit - mintime, (*evt)->enter - aggmintime);

        // Prepare active step for the next round
        for (QSet<Partition *>::Iterator part = active_partitions->begin(); part != active_partitions->end(); ++part)
            if ((*part)->max_global_step == i)
            {
                active_partitions->remove(*part);
                for (QSet<Partition *>::Iterator child = (*part)->children->begin(); part != (*part)->children->end(); ++child)
                    if ((*child)->min_global_step == i + 1) // Only insert if we're the max parent, otherwise wait for max parent
                        active_partitions->insert(*child);
            }
        delete i_list;
    }
    delete active_partitions;
}

void Trace::assignSteps()
{
    // Step
    for (QList<Partition *>::Iterator partition = partitions->begin(); partition != partitions->end(); ++partition)
        (*partition)->step();

    // Global steps
    set_global_steps();

    // Calculate Step metrics
    calculate_lateness();

    /*delete dag_entries;
    for (QMap<int, QSet<Partition *> *>::Iterator itr = dag_step_dict->begin(); itr != dag_step_dict->end(); ++itr)
    {
        delete itr.value();
    }
    delete dag_step_dict;

    // Delete only container, not Events
    for (QVector<QList<Event * > * >::Iterator itr = mpi_events->begin(); itr != mpi_events->end(); ++itr)
    {
        delete *itr;
        *itr = NULL;
    }
    delete mpi_events;
    delete send_events; */
}


// Create connectors of prev/next between send/recv events.
void Trace::chainCommEvents()
{
    // Note that mpi_events go backwards in time, so as we iterate through, the event we just processed
    // is the next event of the one we are processing.
    for (QVector<QList<Event *> *>::Iterator event_list = mpi_events->begin(); event_list != mpi_events->end(); ++event_list) {
        Event * next = NULL;
        for (QList<Event *>::Iterator evt = (*event_list)->begin(); evt != (*event_list)->end(); ++evt) {
            if ((*evt)->messages->size() > 0) {
                (*evt)->comm_next = next;
                if (next)
                    next->comm_prev = (*evt);
                next = (*evt);
            }
        }
    }
}

// Merge the two given partitions to create a new partition
// We need not handle prev/next of each Event because those should never change.
Partition * Trace::mergePartitions(Partition * p1, Partition * p2)
{
    Partition * p = new Partition();

    // Copy events
    for (QMap<int, QList<Event *> *>::Iterator event_list = p1->events->begin(); event_list != p1->events->end(); ++event_list)
    {
        for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
        {
            p->addEvent(*evt);
        }
    }
    for (QMap<int, QList<Event *> *>::Iterator event_list = p2->events->begin(); event_list != p2->events->end(); ++event_list)
    {
        for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
        {
            p->addEvent(*evt);
        }
    }

    // Sort events
    p->sortEvents();

    return p;
}

void Trace::mergeByLeap()
{

}

// Merges partitions that are connected by a message
// We go through all send events and merge them with all recvs
// connected to the event (there should only be 1)
void Trace::mergeForMessages()
{
    Partition * p, * p1, * p2;
    for (QList<Event *>::Iterator send = send_events->begin(); send != send_events->end(); ++send)
    {
        p1 = (*send)->partition;
        for (QVector<Message *>::Iterator msg = (*send)->messages->begin(); msg != (*send)->messages->end(); ++msg)
        {
            p2 = (*msg)->receiver->partition;
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
void Trace::strong_connect_loop(Partition * part, QStack<Partition *> * stack,
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
        QList<Partition *> * component = new QList<Partition *>();
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
int Trace::strong_connect_iter(Partition * partition, QStack<Partition *> * stack,
                                      QList<QList<Partition *> *> * components, int index)
{
    QStack<RecurseInfo *> * recurse = new QStack<RecurseInfo *>();
    recurse->push(new RecurseInfo(partition, NULL, NULL, -1));
    while (recurse->size() > 0)
    {
        RecurseInfo * ri = recurse->pop();

        if (ri->cIndex < 0)
        {
            ri->part->tindex = index;
            ri->part->lowlink = index;
            ++index;
            stack->push(ri->part);
            ri->children = new QList<Partition *>();
            std::cout << "ri part children " << ri->part->children << std::endl;
            std::cout << "          size: " << ri->part->children->size() << std::endl;
            for (QSet<Partition *>::Iterator child = ri->part->children->begin(); child != ri->part->children->end(); ++child)
                ri->children->append(*child);

            strong_connect_loop(ri->part, stack, ri->children, 0, recurse, components);
        }
        else
        {
            std::cout << "ri low" << ri->part->lowlink << std::endl;
            std::cout << "ri ch low" << ri->child->lowlink << std::endl;
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
QList<QList<Partition *> *> * Trace::tarjan()
{
    // Initialize
    for (QList<Partition *>::Iterator partition = partitions->begin(); partition != partitions->end(); ++partition)
    {
        (*partition)->tindex = -1;
        (*partition)->lowlink = -1;
    }

    int index = 0;
    QStack<Partition *> * stack = new QStack<Partition *>();
    QList<QList<Partition *> *> * components = new QList<QList<Partition *> *>();
    for (QList<Partition *>::Iterator partition = partitions->begin(); partition != partitions->end(); ++partition)
        if ((*partition)->tindex < 0)
            index = strong_connect_iter((*partition), stack, components, index);

    delete stack;
    return components;
}

// Goes through current partitions and merges cycles
void Trace::mergeCycles()
{
    // Determine partition parents/children through dag
    // and then determine strongly connected components (SCCs) with tarjan.
    set_partition_dag();
    QList<QList<Partition *> *> * components = tarjan();

    // Go through the SCCs and merge them into single partitions
    QList<Partition *> * merged = new QList<Partition *>();
    for (QList<QList<Partition *> *>::Iterator component = components->begin(); component != components->end(); ++component)
    {
        // If SCC is single partition, keep it
        if ((*component)->size() == 1)
        {
            Partition * p = (*component)->first();
            p->old_parents = p->parents;
            p->old_children = p->children;
            p->new_partition = p;
            p->parents = new QSet<Partition *>();
            p->children = new QSet<Partition *>();
            merged->append(p);
            continue;
        }

        // Otherwise, iterate through the SCC and merge into new partition
        Partition * p = new Partition();
        for (QList<Partition *>::Iterator partition = (*component)->begin(); partition != (*component)->end(); ++partition)
        {
            (*partition)->new_partition = p;

            // Merge all the events into the new partition
            QList<int> keys = (*partition)->events->keys();
            for (QList<int>::Iterator k = keys.begin(); k != keys.end(); ++k)
            {
                if (p->events->contains(*k))
                {
                    *((*(p->events))[*k]) += *((*((*partition)->events))[*k]);
                }
                else
                {
                    (*(p->events))[*k] = new QList<Event *>();
                    *((*(p->events))[*k]) += *((*((*partition)->events))[*k]);
                }
            }

            // Set old_children and old_parents from the children and parents of the partition to merge
            for (QSet<Partition *>::Iterator child = (*partition)->children->begin(); child != (*partition)->children->end(); ++child)
                if (!((*component)->contains(*partition))) // but only if parent/child not already in SCC
                    p->old_children->insert(*partition);
            for (QSet<Partition *>::Iterator parent = (*partition)->parents->begin(); parent != (*partition)->parents->end(); ++parent)
                if (!((*component)->contains(*partition)))
                    p->old_parents->insert(*partition);
        }

        merged->append(p);
    }

    // Now that we have all the merged partitions, figure out parents/children between them,
    // and sort the event lists and such
    // Note we could just set the event partition adn then use set_partition_dag... in theory
    bool parent_flag;
    dag_entries->clear();
    for (QList<Partition *>::Iterator partition = merged->begin(); partition != merged->end(); ++partition)
    {
        parent_flag = false;

        // Update parents/children by taking taking the old parents/children and the new partition they belong to
        for (QSet<Partition *>::Iterator child = (*partition)->old_children->begin(); child != (*partition)->old_children->end(); ++child)
            if ((*child)->new_partition)
                (*partition)->children->insert((*child)->new_partition);
            else
                std::cout << "Error, no new partition set on child" << std::endl;
        for (QSet<Partition *>::Iterator parent = (*partition)->old_parents->begin(); parent != (*partition)->old_parents->end(); ++parent)
            if ((*parent)->new_partition) {
                (*partition)->parents->insert((*parent)->new_partition);
                parent_flag = true;
            }
            else
                std::cout << "Error, no new partition set on parent" << std::endl;

        if (!parent_flag)
            dag_entries->append(*partition);

        // Delete/reset unneeded old stuff
        delete (*partition)->old_children;
        delete (*partition)->old_parents;
        (*partition)->old_children = new QSet<Partition *>();
        (*partition)->old_parents = new QSet<Partition *>();

        // Sort Events
        (*partition)->sortEvents();

        // Set Event partition for all of the events.
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*partition)->events->begin(); event_list != (*partition)->events->end(); ++event_list) {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt) {
                (*evt)->partition = (*partition);
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
void Trace::set_partition_dag()
{
    dag_entries->clear();
    bool parent_flag;
    for (QList<Partition *>::Iterator partition = partitions->begin(); partition != partitions->end(); ++partition)
    {
        parent_flag = false;
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*partition)->events->begin(); event_list != (*partition)->events->end(); ++event_list) {
            std::cout << "Event list size: " << event_list.value()->size() << std::endl;
            if ((event_list.value())->first()->comm_prev)
            {
                std::cout << "comm prev " << event_list.value()->first()->comm_prev << std::endl;
                std::cout << "parents " << (*partition)->parents << std::endl;
                std::cout << "parents size " << (*partition)->parents->size() << std::endl;
                (*partition)->parents->insert(event_list.value()->first()->comm_prev->partition);
                parent_flag = true;
            }
            if ((event_list.value())->last()->comm_next)
                (*partition)->children->insert(event_list.value()->last()->comm_next->partition);
        }

        if (!parent_flag)
            dag_entries->append(*partition);
    }
}

void Trace::set_dag_steps()
{
    // Clear current dag steps
    for (QList<Partition *>::Iterator partition = partitions->begin(); partition != partitions->end(); ++partition)
        (*partition)->dag_leap = -1;

    dag_step_dict->clear();
    QSet<Partition *> * current_level = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin(); part != dag_entries->end(); ++part)
        current_level->insert(*part);
    int accumulated_leap;
    bool allParentsFlag;
    while (!current_level->isEmpty())
    {
        QSet<Partition *> * next_level = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator partition = current_level->begin(); partition != current_level->end(); ++partition)
        {
            accumulated_leap = 0;
            allParentsFlag = true;
            if ((*partition)->dag_leap >= 0) // Already handled
                continue;

            // Deal with parents. If there are any unhandled, we set them to be
            // dealt with next and mark allParentsFlag false so we can put off this one.
            for (QSet<Partition *>::Iterator parent = (*partition)->parents->begin(); parent != (*partition)->parents->end(); ++parent)
            {
                if ((*parent)->dag_leap < 0)
                {
                    next_level->insert(*parent);
                    allParentsFlag = false;
                }
                accumulated_leap = std::max(accumulated_leap, (*parent)->dag_leap);
            }

            // Still need to handle parents
            if (!allParentsFlag)
                continue;

            // All parents were handled, so we can set our steps
            (*partition)->dag_leap = accumulated_leap;
            if (!dag_step_dict->contains((*partition)->dag_leap))
                (*dag_step_dict)[(*partition)->dag_leap] = new QSet<Partition *>();
            ((*dag_step_dict)[(*partition)->dag_leap])->insert(*partition);

            for (QSet<Partition *>::Iterator child = (*partition)->children->begin(); child != (*partition)->children->end(); ++child)
                next_level->insert(*child);
        }
        delete current_level;
        current_level = next_level;
    }
    delete current_level;
}

// Every send/recv event becomes its own partition
void Trace::initializePartitions()
{
    for (QVector<QList<Event *> *>::Iterator event_list = mpi_events->begin(); event_list != mpi_events->end(); ++event_list) {
        for (QList<Event *>::Iterator evt = (*event_list)->begin(); evt != (*event_list)->end(); ++evt)
        {
            // Every event with messages becomes its own partition
            if ((*evt)->messages->size() > 0)
            {
                Partition * p = new Partition();
                p->addEvent(*evt);
                (*evt)->partition = p;
                partitions->append(p);
            }
        }
    }
}

// Waitalls determien if send/recv events are grouped along a process
void Trace::initializePartitionsWaitall()
{
    QVector<int> * collective_ids = new QVector<int>(16);
    int collective_index = 0;
    int waitall_index = -1;
    QString collectives("MPI_BarrierMPI_BcastMPI_ReduceMPI_GatherMPI_ScatterMPI_AllgatherMPI_AllreduceMPI_AlltoallMPI_ScanMPI_Reduce_scatterMPI_Op_createMPI_Op_freeMPIMPI_AlltoallvMPI_AllgathervMPI_GathervMPI_Scatterv");
    for (QMap<int, Function * >::Iterator function = functions->begin(); function != functions->end(); ++function)
    {
        if (function.value()->group == mpi_group)
        {
            if ( collectives.contains(function.value()->name) )
            {
                (*collective_ids)[collective_index] = function.key();
                ++collective_index;
            }
            if (function.value()->name == "MPI_Waitall")
                waitall_index = function.key();
        }
    }


    bool aggregating;
    QList<Event *> * aggregation;
    for (QVector<QList<Event *> *>::Iterator event_list = mpi_events->begin(); event_list != mpi_events->end(); ++event_list) {
        // We note these should be in reverse order because of the way they were added
        aggregating = false;
        for (QList<Event *>::Iterator evt = (*event_list)->begin(); evt != (*event_list)->end(); ++evt)
        {
            if (!aggregating)
            {
                // Is this an event that can stop aggregating?
                // 1. Due to a recv (including waitall recv)
                if ((*evt)->messages->size() > 0)
                {
                    bool recv_found = false;
                    for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
                        if ((*msg)->receiver->process == (*evt)->process) // Receive found
                        {
                            recv_found = true;
                            break;
                        }

                    if (recv_found)
                    {
                        // Do partition for aggregated stuff
                        Partition * p = new Partition();
                        for (QList<Event *>::Iterator saved_evt = aggregation->begin(); saved_evt != aggregation->end(); ++saved_evt)
                        {
                            p->addEvent(*saved_evt);
                            (*saved_evt)->partition = p;
                        }
                        partitions->append(p);

                        // Do partition for this recv
                        Partition * r = new Partition();
                        r->addEvent(*evt);
                        (*evt)->partition = r;
                        partitions->append(r);

                        aggregating = false;
                        delete aggregation;
                    }
                }

                // 2. Due to non-recv waitall or collective
                else if ((*evt)->function == waitall_index || collective_ids->contains((*evt)->function))
                {
                    // Do partition for aggregated stuff
                    Partition * p = new Partition();
                    for (QList<Event *>::Iterator saved_evt = aggregation->begin(); saved_evt != aggregation->end(); ++saved_evt)
                    {
                        p->addEvent(*saved_evt);
                        (*saved_evt)->partition = p;
                    }
                    partitions->append(p);

                    aggregating = false;
                    delete aggregation;
                }

                // Is this an event that should be added?
                else
                {
                    if ((*evt)->messages->size() > 0)
                        aggregation->prepend(*evt); // prepend since we're walking backwarsd
                }
            }
            else
            {
                // Is this an event that should cause aggregation?
                if ((*evt)->function == waitall_index)
                    aggregating = true;

                // Is this an event that should be added?
                if ((*evt)->messages->size() > 0)
                {
                    if (aggregating)
                    {
                        aggregation = new QList<Event *>();
                        aggregation->prepend(*evt); // prepend since we're walking backwards
                    }
                    else
                    {
                        Partition * p = new Partition();
                        p->addEvent(*evt);
                        (*evt)->partition = p;
                        partitions->append(p);
                    }
                }
            } // Not aggregating
        }
    }
}


