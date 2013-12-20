#include "trace.h"
#include <iostream>
#include <fstream>
#include <QElapsedTimer>
#include "general_util.h"

Trace::Trace(int np, bool legacy)
    : num_processes(np),
      units(-9),
      partitions(new QList<Partition *>()),
      isLegacy(legacy),
      options(NULL),
      functionGroups(new QMap<int, QString>()),
      functions(new QMap<int, Function *>()),
      events(new QVector<QVector<Event *> *>(np)),
      roots(new QVector<QVector<Event *> *>(np)),
      mpi_events(NULL),
      send_events(new QList<Event *>()),
      mpi_group(-1),
      global_max_step(-1),
      dag_entries(NULL),
      dag_step_dict(NULL),
      isProcessed(false),
      riTracker(NULL),
      riChildrenTracker(NULL)
{
    for (int i = 0; i < np; i++) {
        (*events)[i] = new QVector<Event *>();
        (*roots)[i] = new QVector<Event *>();
    }
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

void Trace::printStats()
{
    if (!isProcessed)
    {
        std::cout << "Not yet processed, incomplete stats" << std::endl;
        return;
    }

    std::cout << "# Messages: " << send_events->length() << std::endl;

    int num_mpi = 0;
    for (QVector<QList<Event *> *>::Iterator itr = mpi_events->begin(); itr != mpi_events->end(); ++itr)
        num_mpi += (*itr)->length();

    std::cout << "# MPI Events: " << num_mpi << std::endl;

}

void Trace::preprocess(OTFImportOptions * _options)
{
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    options = *_options;
    partition();
    assignSteps();

    isProcessed = true;

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Structure Extraction: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;
}

void Trace::partition()
{

    chainCommEvents(); // Set comm_prev/comm_next

    partitions = new QList<Partition *>();
    dag_entries = new QList<Partition *>();
    dag_step_dict = new QMap<int, QSet<Partition *> *>();

    // Partition - default
    if (!options.partitionByFunction)
    {
          // Partition by Process w or w/o Waitall
        std::cout << "Initializing partitios..." << std::endl;
        if (options.waitallMerge)
            initializePartitionsWaitall();
        else
            initializePartitions();

          // Merge communication
        //set_partition_dag();
        //output_graph("/home/kate/pre_msgpartition.dot");
        std::cout << "Merging for messages..." << std::endl;
        mergeForMessages();

        /*
        for (QList<Partition *>::Iterator part = partitions->begin(); part != partitions->end(); ++part)
        {
            std::cout << "New Partition:" << std::endl;
            for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list)
            {
                std::cout << "  evt process:" << event_list.key() << std::endl;
                for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
                {
                    std::cout << "    evt time: " << (*evt)->enter << " - " << (*evt)->exit << std::endl;
                }
            }
        }*/

          // Tarjan
        std::cout << "Merging cycles..." << std::endl;
        set_partition_dag();
        //output_graph("/home/kate/pre_cyclepartition.dot");
        //output_graph("/home/kate/pre_cyclepartitionparent.dot", true);
        mergeCycles();

          // Merge by rank level [ later ]
        if (options.leapMerge)
        {
            set_dag_steps();
            mergeByLeap();
        }
    }

    // Partition - given
      // Form partitions given in some way -- need to write options for this [ later ]
    else
    {
        partitionByPhase();
    }

    //std::cout << "Setting partition dag for the first time..." << std::endl;
    //set_partition_dag();
    //output_graph("/Users/kate/post_partition.dot");
}

void Trace::set_global_steps()
{
    QSet<Partition *> * current_leap = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin(); part != dag_entries->end(); ++part)
        current_leap->insert(*part);

    int accumulated_step;
    global_max_step = 0;

    while (!current_leap->isEmpty())
    {
        //std::cout << " **** NEW LEAP **** " << std::endl;
        QSet<Partition *> * next_leap = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator part = current_leap->begin(); part != current_leap->end(); ++part)
        {
            //std::cout << "Active partition: " << (*part)->generate_process_string().toStdString().c_str() << ", " << (*part)->gvid.toStdString().c_str() << std::endl;
            accumulated_step = 0;
            bool allParents = true;
            if ((*part)->max_global_step >= 0) // We already handled this parent
                continue;
            for (QSet<Partition *>::Iterator parent = (*part)->parents->begin(); parent != (*part)->parents->end(); ++parent)
            {

                // Check all parents to make sure they have all been handled
                if ((*parent)->max_global_step < 0)
                {
                    next_leap->insert(*parent); // So next time we must handle parent first
                    allParents = false;
                    //std::cout << "     All parents not finished..."  << (*parent)->generate_process_string().toStdString().c_str() << ", " << (*parent)->gvid.toStdString().c_str() << std::endl;
                }
                // Find maximum step of all predecessors
                // We +1 because individual steps start at 0, so when we add 0, we want
                // it to be offset from teh parent
                accumulated_step = std::max(accumulated_step, (*parent)->max_global_step + 2);
            }

            // Skip since all parents haven't been handled
            if (!allParents)
                continue;

            // Set steps for the partition
            (*part)->max_global_step = 2 * ((*part)->max_step) + accumulated_step;
            (*part)->min_global_step = accumulated_step;
            (*part)->mark = false; // Using this to debug again

            // Set steps for partition events
            for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list)
                for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
                {
                    (*evt)->step *= 2;
                    (*evt)->step += accumulated_step;
                }

            // Add children for handling
            for (QSet<Partition *>::Iterator child = (*part)->children->begin(); child != (*part)->children->end(); ++child)
            {
                next_leap->insert(*child);
                //std::cout << "     Adding..."  << (*child)->generate_process_string().toStdString().c_str() << std::endl;
            }

            // Keep track of global max step
            global_max_step = std::max(global_max_step, (*part)->max_global_step);
        }

        delete current_leap;
        current_leap = next_leap;
    }
    delete current_leap;
}

void Trace::calculate_lateness()
{
    // Go through dag, starting at the beginning
    QSet<Partition *> * active_partitions = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin(); part != dag_entries->end(); ++part)
        active_partitions->insert(*part);

    unsigned long long int mintime, aggmintime;

    // For each step, find all the events in the active partitions with that step
    // and calculate lateness
    // Active partition group may change every time the step is changed.
    for (int i = 0; i <= global_max_step; i+= 2)
    {
        //std::cout << "Handling step " << i << std::endl;
        QList<Event *> * i_list = new QList<Event *>();
        for (QSet<Partition *>::Iterator part = active_partitions->begin(); part != active_partitions->end(); ++part)
        {
            //std::cout << "     Active partition: " << (*part)->generate_process_string().toStdString().c_str() << " : ";
            //std::cout << (*part)->min_global_step << " - " << (*part)->max_global_step << std::endl;
            for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list)
            {
                //std::cout << "       p = " << event_list.key() << std::endl;
                for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
                {
                    if ((*evt)->step == i)
                        i_list->append(*evt);
                    //std::cout << "          Event p = " << (*evt)->process << ", s = " << (*evt)->step << std::endl;
                }
            }
        }

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
        //std::cout << "*****STEP***** " << i << std::endl;
        for (QList<Event *>::Iterator evt = i_list->begin(); evt != i_list->end(); ++evt)
        {
            (*evt)->addMetric("Lateness", (*evt)->exit - mintime, (*evt)->enter - aggmintime);
            //std::cout << "Adding lateness to process " << (*evt)->process << std::endl;
        }
        delete i_list;
        //std::cout << "*****END*****" << std::endl;

        // Prepare active step for the next round
        QList<Partition *> * toRemove = new QList<Partition *>();
        QSet<Partition *> * toAdd = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator part = active_partitions->begin(); part != active_partitions->end(); ++part)
        {
            //std::cout << "Considering " << (*part)->generate_process_string().toStdString().c_str() << " : ";
            //std::cout << (*part)->min_global_step << " - " << (*part)->max_global_step << std::endl;
            if ((*part)->max_global_step == i)
            {
                //std::cout << "Remove!" << std::endl;
                toRemove->append(*part); // Stage for removal
                for (QSet<Partition *>::Iterator child = (*part)->children->begin(); child != (*part)->children->end(); ++child)
                {
                    if ((*child)->min_global_step == i + 2) {// Only insert if we're the max parent, otherwise wait for max parent
                        toAdd->insert(*child);
                    /*    std::cout << " Partition " << (*part)->generate_process_string().toStdString().c_str() << " : ";
                        std::cout << (*part)->min_global_step << " - " << (*part)->max_global_step;
                        std::cout << " Adds " << (*child)->generate_process_string().toStdString().c_str() << " : ";
                        std::cout << (*child)->min_global_step << " - " << (*child)->max_global_step << std::endl;
                    } else {
                        std::cout << " Partition " << (*part)->generate_process_string().toStdString().c_str() << " : ";
                        std::cout << (*part)->min_global_step << " - " << (*part)->max_global_step;
                        std::cout << " ***SKIPS*** " << (*child)->generate_process_string().toStdString().c_str() << " : ";
                        std::cout << (*child)->min_global_step << " - " << (*child)->max_global_step << std::endl;
                    */}
                }
            }
        }
        // Remove those staged
        for (QList<Partition *>::Iterator oldpart = toRemove->begin(); oldpart != toRemove->end(); ++oldpart)
            active_partitions->remove(*oldpart);
        delete toRemove;

        // Add those staged
        for (QSet<Partition *>::Iterator newpart = toAdd->begin(); newpart != toAdd->end(); ++newpart)
            active_partitions->insert(*newpart);
        delete toAdd;
    }
    delete active_partitions;
}

void Trace::assignSteps()
{
    // Step
    //int count = 0;
    for (QList<Partition *>::Iterator partition = partitions->begin(); partition != partitions->end(); ++partition)
    {
        //std::cout << "Stepping partition " << count << std::endl;
        (*partition)->step();
        //count++;
    }

    /*
    for (QList<Partition *>::Iterator part = partitions->begin(); part != partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
            {
                std::cout << "partition: " << (*evt)->partition->gvid.toStdString().c_str();
                std::cout << ", event step: " << (*evt)->step << " in process " << (*evt)->process;
                std::cout << ", time: " << (*evt)->enter << " - " << (*evt)->exit << std::endl;
            }
        }
    }*/

    // Global steps
    //std::cout << "Resetting partition dag..." << std::endl;
    //set_partition_dag();
    //output_graph("/home/kate/post_global.dot");
    //output_graph("/home/kate/post_globalparent.dot",true);
    // Don't really need to set dag steps here though it shouldn't take forever I don't get it
    //std::cout << "Setting dag steps..." << std::endl;
    //set_dag_steps();
    std::cout << "Setting global steps..." << std::endl;
    set_global_steps();

    //output_graph("/home/kate/post_global.dot");

    // Calculate Step metrics
    std::cout << "Calculating lateness..." << std::endl;
    calculate_lateness();

    // Verify
    /*bool flag = true;
    for (QList<Partition *>::Iterator part = partitions->begin(); part != partitions->end(); ++part)
    {
        Q_ASSERT((*part)->min_global_step >= 0);
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
            {
                if (!(*evt)->metrics->contains("Lateness"))
                {
                    //std::cout << "Bad partition: " << (*evt)->partition->gvid.toStdString().c_str();
                    //std::cout << ", event step: " << (*evt)->step << " in process " << (*evt)->process << std::endl;
                    flag = false;
                }
                //Q_ASSERT((*evt)->metrics->contains("Lateness"));
            }
        }
    }
    Q_ASSERT(flag);*/
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
/*
Partition * Trace::mergePartitions(Partition * p1, Partition * p2)
{
    Partition * p = new Partition();

    // Copy events
    Q_ASSERT(p1 != NULL);
    Q_ASSERT(p2 != NULL);
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
    p1->new_partition = p;
    p2->new_partition = p;
    p->new_partition = p;

    // Sort events
    p->sortEvents();

    return p;
}
*/

void Trace::mergeByLeap()
{
    int leap = 0;
    QSet<Partition *> * new_partitions = new QSet<Partition *>();
    QSet<Partition *> * current_leap = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin(); part != dag_entries->end(); ++part)
        current_leap->insert(*part);
    while (!current_leap->isEmpty())
    {
        QSet<int> processes = QSet<int>();
        QSet<Partition *> * next_leap = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator partition = current_leap->begin(); partition != current_leap->end(); ++partition)
            processes += QSet<int>::fromList((*partition)->events->keys());

        // If this leap doesn't have all the processes we have to do something
        if (processes.size() < num_processes)
        {
            QSet<Partition *> * new_leap_parts = new QSet<Partition *>();
            QSet<int> added_processes = QSet<int>();
            bool back_merge = false;
            for (QSet<Partition *>::Iterator partition = current_leap->begin(); partition != current_leap->end(); ++partition)
            {
                // Determine distances from parent and child partitions 1 leap away
                unsigned long long int parent_distance = ULLONG_MAX;
                unsigned long long int child_distance = ULLONG_MAX;
                for (QSet<Partition *>::Iterator parent = (*partition)->parents->begin(); parent != (*partition)->parents->end(); ++parent)
                    if ((*parent)->dag_leap == (*partition)->dag_leap - 1)
                        parent_distance = std::min(parent_distance, (*partition)->distance(*parent));
                for (QSet<Partition *>::Iterator child = (*partition)->children->begin(); child != (*partition)->children->end(); ++child)
                    if ((*child)->dag_leap == (*partition)->dag_leap + 1)
                    {
                        (*child)->calculate_dag_leap();
                        child_distance = std::min(child_distance, (*partition)->distance(*child));
                    }

                // If we are sufficiently close to the parent, back merge
                if (child_distance > 10 * parent_distance)
                {
                    for (QSet<Partition *>::Iterator parent = (*partition)->parents->begin(); parent != (*partition)->parents->end(); ++parent)
                        if ((*parent)->dag_leap == (*partition)->dag_leap - 1)
                        {
                            (*partition)->group->unite(*((*parent)->group));
                            for (QSet<Partition *>::Iterator group_member = (*partition)->group->begin(); group_member != (*partition)->group->end(); ++group_member)
                            {
                                if (*group_member != *partition)
                                {
                                    delete (*group_member)->group;
                                    (*group_member)->group = (*partition)->group;
                                }
                            }
                            back_merge = true;
                        }
                }
                else // merge children in
                {
                    for (QSet<Partition *>::Iterator child = (*partition)->children->begin(); child != (*partition)->children->end(); ++child)
                        if ((*child)->dag_leap == (*partition)->dag_leap + 1
                                && ((QSet<int>::fromList((*child)->events->keys()) - processes)).size() > 0)
                        {
                            added_processes += (QSet<int>::fromList((*child)->events->keys()) - processes);
                            (*partition)->group->unite(*((*child)->group));
                            for (QSet<Partition *>::Iterator group_member = (*partition)->group->begin(); group_member != (*partition)->group->end(); ++group_member)
                            {
                                if (*group_member != *partition)
                                {
                                    delete (*group_member)->group;
                                    (*group_member)->group = (*partition)->group;
                                }
                            }
                        }
                }
                // Groups created now
            }

            if (!back_merge && added_processes.size() <= 0)
                if (options.leapSkip) // Skip leap if we didn't add anything
                {
                    for (QSet<Partition *>::Iterator partition = current_leap->begin(); partition != current_leap->end(); ++partition)
                    {
                        new_partitions->insert(*partition);
                        for (QSet<Partition *>::Iterator child = (*partition)->children->begin(); child != (*partition)->children->end(); ++child)
                        {
                            (*child)->calculate_dag_leap();
                            if ((*child)->dag_leap == leap + 1)
                                next_leap->insert(*child);
                        }
                    }
                    ++leap;
                    delete current_leap;
                    current_leap = next_leap;
                    continue;
                }
                else // Merge everything if we didn't add anything
                {
                    for (QSet<Partition *>::Iterator partition = current_leap->begin(); partition != current_leap->end(); ++partition)
                        for (QSet<Partition *>::Iterator child = (*partition)->children->begin(); child != (*partition)->children->end(); ++child)
                            if ((*child)->dag_leap == leap + 1)
                            {
                                (*partition)->group->unite(*((*child)->group));
                                for (QSet<Partition *>::Iterator group_member = (*partition)->group->begin(); group_member != (*partition)->group->end(); ++group_member)
                                {
                                    if (*group_member != *partition)
                                    {
                                        delete (*group_member)->group;
                                        (*group_member)->group = (*partition)->group;
                                    }
                                }
                            }
                }
            // Handled no merger case

            // Now do the merger
            for (QSet<Partition *>::Iterator group = current_leap->begin(); group != current_leap->end(); ++group)
            {
                if ((*group)->leapmark) // We've already merged this
                    continue;

                Partition * p = new Partition();
                int min_leap = leap;

                for (QSet<Partition *>::Iterator partition = (*group)->group->begin(); partition != (*group)->group->end(); ++partition)
                {
                    min_leap = std::min((*partition)->dag_leap, min_leap);

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

                    p->dag_leap = min_leap;


                    // Update parents/children links
                    for (QSet<Partition *>::Iterator child = (*partition)->children->begin(); child != (*partition)->children->end(); ++child)
                        if (!((*group)->group->contains(*child)))
                        {
                            p->children->insert(*child);
                            for (QSet<Partition *>::Iterator group_member = (*group)->group->begin(); group_member != (*group)->group->end(); ++group_member)
                                if ((*child)->parents->contains(*group_member))
                                    (*child)->parents->remove(*group_member);
                            (*child)->parents->insert(p);
                        }

                    for (QSet<Partition *>::Iterator parent = (*partition)->parents->begin(); parent != (*partition)->parents->end(); ++parent)
                        if (!((*group)->group->contains(*parent)))
                        {
                            p->parents->insert(*parent);
                            for (QSet<Partition *>::Iterator group_member = (*group)->group->begin(); group_member != (*group)->group->end(); ++group_member)
                                if ((*parent)->children->contains(*group_member))
                                    (*parent)->children->remove(*group_member);
                            (*parent)->children->insert(p);
                        }

                    // Update new partitions
                    if (new_partitions->contains(*partition))
                        new_partitions->remove(*partition);
                    if (new_leap_parts->contains(*partition))
                        new_leap_parts->remove(*partition);
                }

                for (QSet<Partition *>::Iterator group_member = (*group)->group->begin(); group_member != (*group)->group->end(); ++group_member)
                    (*group_member)->leapmark = true;

                p->sortEvents();
                new_partitions->insert(p);
                new_leap_parts->insert(p);
            }

            if (next_leap->size() < 1)
                for (QSet<Partition *>::Iterator partition = new_leap_parts->begin(); partition != new_leap_parts->end(); ++partition)
                    for (QSet<Partition *>::Iterator child = (*partition)->children->begin(); child != (*partition)->children->end(); ++ child)
                    {
                        (*child)->calculate_dag_leap();
                        if ((*child)->dag_leap == leap + 1)
                            next_leap->insert(*child);
                    }
                ++leap;

            delete current_leap;
            current_leap = next_leap;

            delete new_leap_parts;
        } // End If Processes Incomplete
        else
        {
            for (QSet<Partition *>::Iterator partition = current_leap->begin(); partition != current_leap->end(); ++partition)
            {
                new_partitions->insert(*partition);
                for (QSet<Partition *>::Iterator child = (*partition)->children->begin(); child != (*partition)->children->end(); ++child)
                {
                    (*child)->calculate_dag_leap();
                    if ((*child)->dag_leap == leap + 1)
                        next_leap->insert(*child);
                }
            }
            ++leap;
            delete current_leap;
            current_leap = next_leap;
        }
    } // End Leap While
    delete current_leap;

    // Delete all old partitions
    for (QList<Partition *>::Iterator partition = partitions->begin(); partition != partitions->end(); ++partition)
        if (!(new_partitions->contains(*partition)))
            delete *partition;
    delete partitions;
    partitions = new QList<Partition *>();

    // Need to calculate new dag_entries
    delete dag_entries;
    dag_entries = new QList<Partition *>();
    for (QSet<Partition *>::Iterator partition = new_partitions->begin(); partition != new_partitions->end(); ++partition)
    {
        partitions->append(*partition);
        if ((*partition)->parents->size() <= 0)
            dag_entries->append(*partition);

        // Update event's reference just in case
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*partition)->events->begin(); event_list != (*partition)->events->end(); ++event_list) {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt) {
                (*evt)->partition = *partition;
            }
        }
    }
    set_dag_steps();

    delete new_partitions;
}

// Merges partitions that are connected by a message
// We go through all send events and merge them with all recvs
// connected to the event (there should only be 1)
void Trace::mergeForMessagesHelper(Partition * part, QSet<Partition *> * to_merge, QQueue<Partition *> * to_process)
{
    for(QMap<int, QList<Event *> *>::Iterator event_list = part->events->begin(); event_list != part->events->end(); ++event_list)
    {
        for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
        {
            for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
            {
                Partition * rpart = (*msg)->receiver->partition;
                Partition * spart = (*msg)->sender->partition;
                to_merge->insert(rpart);
                to_merge->insert(spart);
                if (!rpart->mark && !to_process->contains(rpart))
                    to_process->enqueue(rpart);
                if (!spart->mark && !to_process->contains(spart))
                    to_process->enqueue(spart);
            }
        }

    }
    part->mark = true;
}

void Trace::mergeForMessages()
{
    Partition * p;

    // Loop through partitions to determine which to merge based on messages
    QSet<Partition *> * to_merge = new QSet<Partition *>();
    QQueue<Partition *> * to_process = new QQueue<Partition *>();
    QList<QList<Partition *> *> * components = new QList<QList<Partition *> *>();
    for(QList<Partition *>::Iterator part = partitions->begin(); part != partitions->end(); ++ part)
    {
        if ((*part)->mark)
            continue;
        to_merge->clear();
        to_process->clear();
        mergeForMessagesHelper(*part, to_merge, to_process);
        while (!to_process->isEmpty())
        {
            p = to_process->dequeue();
            if (!p->mark)
                mergeForMessagesHelper(p, to_merge, to_process);
        }

        QList<Partition *> * component = new QList<Partition *>();
        for (QSet<Partition *>::Iterator mpart = to_merge->begin(); mpart != to_merge->end(); ++mpart)
            component->append(*mpart);
        components->append(component);
    }

    std::cout << "Now merging components..." << std::endl;
    mergePartitions(components);
    delete to_merge;
    delete to_process;
}

// Looping section of Tarjan algorithm
void Trace::strong_connect_loop(Partition * part, QStack<Partition *> * stack,
                                      QList<Partition *> * children, int cIndex,
                                      QStack<RecurseInfo *> * recurse,
                                      QList<QList<Partition *> *> * components)
{
    Q_ASSERT(cIndex >= 0);
    Q_ASSERT(children);
    while (cIndex < children->size())
    {
        Partition * child = (*children)[cIndex];
        Q_ASSERT(child);
        if (child->tindex < 0)
        {
            // Push onto recursion stack, return so we can deal with it
            recurse->push(new RecurseInfo(part, child, children, cIndex));
            riTracker->insert(recurse->top());
            recurse->push(new RecurseInfo(child, NULL, NULL, -1));
            riTracker->insert(recurse->top());
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
// Please redo with shared pointers
int Trace::strong_connect_iter(Partition * partition, QStack<Partition *> * stack,
                                      QList<QList<Partition *> *> * components, int index)
{
    QStack<RecurseInfo *> * recurse = new QStack<RecurseInfo *>();
    riTracker = new QSet<RecurseInfo *>();
    riChildrenTracker = new QSet<QList<Partition *> *>();
    recurse->push(new RecurseInfo(partition, NULL, NULL, -1));
    riTracker->insert(recurse->top());
    while (recurse->size() > 0)
    {
        RecurseInfo * ri = recurse->pop();

        if (ri->cIndex < 0)
        {
            ri->part->tindex = index;
            ri->part->lowlink = index;
            ++index;
            stack->push(ri->part);
            Q_ASSERT(ri->children == NULL);
            ri->children = new QList<Partition *>();
            riChildrenTracker->insert(ri->children);
            for (QSet<Partition *>::Iterator child = ri->part->children->begin(); child != ri->part->children->end(); ++child)
                ri->children->append(*child);

            strong_connect_loop(ri->part, stack, ri->children, 0, recurse, components);
        }
        else
        {
            ri->part->lowlink = std::min(ri->part->lowlink, ri->child->lowlink);
            strong_connect_loop(ri->part, stack, ri->children, ++(ri->cIndex), recurse, components);
        }

        //delete ri->children;
        //delete ri;
    }

    for (QSet<QList<Partition *> *>::Iterator riC = riChildrenTracker->begin(); riC != riChildrenTracker->end(); ++riC)
        delete *riC;
    for (QSet<RecurseInfo *>::Iterator ri = riTracker->begin(); ri != riTracker->end(); ++ri)
        delete *ri;
    delete riTracker;
    riTracker = NULL;
    delete riChildrenTracker;
    riChildrenTracker = NULL;
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
    //output_graph("/Users/kate/pre_merge_cycles.dot");
    QList<QList<Partition *> *> * components = tarjan();

    mergePartitions(components);
}

void Trace::mergePartitions(QList<QList<Partition *> *> * components) {
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
                if (!((*component)->contains(*child))) // but only if parent/child not already in SCC
                    p->old_children->insert(*child);
            for (QSet<Partition *>::Iterator parent = (*partition)->parents->begin(); parent != (*partition)->parents->end(); ++parent)
                if (!((*component)->contains(*parent)))
                    p->old_parents->insert(*parent);
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
        if ((*partition)->parents->size() > 0)
            std::cout << "Partition has parent already??" << std::endl;

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

    // Reset new_partition
    for (QList<Partition *>::Iterator part = partitions->begin(); part != partitions->end(); ++part)
        (*part)->new_partition = NULL;
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
            Q_ASSERT(event_list.value());
            Q_ASSERT(event_list.value()->first());
            if ((event_list.value())->first()->comm_prev)
            {
                (*partition)->parents->insert(event_list.value()->first()->comm_prev->partition);
                event_list.value()->first()->comm_prev->partition->children->insert(*partition);
                parent_flag = true;
            }
            if ((event_list.value())->last()->comm_next)
            {
                (*partition)->children->insert(event_list.value()->last()->comm_next->partition);
                event_list.value()->last()->comm_next->partition->parents->insert(*partition);
            }

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
                accumulated_leap = std::max(accumulated_leap, (*parent)->dag_leap + 1);
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

// Note, phases were set on importing from OTF
void Trace::partitionByPhase()
{
    QMap<int, Partition *> * partition_dict = new QMap<int, Partition *>();
    for (QVector<QList<Event *> *>::Iterator event_list = mpi_events->begin(); event_list != mpi_events->end(); ++event_list)
        for (QList<Event *>::Iterator evt = (*event_list)->begin(); evt != (*event_list)->end(); ++evt)
            if ((*evt)->messages->size() > 0)
            {
                if (!partition_dict->contains((*evt)->phase))
                    (*partition_dict)[(*evt)->phase] = new Partition();
                ((*partition_dict)[(*evt)->phase])->addEvent(*evt);
                (*evt)->partition = (*partition_dict)[(*evt)->phase];
            }

    for (QMap<int, Partition *>::Iterator partition = partition_dict->begin(); partition != partition_dict->end(); ++partition)
    {
        (*partition)->sortEvents();
        partitions->append(*partition);
    }

    delete partition_dict;
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
                p->new_partition = p;
                partitions->append(p);
            }
        }
    }
}

// Waitalls determine if send/recv events are grouped along a process
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
    for (QVector<QList<Event *> *>::Iterator event_list = mpi_events->begin(); event_list != mpi_events->end(); ++event_list)
    {
        // We note these should be in reverse order because of the way they were added
        aggregating = false;
        for (QList<Event *>::Iterator evt = (*event_list)->begin(); evt != (*event_list)->end(); ++evt)
        {
            if (aggregating)
            {
                // Is this an event that can stop aggregating?
                // 1. Due to a recv (including waitall recv)
                if ((*evt)->messages->size() > 0)
                {
                    bool recv_found = false;
                    for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
                        if ((*msg)->receiver == *evt) // Receive found
                        {
                            recv_found = true;
                            break;
                        }

                    if (recv_found)
                    {
                        // Do partition for aggregated stuff
                        if (aggregation->size() > 0)
                        {
                            Partition * p = new Partition();
                            for (QList<Event *>::Iterator saved_evt = aggregation->begin(); saved_evt != aggregation->end(); ++saved_evt)
                            {
                                p->addEvent(*saved_evt);
                                (*saved_evt)->partition = p;
                            }
                            p->new_partition = p;
                            partitions->append(p);
                        }

                        // Do partition for this recv
                        Partition * r = new Partition();
                        r->addEvent(*evt);
                        (*evt)->partition = r;
                        r->new_partition = r;
                        partitions->append(r);

                        aggregating = false;
                        delete aggregation;
                    }
                    else
                    {   // Must be a send only, prepend and continue on
                        aggregation->prepend(*evt);
                    }
                }

                // 2. Due to non-recv/non-comm waitall or collective
                else if ((*evt)->function == waitall_index || collective_ids->contains((*evt)->function))
                {
                    // Do partition for aggregated stuff
                    if (aggregation->size() > 0)
                    {
                        Partition * p = new Partition();
                        for (QList<Event *>::Iterator saved_evt = aggregation->begin(); saved_evt != aggregation->end(); ++saved_evt)
                        {
                            p->addEvent(*saved_evt);
                            (*saved_evt)->partition = p;
                        }
                        p->new_partition = p;
                        partitions->append(p);
                    }

                    aggregating = false;
                    delete aggregation;
                }
                // Neither a comm event or waitall/collective, skip this event.

            }
            else // We are not aggregating
            {
                // Is this an event that should cause aggregation?
                if ((*evt)->function == waitall_index)
                {
                    aggregating = true;
                    aggregation = new QList<Event *>();
                }

                // Is this an event that should be added?
                if ((*evt)->messages->size() > 0)
                {
                    if (aggregating)
                        aggregation->prepend(*evt); // prepend since we're walking backwards
                    else
                    {
                        Partition * p = new Partition();
                        p->addEvent(*evt);
                        (*evt)->partition = p;
                        p->new_partition = p;
                        partitions->append(p);
                    }
                }
            } // Not aggregating
        } // End this event

        // Check if we have any left over and aggregate them
        if (aggregating)
        {
            if (aggregation->size() > 0)
            {
                Partition * p = new Partition();
                for (QList<Event *>::Iterator saved_evt = aggregation->begin(); saved_evt != aggregation->end(); ++saved_evt)
                {
                    p->addEvent(*saved_evt);
                    (*saved_evt)->partition = p;
                }
                p->new_partition = p;
                partitions->append(p);
            }
            delete aggregation;
            aggregating = false;
        }
    } // End event loop for one process
}

void Trace::output_graph(QString filename, bool byparent)
{
    std::ofstream graph;
    graph.open(filename.toStdString().c_str());

    QString indent = "     ";

    graph << "digraph {\n";
    graph << indent.toStdString().c_str() << "graph [bgcolor=transparent];\n";
    graph << indent.toStdString().c_str() << "node [label=\"\\N\"];\n";

    int id = 0;
    for (QList<Partition *>::Iterator partition = partitions->begin(); partition != partitions->end(); ++partition)
    {
        (*partition)->gvid = QString::number(id);
        graph << indent.toStdString().c_str() << (*partition)->gvid.toStdString().c_str();
        graph << " [label=\"" << (*partition)->generate_process_string().toStdString().c_str();
        graph << " :  " << QString::number((*partition)->min_global_step).toStdString().c_str();
        graph << " - " << QString::number((*partition)->max_global_step).toStdString().c_str();
        graph << ", gv: " << (*partition)->gvid.toStdString().c_str();
        graph << ", ne: " << (*partition)->num_events();
        graph << "\"];\n";
        ++id;
    }

    if (byparent)
        for (QList<Partition *>::Iterator partition = partitions->begin(); partition != partitions->end(); ++partition)
        {
            for (QSet<Partition *>::Iterator parent = (*partition)->parents->begin(); parent != (*partition)->parents->end(); ++parent)
            {
                graph << indent.toStdString().c_str() << (*parent)->gvid.toStdString().c_str();
                graph << " -> " << (*partition)->gvid.toStdString().c_str() << ";\n";
            }
        }
    else
        for (QList<Partition *>::Iterator partition = partitions->begin(); partition != partitions->end(); ++partition)
        {
            for (QSet<Partition *>::Iterator child = (*partition)->children->begin(); child != (*partition)->children->end(); ++child)
            {
                graph << indent.toStdString().c_str() << (*partition)->gvid.toStdString().c_str();
                graph << " -> " << (*child)->gvid.toStdString().c_str() << ";\n";
            }
        }
    graph << "}";

    graph.close();
    //std::cout << "Finished graph" << std::endl;
}
