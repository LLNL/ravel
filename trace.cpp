#include "trace.h"
#include "exchangegnome.h"
#include <iostream>
#include <fstream>
#include <QElapsedTimer>
#include "general_util.h"
#include <cmath>

const QString Trace::collectives_string
    = QString("MPI_BarrierMPI_BcastMPI_ReduceMPI_GatherMPI_Scatter")
      + QString("MPI_AllgatherMPI_AllreduceMPI_AlltoallMPI_Scan")
      + QString("MPI_Reduce_scatterMPI_Op_createMPI_Op_freeMPIMPI_Alltoallv")
      + QString("MPI_AllgathervMPI_GathervMPI_Scatterv");

Trace::Trace(int np)
    : name(""),
      num_processes(np),
      units(-9),
      partitions(new QList<Partition *>()),
      metrics(new QList<QString>()),
      gnomes(new QList<Gnome *>()),
      options(NULL),
      functionGroups(new QMap<int, QString>()),
      functions(new QMap<int, Function *>()),
      communicators(NULL),
      collective_definitions(NULL),
      collectives(NULL),
      collectiveMap(NULL),
      events(new QVector<QVector<Event *> *>(np)),
      roots(new QVector<QVector<Event *> *>(np)),
      mpi_events(NULL),
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

    gnomes->append(new ExchangeGnome());
}

Trace::~Trace()
{
    delete metrics;
    delete functionGroups;

    for (QMap<int, Function *>::Iterator itr = functions->begin();
         itr != functions->end(); ++itr)
    {
        delete (itr.value());
        itr.value() = NULL;
    }
    delete functions;

    for (QList<Partition *>::Iterator itr = partitions->begin();
         itr != partitions->end(); ++itr)
    {
        //(*itr)->deleteEvents(); // This conflicts with the next deletion
        delete *itr;
        *itr = NULL;
    }
    delete partitions;

    for (QVector<QVector<Event *> *>::Iterator eitr = events->begin();
         eitr != events->end(); ++eitr)
    {
        for (QVector<Event *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete events;

    // Don't need to delete Events, they were deleted above
    for (QVector<QVector<Event *> *>::Iterator eitr = roots->begin();
         eitr != roots->end(); ++eitr)
    {
        delete *eitr;
        *eitr = NULL;
    }
    delete roots;

    for (QList<Gnome *>::Iterator gnome = gnomes->begin();
         gnome != gnomes->end(); ++gnome)
    {
        delete *gnome;
        *gnome = NULL;
    }
    delete gnomes;

    for (QMap<int, Communicator *>::Iterator comm = communicators->begin();
         comm != communicators->end(); ++comm)
    {
        delete *comm;
        *comm = NULL;
    }
    delete communicators;

    for (QMap<int, OTFCollective *>::Iterator cdef = collective_definitions->begin();
         cdef != collective_definitions->end(); ++cdef)
    {
        delete *cdef;
        *cdef = NULL;
    }
    delete collective_definitions;

    for (QMap<unsigned long long, CollectiveRecord *>::Iterator coll = collectives->begin();
         coll != collectives->end(); ++coll)
    {
        delete *coll;
        *coll = NULL;
    }
    delete collectives;

    delete collectiveMap;
}

void Trace::printStats()
{
    if (!isProcessed)
    {
        std::cout << "Not yet processed, incomplete stats" << std::endl;
        return;
    }

    int num_mpi = 0;
    for (QVector<QList<Event *> *>::Iterator itr = mpi_events->begin();
         itr != mpi_events->end(); ++itr)
    {
        num_mpi += (*itr)->length();
    }

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
    if (options.globalMerge)
        mergeGlobalSteps();
    emit(startClustering());
    std::cout << "Gnomifying..." << std::endl;
    if (options.cluster)
        gnomify();

    qSort(partitions->begin(), partitions->end(),
          dereferencedLessThan<Partition>);
    addPartitionMetric(); // For debugging

    isProcessed = true;

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Structure Extraction: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;
}

// Check every gnome in our set for matching and set which gnome as a metric
// There is probably a more efficient way to do this but it can be
// easily parallelized by partition
void Trace::gnomify()
{
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();
    metrics->append("Gnome");
    Gnome * gnome;
    float stepPortion = 100.0 / global_max_step;
    int total = 0;
    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        (*part)->makeClusterVectors("Lateness");
        int num_steps = (*part)->max_global_step - (*part)->min_global_step;
        for (int i = 0; i < gnomes->size(); i++)
        {
            gnome = gnomes->at(i);
            if (gnome->detectGnome(*part))
            {
                (*part)->gnome_type = i;
                (*part)->gnome = gnome->create();
                (*part)->gnome->setPartition(*part);
                (*part)->gnome->setFunctions(functions);
                setGnomeMetric(*part, i);
                (*part)->gnome->preprocess();
                break;
            }
        }
        if ((*part)->gnome == NULL)
        {
            (*part)->gnome_type = -1;
            (*part)->gnome = new Gnome();
            (*part)->gnome->setPartition(*part);
            (*part)->gnome->setFunctions(functions);
            setGnomeMetric(*part, -1);
            (*part)->gnome->preprocess();
        }
        total += stepPortion * num_steps;
        emit(updateClustering(total));
    }

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Gnomification/Clustering: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;
}

void Trace::setGnomeMetric(Partition * part, int gnome_index)
{
    for (QMap<int, QList<Event *> *>::Iterator event_list
         = part->events->begin();
         event_list != part->events->end(); ++event_list)
    {
        for (QList<Event *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            (*evt)->addMetric("Gnome", gnome_index, gnome_index);
        }
    }
}

// For debugging, so we know what partition we're in
void Trace::addPartitionMetric()
{
    metrics->append("Partition");
    long long partition = 0;
    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list
             = (*part)->events->begin();
             event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                (*evt)->addMetric("Partition", partition, partition);
            }
        }
        partition++;
    }
}


void Trace::partition()
{
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    chainCommEvents(); // Set comm_prev/comm_next and cc_prev/cc_next

    partitions = new QList<Partition *>();
    dag_entries = new QList<Partition *>();
    dag_step_dict = new QMap<int, QSet<Partition *> *>();

    // Partition - default
    if (!options.partitionByFunction)
    {
        traceTimer.start();
        // Partition by Process w or w/o Waitall
        std::cout << "Initializing partitios..." << std::endl;
        if (options.waitallMerge)
            initializePartitionsWaitall();
        else
            initializePartitions();

        // Merge collectives should go here
        std::cout << "Merging collectives..." << std::endl;
        mergeCollectives();

        traceElapsed = traceTimer.nsecsElapsed();
        std::cout << "Partition initialization: ";
        gu_printTime(traceElapsed);
        std::cout << std::endl;

        traceTimer.start();
          // Merge communication
        std::cout << "Merging for messages..." << std::endl;
        mergeForMessages();
        traceElapsed = traceTimer.nsecsElapsed();
        std::cout << "Message Merge: ";
        gu_printTime(traceElapsed);
        std::cout << std::endl;

          // Tarjan
        std::cout << "Merging cycles..." << std::endl;
        traceTimer.start();
        mergeCycles();
        traceElapsed = traceTimer.nsecsElapsed();
        std::cout << "Cycle Merge: ";
        gu_printTime(traceElapsed);
        std::cout << std::endl;

          // Merge by rank level [ later ]
        if (options.leapMerge)
        {
            std::cout << "Merging to complete leaps..." << std::endl;
            traceTimer.start();
            set_dag_steps();
            mergeByLeap();
            traceElapsed = traceTimer.nsecsElapsed();
            std::cout << "Leap Merge: ";
            gu_printTime(traceElapsed);
            std::cout << std::endl;

        }
    }

    // Partition - given
    // Form partitions given in some way -- need to write options for this [ later ]
    else
    {
        std::cout << "Partitioning by phase..." << std::endl;
        partitionByPhase();
        set_partition_dag();
    }
}

void Trace::set_global_steps()
{
    QSet<Partition *> * current_leap = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin();
         part != dag_entries->end(); ++part)
    {
        current_leap->insert(*part);
    }

    int accumulated_step;
    global_max_step = 0;

    while (!current_leap->isEmpty())
    {
        QSet<Partition *> * next_leap = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator part = current_leap->begin();
             part != current_leap->end(); ++part)
        {
            accumulated_step = 0;
            bool allParents = true;
            if ((*part)->max_global_step >= 0) // We already handled this parent
                continue;
            for (QSet<Partition *>::Iterator parent = (*part)->parents->begin();
                 parent != (*part)->parents->end(); ++parent)
            {

                // Check all parents to make sure they have all been handled
                if ((*parent)->max_global_step < 0)
                {
                    // So next time we must handle parent first
                    next_leap->insert(*parent);
                    allParents = false;
                }
                // Find maximum step of all predecessors
                // We +2 because individual steps start at 0, so when we add 0,
                // we want it to be offset from the parent
                accumulated_step = std::max(accumulated_step,
                                            (*parent)->max_global_step + 2);
            }

            // Skip since all parents haven't been handled
            if (!allParents)
                continue;

            // Set steps for the partition
            (*part)->max_global_step = 2 * ((*part)->max_step)
                                       + accumulated_step;
            (*part)->min_global_step = accumulated_step;
            (*part)->mark = false; // Using this to debug again

            // Set steps for partition events
            for (QMap<int, QList<Event *> *>::Iterator event_list
                 = (*part)->events->begin();
                 event_list != (*part)->events->end(); ++event_list)
            {
                for (QList<Event *>::Iterator evt
                     = (event_list.value())->begin();
                     evt != (event_list.value())->end(); ++evt)
                {
                    (*evt)->step *= 2;
                    (*evt)->step += accumulated_step;
                }
            }

            // Add children for handling
            for (QSet<Partition *>::Iterator child
                 = (*part)->children->begin();
                 child != (*part)->children->end(); ++child)
            {
                next_leap->insert(*child);
            }

            // Keep track of global max step
            global_max_step = std::max(global_max_step,
                                       (*part)->max_global_step);
        }

        delete current_leap;
        current_leap = next_leap;
    }
    delete current_leap;
}

// This actually calculates differential metric_name based on existing
// metric base_name (e.g. D. Lateness and Lateness)
void Trace::calculate_differential_lateness(QString metric_name,
                                            QString base_name)
{
    metrics->append(metric_name);

    long long int max_parent;
    long long int max_agg_parent;
    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list
             = (*part)->events->begin();
             event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<Event *>::Iterator evt
                 = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                max_parent = (*evt)->getMetric(base_name, true);
                max_agg_parent = 0;
                if ((*evt)->comm_prev)
                    max_agg_parent = ((*evt)->comm_prev->getMetric(base_name));
                for (QVector<Message *>::Iterator msg
                     = (*evt)->messages->begin();
                     msg != (*evt)->messages->end(); ++msg)
                {
                    if ((*msg)->receiver == *evt
                        && (*msg)->sender->getMetric(base_name) > max_parent)
                        max_parent = (*msg)->sender->getMetric(base_name);
                }

                (*evt)->addMetric(metric_name,
                                  std::max(0LL,
                                           (*evt)->getMetric(base_name)
                                            - max_parent),
                                  std::max(0LL,
                                           (*evt)->getMetric(base_name, true)
                                            - max_agg_parent));
            }
        }
    }

}

// Calculates lateness per partition rather than global step
void Trace::calculate_partition_lateness()
{

    QString p_late = "P. Lateness";
    metrics->append(p_late);

    unsigned long long int mintime, aggmintime;


    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        for (int i = (*part)->min_global_step;
             i <= (*part)->max_global_step; i += 2)
        {
            QList<Event *> * i_list = new QList<Event *>();

            for (QMap<int, QList<Event *> *>::Iterator event_list
                 = (*part)->events->begin();
                 event_list != (*part)->events->end(); ++event_list)
            {
                for (QList<Event *>::Iterator evt
                     = (event_list.value())->begin();
                     evt != (event_list.value())->end(); ++evt)
                {
                    if ((*evt)->step == i)
                        i_list->append(*evt);
                }
            }

            // Find min leave time
            mintime = ULLONG_MAX;
            aggmintime = ULLONG_MAX;
            for (QList<Event *>::Iterator evt = i_list->begin();
                 evt != i_list->end(); ++evt)
            {
                if ((*evt)->exit < mintime)
                    mintime = (*evt)->exit;
                if ((*evt)->enter < aggmintime)
                    aggmintime = (*evt)->enter;
            }

            // Set lateness;
            for (QList<Event *>::Iterator evt = i_list->begin();
                 evt != i_list->end(); ++evt)
            {
                (*evt)->addMetric(p_late, (*evt)->exit - mintime,
                                  (*evt)->enter - aggmintime);
            }
            delete i_list;
        }
    }

}

// Calculates lateness per global step
void Trace::calculate_lateness()
{
    metrics->append("Lateness");

    // Go through dag, starting at the beginning
    QSet<Partition *> * active_partitions = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin();
         part != dag_entries->end(); ++part)
    {
        active_partitions->insert(*part);
    }

    unsigned long long int mintime, aggmintime;

    // For each step, find all the events in the active partitions with that
    // step and calculate lateness
    // Active partition group may change every time the step is changed.
    int progressPortion = std::max(round(global_max_step / 2.0
                                         / lateness_portion),
                                   1.0);
    int currentPortion = 0;
    int currentIter = 0;

    for (int i = 0; i <= global_max_step; i+= 2)
    {
        if (round(currentIter / 1.0 / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(updatePreprocess(steps_portion + partition_portion
                                  + currentPortion,
                                  "Calculating Lateness..."));
        }
        ++currentIter;

        QList<Event *> * i_list = new QList<Event *>();
        for (QSet<Partition *>::Iterator part = active_partitions->begin();
             part != active_partitions->end(); ++part)
        {
            for (QMap<int, QList<Event *> *>::Iterator event_list
                 = (*part)->events->begin();
                 event_list != (*part)->events->end(); ++event_list)
            {
                for (QList<Event *>::Iterator evt
                     = (event_list.value())->begin();
                     evt != (event_list.value())->end(); ++evt)
                {
                    if ((*evt)->step == i)
                        i_list->append(*evt);
                }
            }
        }

        // Find min leave time
        mintime = ULLONG_MAX;
        aggmintime = ULLONG_MAX;
        for (QList<Event *>::Iterator evt = i_list->begin();
             evt != i_list->end(); ++evt)
        {
            if ((*evt)->exit < mintime)
                mintime = (*evt)->exit;
            if ((*evt)->enter < aggmintime)
                aggmintime = (*evt)->enter;
        }

        // Set lateness
        for (QList<Event *>::Iterator evt = i_list->begin();
             evt != i_list->end(); ++evt)
        {
            (*evt)->addMetric("Lateness", (*evt)->exit - mintime,
                              (*evt)->enter - aggmintime);
        }
        delete i_list;

        // Prepare active step for the next round
        QList<Partition *> * toRemove = new QList<Partition *>();
        QSet<Partition *> * toAdd = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator part = active_partitions->begin();
             part != active_partitions->end(); ++part)
        {
            if ((*part)->max_global_step == i)
            {
                toRemove->append(*part); // Stage for removal
                for (QSet<Partition *>::Iterator child
                     = (*part)->children->begin();
                     child != (*part)->children->end(); ++child)
                {
                    // Only insert if we're the max parent,
                    // otherwise wait for max parent
                    if ((*child)->min_global_step == i + 2)
                    {
                        toAdd->insert(*child);
                    }
                }
            }
        }
        // Remove those staged
        for (QList<Partition *>::Iterator oldpart = toRemove->begin();
             oldpart != toRemove->end(); ++oldpart)
        {
            active_partitions->remove(*oldpart);
        }
        delete toRemove;

        // Add those staged
        for (QSet<Partition *>::Iterator newpart = toAdd->begin();
             newpart != toAdd->end(); ++newpart)
        {
            active_partitions->insert(*newpart);
        }
        delete toAdd;
    }
    delete active_partitions;
}

// Iterates through all partitions and sets the steps
void Trace::assignSteps()
{
    // Step
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();
    std::cout << "Assigning local steps" << std::endl;
    int progressPortion = std::max(round(partitions->size() / 1.0
                                         / steps_portion),
                                   1.0);
    int currentPortion = 0;
    int currentIter = 0;
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        if (round(currentIter / 1.0 / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(updatePreprocess(partition_portion + currentPortion,
                                  "Assigning steps..."));
        }
        ++currentIter;
        (*partition)->step();
    }
    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Local Stepping: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;

    std::cout << "Setting global steps..." << std::endl;
    traceTimer.start();

    set_global_steps();

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Global Stepping: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;
    std::cout << "Num partitions " << partitions->length() << std::endl;


    // Calculate Step metrics
    std::cout << "Calculating lateness..." << std::endl;
    traceTimer.start();

    calculate_lateness();
    calculate_differential_lateness("D. Lateness", "Lateness");
    calculate_partition_lateness();
    calculate_differential_lateness("DP Lateness", "P. Lateness");

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Lateness Calculation: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;
}


// Create connectors of prev/next between send/recv events.
void Trace::chainCommEvents()
{
    // Note that mpi_events go backwards in time, so as we iterate through,
    // the event we just processed is the next event of the one we are
    // processing.
    for (QVector<QList<Event *> *>::Iterator event_list = mpi_events->begin();
         event_list != mpi_events->end(); ++event_list)
    {
        Event * next = NULL;
        Event * ccnext = NULL;
        for (QList<Event *>::Iterator evt = (*event_list)->begin();
             evt != (*event_list)->end(); ++evt)
        {
            if ((*evt)->messages->size() > 0 || (*evt)->collective)
            {
                (*evt)->comm_next = next;
                (*evt)->cc_next = ccnext;
                if (next)
                    next->comm_prev = (*evt);
                if (ccnext)
                    ccnext->cc_prev = (*evt);
                next = (*evt);
                ccnext = (*evt);
            }
            else if (collectives_string.contains(functions->value((*evt)->function)->name))
            {
                (*evt)->cc_next = ccnext;
                if (ccnext)
                    ccnext->cc_prev = (*evt);
                ccnext = (*evt);
            }
        }
    }
}

// This is the most difficult to understand part of the algorithm and the code.
// At least that's consistent!
void Trace::mergeByLeap()
{
    int leap = 0;
    bool blockMergeAll = false;
    QSet<Partition *> * new_partitions = new QSet<Partition *>();
    QSet<Partition *> * current_leap = new QSet<Partition *>();
    QSet<QSet<Partition *> *> * toDelete = new QSet<QSet<Partition *> *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin();
         part != dag_entries->end(); ++part)
    {
        current_leap->insert(*part);
    }
    while (!current_leap->isEmpty())
    {
        QSet<int> processes = QSet<int>();
        QSet<Partition *> * next_leap = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator partition = current_leap->begin();
             partition != current_leap->end(); ++partition)
        {
            processes += QSet<int>::fromList((*partition)->events->keys());
        }


        // If this leap doesn't have all the processes we have to do something
        if (processes.size() < num_processes)
        {
            blockMergeAll = false;
            QSet<Partition *> * new_leap_parts = new QSet<Partition *>();
            QSet<int> added_processes = QSet<int>();
            bool back_merge = false;
            for (QSet<Partition *>::Iterator partition = current_leap->begin();
                 partition != current_leap->end(); ++partition)
            {
                (*partition)->setMergables(options.leapCollective);
                // Determine distances from parent and child partitions
                // that are 1 leap away
                unsigned long long int parent_distance = ULLONG_MAX;
                unsigned long long int child_distance = ULLONG_MAX;
                // If we're doing leapCollectives and we might not be able to
                // merge all parents, do not do parent_distance
                // calculation because we absolutely will not back merge.
                if (!options.leapCollective
                    || ((*partition)->parents->size()
                        - (*(*partition)->mergable_parents).size() == 0
                       && (*partition)->parents->size() != 0))
                {
                    for (QSet<Partition *>::Iterator parent
                         = (*partition)->parents->begin();
                         parent != (*partition)->parents->end(); ++parent)
                    {
                        if ((*parent)->dag_leap == (*partition)->dag_leap - 1)
                            parent_distance = std::min(parent_distance,
                                                       (*partition)->distance(*parent));
                    }
                }
                for (QSet<Partition *>::Iterator child
                     = (*partition)->children->begin();
                     child != (*partition)->children->end(); ++child)
                {
                    // Recalculate, maybe have changed in previous iteration
                    (*child)->calculate_dag_leap();
                    if ((*child)->dag_leap == (*partition)->dag_leap + 1)
                        child_distance = std::min(child_distance,
                                                  (*partition)->distance(*child));
                }

                // If we are sufficiently close to the parent, back merge
                if (child_distance > 10 * parent_distance)
                {
                    for (QSet<Partition *>::Iterator parent
                         = (*partition)->parents->begin();
                         parent != (*partition)->parents->end(); ++parent)
                    {
                        if ((*parent)->dag_leap == (*partition)->dag_leap - 1)
                        {
                            (*partition)->group->unite(*((*parent)->group));
                            for (QSet<Partition *>::Iterator group_member
                                 = (*partition)->group->begin();
                                 group_member != (*partition)->group->end();
                                 ++group_member)
                            {
                                if (*group_member != *partition)
                                {
                                    toDelete->insert((*group_member)->group);
                                    (*group_member)->group = (*partition)->group;
                                }
                            }
                            back_merge = true;
                        }
                    }
                }
                else // merge children in
                {
                    if (options.leapCollective
                        && (*partition)->children->size()
                            - (*(*partition)->mergable_children).size() > 0)
                    {
                        blockMergeAll = true;
                    }
                    for (QSet<Partition *>::Iterator child
                         = (*partition)->mergable_children->begin();
                         child != (*partition)->mergable_children->end(); ++child)
                    {
                        if ((*child)->dag_leap == (*partition)->dag_leap + 1
                             && ((QSet<int>::fromList((*child)->events->keys())
                                  - processes)).size() > 0)
                        {
                            added_processes += (QSet<int>::fromList((*child)->events->keys())
                                                                     - processes);
                            (*partition)->group->unite(*((*child)->group));
                            for (QSet<Partition *>::Iterator group_member
                                 = (*partition)->group->begin();
                                 group_member != (*partition)->group->end();
                                 ++group_member)
                            {
                                if (*group_member != *partition)
                                {
                                    toDelete->insert((*group_member)->group);
                                    (*group_member)->group = (*partition)->group;
                                }
                            }
                        }
                    }
                }
                // Groups created now
            }

            if (!back_merge && added_processes.size() <= 0)
            {
                // Skip leap if we didn't add anything
                if (options.leapSkip || blockMergeAll)
                {
                    for (QSet<Partition *>::Iterator partition
                         = current_leap->begin(); partition
                         != current_leap->end(); ++partition)
                    {
                        new_partitions->insert(*partition);
                        for (QSet<Partition *>::Iterator child
                             = (*partition)->children->begin();
                             child != (*partition)->children->end(); ++child)
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
                    for (QSet<Partition *>::Iterator partition
                         = current_leap->begin();
                         partition != current_leap->end(); ++partition)
                    {
                        for (QSet<Partition *>::Iterator child
                             = (*partition)->children->begin();
                             child != (*partition)->children->end(); ++child)
                        {
                            if ((*child)->dag_leap == leap + 1)
                            {
                                (*partition)->group->unite(*((*child)->group));
                                for (QSet<Partition *>::Iterator group_member
                                     = (*partition)->group->begin();
                                     group_member != (*partition)->group->end();
                                     ++group_member)
                                {
                                    if (*group_member != *partition)
                                    {
                                        toDelete->insert((*group_member)->group);
                                        (*group_member)->group = (*partition)->group;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // Handled no merger case

            // Now do the merger - we go through the leap and look at each
            // partition's group and mark anything we've already merged.
            for (QSet<Partition *>::Iterator group = current_leap->begin();
                 group != current_leap->end(); ++group)
            {
                if ((*group)->leapmark) // We've already merged this
                    continue;

                Partition * p = new Partition();
                int min_leap = leap;

                for (QSet<Partition *>::Iterator partition
                     = (*group)->group->begin();
                     partition != (*group)->group->end(); ++partition)
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
                    for (QSet<Partition *>::Iterator child
                         = (*partition)->children->begin();
                         child != (*partition)->children->end(); ++child)
                    {
                        if (!((*group)->group->contains(*child)))
                        {
                            p->children->insert(*child);
                            for (QSet<Partition *>::Iterator group_member
                                 = (*group)->group->begin();
                                 group_member != (*group)->group->end();
                                 ++group_member)
                            {
                                if ((*child)->parents->contains(*group_member))
                                    (*child)->parents->remove(*group_member);
                            }
                            (*child)->parents->insert(p);
                        }
                    }

                    for (QSet<Partition *>::Iterator parent
                         = (*partition)->parents->begin();
                         parent != (*partition)->parents->end(); ++parent)
                    {
                        if (!((*group)->group->contains(*parent)))
                        {
                            p->parents->insert(*parent);
                            for (QSet<Partition *>::Iterator group_member
                                 = (*group)->group->begin();
                                 group_member != (*group)->group->end();
                                 ++group_member)
                            {
                                if ((*parent)->children->contains(*group_member))
                                    (*parent)->children->remove(*group_member);
                            }
                            (*parent)->children->insert(p);
                        }
                    }

                    // Update new partitions
                    if (new_partitions->contains(*partition))
                        new_partitions->remove(*partition);
                    if (new_leap_parts->contains(*partition))
                        new_leap_parts->remove(*partition);
                }

                for (QSet<Partition *>::Iterator group_member
                     = (*group)->group->begin();
                     group_member != (*group)->group->end(); ++group_member)
                {
                    (*group_member)->leapmark = true;
                }

                p->sortEvents();
                new_partitions->insert(p);
                new_leap_parts->insert(p);
            }

            // Don't advance leap until if there are new_leap_parts
            // that are at this leap (that therefore must be considered again)
            for (QSet<Partition *>::Iterator partition
                 = new_leap_parts->begin();
                 partition != new_leap_parts->end(); ++partition)
            {
                if ((*partition)->dag_leap == leap)
                    next_leap->insert(*partition);
                else if ((*partition)->dag_leap < leap)
                {
                    for (QSet<Partition *>::Iterator child
                         = (*partition)->children->begin();
                         child != (*partition)->children->end(); ++child)
                    {
                        (*child)->calculate_dag_leap();
                        if ((*child)->dag_leap == leap)
                            next_leap->insert(*child);
                    }
                }
            }

            // If we've added nothing, move forward to children
            if (next_leap->size() < 1)
            {
                for (QSet<Partition *>::Iterator partition
                     = new_leap_parts->begin();
                     partition != new_leap_parts->end(); ++partition)
                {
                    for (QSet<Partition *>::Iterator child
                         = (*partition)->children->begin();
                         child != (*partition)->children->end(); ++child)
                    {
                        (*child)->calculate_dag_leap();
                        if ((*child)->dag_leap == leap + 1)
                            next_leap->insert(*child);
                    }
                }
                ++leap;
            }

            delete current_leap;
            current_leap = next_leap;

            delete new_leap_parts;
        } // End If Processes Incomplete
        else
        {
            for (QSet<Partition *>::Iterator partition
                 = current_leap->begin();
                 partition != current_leap->end(); ++partition)
            {
                new_partitions->insert(*partition);
                for (QSet<Partition *>::Iterator child
                     = (*partition)->children->begin();
                     child != (*partition)->children->end(); ++child)
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

    // Delete all the old partition groups
    for (QSet<QSet<Partition *> *>::Iterator group = toDelete->begin();
         group != toDelete->end(); ++group)
    {
        delete *group;
    }
    delete toDelete;

    // Delete all old partitions
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        if (!(new_partitions->contains(*partition)))
            delete *partition;
    }
    delete partitions;
    partitions = new QList<Partition *>();

    // Need to calculate new dag_entries
    delete dag_entries;
    dag_entries = new QList<Partition *>();
    for (QSet<Partition *>::Iterator partition = new_partitions->begin();
         partition != new_partitions->end(); ++partition)
    {
        partitions->append(*partition);
        if ((*partition)->parents->size() <= 0)
            dag_entries->append(*partition);

        // Update event's reference just in case
        for (QMap<int, QList<Event *> *>::Iterator event_list
             = (*partition)->events->begin();
             event_list != (*partition)->events->end(); ++event_list)
        {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                (*evt)->partition = *partition;
            }
        }
    }
    set_dag_steps();

    delete new_partitions;
}


// Any partition with overlapping steps will get merged
void Trace::mergeGlobalSteps()
{
    std::cout << "Merging global steps..." << std::endl;
    int spanMin = 0;
    int spanMax = 0;
    QSet<Partition *> * new_partitions = new QSet<Partition *>();
    QSet<Partition *> * working_set = new QSet<Partition *>();
    QSet<Partition *> * toAdd = new QSet<Partition *>();

    // Initialize to dag entries
    for (QList<Partition *>::Iterator part = dag_entries->begin();
         part != dag_entries->end(); ++part)
    {
        working_set->insert(*part);
        if ((*part)->max_global_step > spanMax)
            spanMax = (*part)->max_global_step;
    }

    while (!working_set->isEmpty())
    {
        //TODO: Fix bug when partitions have imperfect overlap
        // Find all the overlapping partitions
        bool addFlag = true;
        while (addFlag)
        {
            toAdd->clear();
            addFlag = false;
            for (QSet<Partition *>::Iterator partition = working_set->begin();
                 partition != working_set->end(); ++partition)
            {
                for (QSet<Partition *>::Iterator child
                     = (*partition)->children->begin();
                     child != (*partition)->children->end(); ++child)
                {
                    if ((*child)->min_global_step <= spanMax) {
                        toAdd->insert(*child);
                        if ((*child)->max_global_step > spanMax)
                            spanMax = (*child)->max_global_step;
                        addFlag = true;
                    }
                }
            }

            if (addFlag)
                *working_set += *toAdd;
        }

        int childMin = INT_MAX;
        Partition * p = new Partition();
        for (QSet<Partition *>::Iterator partition = working_set->begin();
             partition != working_set->end(); ++partition)
        {
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

            // Update parents/children links
            for (QSet<Partition *>::Iterator child
                 = (*partition)->children->begin();
                 child != (*partition)->children->end(); ++child)
            {
                if (!(working_set->contains(*child)))
                {
                    p->children->insert(*child);
                    for (QSet<Partition *>::Iterator group_member
                         = working_set->begin();
                         group_member != working_set->end(); ++group_member)
                    {
                        if ((*child)->parents->contains(*group_member))
                            (*child)->parents->remove(*group_member);
                    }
                    (*child)->parents->insert(p);
                    if ((*child)->min_global_step < childMin)
                        childMin = (*child)->min_global_step;
                }
            }

            for (QSet<Partition *>::Iterator parent
                 = (*partition)->parents->begin();
                 parent != (*partition)->parents->end(); ++parent)
            {
                if (!(working_set->contains(*parent)))
                {
                    p->parents->insert(*parent);
                    for (QSet<Partition *>::Iterator group_member
                         = working_set->begin();
                         group_member != working_set->end(); ++group_member)
                    {
                        if ((*parent)->children->contains(*group_member))
                            (*parent)->children->remove(*group_member);
                    }
                    (*parent)->children->insert(p);
                }
            }

            // Update new partitions
            if (new_partitions->contains(*partition))
                new_partitions->remove(*partition);

        }
        p->sortEvents();
        p->min_global_step = spanMin;
        p->max_global_step = spanMax;
        new_partitions->insert(p);

        // Start the next working_set
        working_set->clear();
        for (QSet<Partition *>::Iterator child = p->children->begin();
             child != p->children->end(); ++child)
        {
            if ((*child)->min_global_step == childMin)
            {
                working_set->insert(*child);
                if ((*child)->max_global_step > spanMax)
                    spanMax = (*child)->max_global_step;
            }
        }
        spanMin = childMin;

    } // End Working Set While
    delete working_set;
    delete toAdd;

    // Delete all old partitions
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        if (!(new_partitions->contains(*partition)))
            delete *partition;
    }
    delete partitions;
    partitions = new QList<Partition *>();

    // Need to calculate new dag_entries
    delete dag_entries;
    dag_entries = new QList<Partition *>();
    for (QSet<Partition *>::Iterator partition = new_partitions->begin();
         partition != new_partitions->end(); ++partition)
    {
        partitions->append(*partition);
        if ((*partition)->parents->size() <= 0)
            dag_entries->append(*partition);

        // Update event's reference just in case
        for (QMap<int, QList<Event *> *>::Iterator event_list
             = (*partition)->events->begin();
             event_list != (*partition)->events->end(); ++event_list)
        {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                (*evt)->partition = *partition;
            }
        }
    }
    set_dag_steps();

    delete new_partitions;
}


// Merges partitions that are connected by a message
// We go through all send events and merge them with all recvs
// connected to the event (there should only be 1) but in the
// ISend future...
void Trace::mergeForMessagesHelper(Partition * part,
                                   QSet<Partition *> * to_merge,
                                   QQueue<Partition *> * to_process)
{
    for(QMap<int, QList<Event *> *>::Iterator event_list
        = part->events->begin();
        event_list != part->events->end(); ++event_list)
    {
        for (QList<Event *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            // If Event is collective, merge the collective
            if ((*evt)->collective && !(*evt)->collective->mark)
            {
                for (QList<Event *>::Iterator ev2
                     = (*evt)->collective->events->begin();
                     ev2 != (*evt)->collective->events->end(); ++ev2)
                {
                    to_merge->insert((*ev2)->partition);
                    if (!(*ev2)->partition->mark
                            && !to_process->contains((*ev2)->partition))
                        to_process->enqueue((*ev2)->partition);
                }

                // Mark so we don't have to do the above again
                (*evt)->collective->mark = true;
            }
            else // We have messages, merge the messages
            {
                for (QVector<Message *>::Iterator msg = (*evt)->messages->begin();
                     msg != (*evt)->messages->end(); ++msg)
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

    }
    part->mark = true;
}


// Loop through the partitions and merge all connected by messages.
void Trace::mergeForMessages()
{
    int progressPortion = std::max(round(partitions->size() / 1.0 / 35),1.0);
    int currentPortion = 0;
    int currentIter = 0;

    Partition * p = NULL;

    // Loop through partitions to determine which to merge based on messages
    QSet<Partition *> * to_merge = new QSet<Partition *>();
    QQueue<Partition *> * to_process = new QQueue<Partition *>();
    QList<QList<Partition *> *> * components = new QList<QList<Partition *> *>();
    for(QList<Partition *>::Iterator part = partitions->begin();
        part != partitions->end(); ++ part)
    {
        if (round(currentIter / 1.0 / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(updatePreprocess(5 + currentPortion,
                                  "Merging for messages..."));
        }
        ++currentIter;
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
        for (QSet<Partition *>::Iterator mpart = to_merge->begin();
             mpart != to_merge->end(); ++mpart)
        {
            component->append(*mpart);
        }
        components->append(component);
    }

    // Merge the partition groups discovered
    mergePartitions(components);
    delete to_merge;
    delete to_process;
}

// merge events of the same collective into a single partition
// Assumption: this partition will be one event wide. I don't think I use
// this assumption explicitly yet though.
void Trace::mergeCollectives()
{
    return;
    for (QMap<unsigned long long, CollectiveRecord *>::Iterator cr
         = collectives->begin();
         cr != collectives->end(); ++cr)
    {
        // Create new partition for this collective

        for (QList<Event *>::Iterator evt = (*cr)->events->begin();
             evt != (*cr)->events->end(); ++evt)
        {
            // Merge in partition of this event

            // Update parent/child relationships for event

            // Set up this event's partition for deletion
        }

        // Delete old partitions
    }

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
int Trace::strong_connect_iter(Partition * partition,
                               QStack<Partition *> * stack,
                               QList<QList<Partition *> *> * components,
                               int index)
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
            for (QSet<Partition *>::Iterator child
                 = ri->part->children->begin();
                 child != ri->part->children->end(); ++child)
            {
                ri->children->append(*child);
            }

            strong_connect_loop(ri->part, stack, ri->children, 0,
                                recurse, components);
        }
        else
        {
            ri->part->lowlink = std::min(ri->part->lowlink,
                                         ri->child->lowlink);
            strong_connect_loop(ri->part, stack, ri->children, ++(ri->cIndex),
                                recurse, components);
        }
    }

    for (QSet<QList<Partition *> *>::Iterator riC = riChildrenTracker->begin();
         riC != riChildrenTracker->end(); ++riC)
    {
        delete *riC;
    }
    for (QSet<RecurseInfo *>::Iterator ri = riTracker->begin();
         ri != riTracker->end(); ++ri)
    {
        delete *ri;
    }
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
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        (*partition)->tindex = -1;
        (*partition)->lowlink = -1;
    }

    int index = 0;
    QStack<Partition *> * stack = new QStack<Partition *>();
    QList<QList<Partition *> *> * components = new QList<QList<Partition *> *>();
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        if ((*partition)->tindex < 0)
            index = strong_connect_iter((*partition), stack, components, index);
    }

    delete stack;
    return components;
}


// Goes through current partitions and merges cycles
void Trace::mergeCycles()
{
    // Determine partition parents/children through dag
    // and then determine strongly connected components (SCCs) with tarjan.
    emit(updatePreprocess(41, "Merging cycles..."));
    set_partition_dag();

    QList<QList<Partition *> *> * components = tarjan();
    emit(updatePreprocess(43, "Merging cycles..."));

    mergePartitions(components);
    emit(updatePreprocess(45, "Merging cycles..."));
}


// Input: A list of partition lists. Each partition list should be merged
// into a single partition. This updates parent/child relationships so
// there is no need to reset the dag.
void Trace::mergePartitions(QList<QList<Partition *> *> * components) {
    // Go through the SCCs and merge them into single partitions
    QList<Partition *> * merged = new QList<Partition *>();
    for (QList<QList<Partition *> *>::Iterator component = components->begin();
         component != components->end(); ++component)
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
        for (QList<Partition *>::Iterator partition = (*component)->begin();
             partition != (*component)->end(); ++partition)
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

            // Set old_children and old_parents from the children and parents
            // of the partition to merge
            for (QSet<Partition *>::Iterator child
                 = (*partition)->children->begin();
                 child != (*partition)->children->end(); ++child)
            {
                // but only if parent/child not already in SCC
                if (!((*component)->contains(*child)))
                    p->old_children->insert(*child);
            }
            for (QSet<Partition *>::Iterator parent
                 = (*partition)->parents->begin();
                 parent != (*partition)->parents->end(); ++parent)
            {
                if (!((*component)->contains(*parent)))
                    p->old_parents->insert(*parent);
            }
        }

        merged->append(p);
    }

    // Now that we have all the merged partitions, figure out parents/children
    // between them, and sort the event lists and such
    // Note we could just set the event partition adn then use
    // set_partition_dag... in theory
    bool parent_flag;
    dag_entries->clear();
    for (QList<Partition *>::Iterator partition = merged->begin();
         partition != merged->end(); ++partition)
    {
        parent_flag = false;

        // Update parents/children by taking taking the old parents/children
        // and the new partition they belong to
        for (QSet<Partition *>::Iterator child
             = (*partition)->old_children->begin();
             child != (*partition)->old_children->end(); ++child)
        {
            if ((*child)->new_partition)
                (*partition)->children->insert((*child)->new_partition);
            else
                std::cout << "Error, no new partition set on child" << std::endl;
        }
        for (QSet<Partition *>::Iterator parent
             = (*partition)->old_parents->begin();
             parent != (*partition)->old_parents->end(); ++parent)
        {
            if ((*parent)->new_partition)
            {
                (*partition)->parents->insert((*parent)->new_partition);
                parent_flag = true;
            }
            else
                std::cout << "Error, no new partition set on parent" << std::endl;
        }

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
        for (QMap<int, QList<Event *> *>::Iterator event_list
             = (*partition)->events->begin();
             event_list != (*partition)->events->end(); ++event_list)
        {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                (*evt)->partition = (*partition);
            }
        }
    }

    delete partitions;
    partitions = merged;

    // Clean up by deleting all of the old partitions through the components
    for (QList<QList<Partition *> *>::Iterator citr = components->begin();
         citr != components->end(); ++citr)
    {
        for (QList<Partition *>::Iterator itr = (*citr)->begin();
             itr != (*citr)->end(); ++itr)
        {
            // Don't delete the singleton SCCs as we keep those
            if ((*itr)->new_partition != (*itr))
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
    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        (*part)->new_partition = NULL;
    }
}


// Set all the parents/children in the partition by checking where first and
// last events in each process pointer.
void Trace::set_partition_dag()
{
    dag_entries->clear();
    bool parent_flag;
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        parent_flag = false;
        for (QMap<int, QList<Event *> *>::Iterator event_list
             = (*partition)->events->begin();
             event_list != (*partition)->events->end(); ++event_list)
        {
            Q_ASSERT(event_list.value());
            Q_ASSERT(event_list.value()->first());
            // Note we insert parents and a children to ourselves and insert
            // ourselves to our parents and children. In many cases this will
            // duplicate the insert, but merging by Waitall can create
            // situations where our first event is preceded by a partition but
            // its last event points elsewhere. By connecting here, these
            // cycles will be found later on in merge cycles.
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

// In other words, assign to each partitions its leap value
void Trace::set_dag_steps()
{
    // Clear current dag steps
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        (*partition)->dag_leap = -1;
    }

    dag_step_dict->clear();
    QSet<Partition *> * current_level = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin();
         part != dag_entries->end(); ++part)
    {
        current_level->insert(*part);
    }
    int accumulated_leap;
    bool allParentsFlag;
    while (!current_level->isEmpty())
    {
        QSet<Partition *> * next_level = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator partition = current_level->begin();
             partition != current_level->end(); ++partition)
        {
            accumulated_leap = 0;
            allParentsFlag = true;
            if ((*partition)->dag_leap >= 0) // Already handled
                continue;

            // Deal with parents. If there are any unhandled, we set them to be
            // dealt with next and mark allParentsFlag false so we can put off
            // this one.
            for (QSet<Partition *>::Iterator parent
                 = (*partition)->parents->begin();
                 parent != (*partition)->parents->end(); ++parent)
            {
                if ((*parent)->dag_leap < 0)
                {
                    next_level->insert(*parent);
                    allParentsFlag = false;
                }
                accumulated_leap = std::max(accumulated_leap,
                                            (*parent)->dag_leap + 1);
            }

            // Still need to handle parents
            if (!allParentsFlag)
                continue;

            // All parents were handled, so we can set our steps
            (*partition)->dag_leap = accumulated_leap;
            if (!dag_step_dict->contains((*partition)->dag_leap))
                (*dag_step_dict)[(*partition)->dag_leap] = new QSet<Partition *>();
            ((*dag_step_dict)[(*partition)->dag_leap])->insert(*partition);

            for (QSet<Partition *>::Iterator child
                 = (*partition)->children->begin();
                 child != (*partition)->children->end(); ++child)
            {
                next_level->insert(*child);
            }
        }
        delete current_level;
        current_level = next_level;
    }
    delete current_level;
}

// Note, phases were set on importing from OTF so here we just have to
// group by phase but alter the phases so that ordering constraints
// are still met.
void Trace::partitionByPhase()
{
    QMap<int, Partition *> * partition_dict = new QMap<int, Partition *>();
    for (QVector<QList<Event *> *>::Iterator event_list = mpi_events->begin();
         event_list != mpi_events->end(); ++event_list)
    {
        // The mpi_events list was set in reverse chronological order while
        // the phase consistency check we do goes in forward chronological
        // order, so we iterate through the list backwards.
        for (int i = (*event_list)->size() - 1; i >= 0; i--)
        {
            Event * evt = (*event_list)->at(i);
            if ((evt)->messages->size() > 0)
            {
                if ((evt)->comm_prev
                    && (evt)->comm_prev->phase > (evt)->phase)
                {
                    (evt)->phase = (evt)->comm_prev->phase;
                }
                for (QVector<Message *>::Iterator msg
                     = (evt)->messages->begin();
                     msg != (evt)->messages->end(); ++msg)
                {
                    if (!msg)
                        continue;
                    if ((*msg)->sender->phase > (evt)->phase)
                        (evt)->phase = (*msg)->sender->phase;
                    else if ((*msg)->sender->phase < (evt)->phase)
                        (*msg)->sender->phase = (evt)->phase;
                    if ((*msg)->receiver->phase > (evt)->phase)
                        (evt)->phase = (*msg)->receiver->phase;
                    else if ((*msg)->receiver->phase < (evt)->phase)
                        (*msg)->receiver->phase = (evt)->phase;
                }
                if (!partition_dict->contains((evt)->phase))
                    (*partition_dict)[(evt)->phase] = new Partition();
                ((*partition_dict)[(evt)->phase])->addEvent(evt);
                (evt)->partition = (*partition_dict)[(evt)->phase];
            }
        }
    }

    for (QMap<int, Partition *>::Iterator partition
         = partition_dict->begin();
         partition != partition_dict->end(); ++partition)
    {
        (*partition)->sortEvents();
        partitions->append(*partition);
    }

    delete partition_dict;
}

// Every send/recv event becomes its own partition
void Trace::initializePartitions()
{
    int progressPortion = std::max(round(num_processes / 1.0 / 5), 1.0);
    int currentPortion = 0;
    int currentIter = 0;
    for (QVector<QList<Event *> *>::Iterator event_list = mpi_events->begin();
         event_list != mpi_events->end(); ++event_list)
    {
        if (round(currentIter / 1.0 / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(updatePreprocess(currentPortion,
                                  "Initializing partitions..."));
        }
        ++currentIter;
        for (QList<Event *>::Iterator evt = (*event_list)->begin();
             evt != (*event_list)->end(); ++evt)
        {
            // Every event with messages becomes its own partition
            if ((*evt)->messages->size() > 0 || (*evt)->collective)
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
    int testall_index = -1;
    for (QMap<int, Function * >::Iterator function = functions->begin();
         function != functions->end(); ++function)
    {
        if (function.value()->group == mpi_group)
        {
            if ( collectives_string.contains(function.value()->name) )
            {
                (*collective_ids)[collective_index] = function.key();
                ++collective_index;
            }
            if (function.value()->name == "MPI_Waitall")
                waitall_index = function.key();
            if (function.value()->name == "MPI_Testall")
                testall_index = function.key();
        }
    }

    int progressPortion = std::max(round(num_processes / 1.0 / 5), 1.0);
    int currentPortion = 0;
    int currentIter = 0;

    bool aggregating;
    QList<Event *> * aggregation;
    for (QVector<QList<Event *> *>::Iterator event_list = mpi_events->begin();
         event_list != mpi_events->end(); ++event_list)
    {
        if (round(currentIter / 1.0 / progressPortion) > currentPortion)
        {
            ++currentPortion;
            emit(updatePreprocess(currentPortion, "Initializing partitions..."));
        }
        ++currentIter;
        // We note these should be in reverse order because of the way
        // they were added
        aggregating = false;
        for (QList<Event *>::Iterator evt = (*event_list)->begin();
             evt != (*event_list)->end(); ++evt)
        {
            if (aggregating)
            {
                // Is this an event that can stop aggregating?
                // 1. Due to a recv (including waitall recv)
                if ((*evt)->messages->size() > 0 || (*evt)->collective)
                {
                    bool recv_found = false;
                    for (QVector<Message *>::Iterator msg
                         = (*evt)->messages->begin();
                         msg != (*evt)->messages->end(); ++msg)
                    {
                        if ((*msg)->receiver == *evt) // Receive found
                        {
                            recv_found = true;
                            break;
                        }
                    }

                    if (recv_found)
                    {
                        // Do partition for aggregated stuff
                        if (aggregation->size() > 0)
                        {
                            Partition * p = new Partition();
                            for (QList<Event *>::Iterator saved_evt
                                 = aggregation->begin();
                                 saved_evt != aggregation->end();
                                 ++saved_evt)
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
                else if ((*evt)->function == waitall_index
                         || (*evt)->function == testall_index
                         || collective_ids->contains((*evt)->function))
                {
                    // Do partition for aggregated stuff
                    if (aggregation->size() > 0)
                    {
                        Partition * p = new Partition();
                        for (QList<Event *>::Iterator saved_evt
                             = aggregation->begin();
                             saved_evt != aggregation->end(); ++saved_evt)
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
                if ((*evt)->function == waitall_index
                    || (*evt)->function == testall_index)
                {
                    aggregating = true;
                    aggregation = new QList<Event *>();
                }

                // Is this an event that should be added?
                if ((*evt)->messages->size() > 0 || (*evt)->collective)
                {

                    if (aggregating)
                        // prepend since we're walking backwards
                        aggregation->prepend(*evt);
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
                for (QList<Event *>::Iterator saved_evt = aggregation->begin();
                     saved_evt != aggregation->end(); ++saved_evt)
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

// In progress: will be used for extra information on aggregate events
QList<Trace::FunctionPair> Trace::getAggregateFunctions(Event * evt)
{
    unsigned long long int stoptime = evt->enter;
    unsigned long long int starttime = 0;
    if (evt->comm_prev)
        starttime = evt->comm_prev->exit;

    int process = evt->process;
    QVector<Event *> * proots = roots->at(process);
    QMap<int, FunctionPair> * fpMap = new QMap<int, FunctionPair>();

    for (QVector<Event *>::Iterator root = proots->begin();
         root != proots->end(); ++root)
    {
        getAggregateFunctionRecurse(*root, fpMap, starttime, stoptime);
    }

    QList<FunctionPair> fpList = fpMap->values();
    delete fpMap;
    return fpList;
}

long long int Trace::getAggregateFunctionRecurse(Event * evt,
                                                 QMap<int, FunctionPair> * fpMap,
                                                 unsigned long long start,
                                                 unsigned long long stop)
{
    if (evt->enter > stop || evt->exit < start)
        return 0;

    unsigned long long overlap_stop = std::min(stop, evt->exit);
    unsigned long long overlap_start = std::max(start, evt->enter);
    long long overlap = overlap_stop - overlap_start;
    long long child_overlap;
    for (QVector<Event *>::Iterator child = evt->callees->begin();
         child != evt->callees->end(); ++child)
    {
        child_overlap = getAggregateFunctionRecurse(*child, fpMap,
                                                    start, stop);
        overlap -= child_overlap;
    }

    if (fpMap->contains(evt->function))
    {
        FunctionPair oldfp = fpMap->value(evt->function);
        (*fpMap)[evt->function] = FunctionPair(evt->function,
                                               oldfp.time + overlap);
    }
    else
    {
        (*fpMap)[evt->function] = FunctionPair(evt->function, overlap);
    }
    return overlap;
}

// use GraphViz to see partition graph for debugging
void Trace::output_graph(QString filename, bool byparent)
{
    std::ofstream graph;
    graph.open(filename.toStdString().c_str());

    QString indent = "     ";

    graph << "digraph {\n";
    graph << indent.toStdString().c_str() << "graph [bgcolor=transparent];\n";
    graph << indent.toStdString().c_str() << "node [label=\"\\N\"];\n";

    int id = 0;
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
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
        for (QList<Partition *>::Iterator partition = partitions->begin();
             partition != partitions->end(); ++partition)
        {
            for (QSet<Partition *>::Iterator parent
                 = (*partition)->parents->begin();
                 parent != (*partition)->parents->end(); ++parent)
            {
                graph << indent.toStdString().c_str() << (*parent)->gvid.toStdString().c_str();
                graph << " -> " << (*partition)->gvid.toStdString().c_str() << ";\n";
            }
        }
    else
        for (QList<Partition *>::Iterator partition = partitions->begin();
             partition != partitions->end(); ++partition)
        {
            for (QSet<Partition *>::Iterator child
                 = (*partition)->children->begin();
                 child != (*partition)->children->end(); ++child)
            {
                graph << indent.toStdString().c_str() << (*partition)->gvid.toStdString().c_str();
                graph << " -> " << (*child)->gvid.toStdString().c_str() << ";\n";
            }
        }
    graph << "}";

    graph.close();
}
