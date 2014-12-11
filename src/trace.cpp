#include "trace.h"

#include <iostream>
#include <fstream>
#include <QElapsedTimer>
#include <QTime>
#include <cmath>
#include <climits>
#include <cfloat>

#include "task.h"
#include "event.h"
#include "commevent.h"
#include "p2pevent.h"
#include "collectiveevent.h"
#include "function.h"
#include "rpartition.h"
#include "otfimportoptions.h"
#include "gnome.h"
#include "exchangegnome.h"
#include "taskgroup.h"
#include "otfcollective.h"
#include "general_util.h"

Trace::Trace(int nt)
    : name(""),
      num_tasks(nt),
      units(-9),
      partitions(new QList<Partition *>()),
      metrics(new QList<QString>()),
      metric_units(new QMap<QString, QString>()),
      gnomes(new QList<Gnome *>()),
      options(NULL),
      functionGroups(new QMap<int, QString>()),
      functions(new QMap<int, Function *>()),
      tasks(NULL),
      taskgroups(NULL),
      collective_definitions(NULL),
      collectives(NULL),
      collectiveMap(NULL),
      events(new QVector<QVector<Event *> *>(nt)),
      roots(new QVector<QVector<Event *> *>(nt)),
      mpi_group(-1),
      global_max_step(-1),
      dag_entries(new QList<Partition *>()),
      dag_step_dict(new QMap<int, QSet<Partition *> *>()),
      isProcessed(false)
{
    for (int i = 0; i < nt; i++) {
        (*events)[i] = new QVector<Event *>();
        (*roots)[i] = new QVector<Event *>();
    }

    gnomes->append(new ExchangeGnome());
}

Trace::~Trace()
{
    delete metrics;
    delete metric_units;
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

    for (QMap<int, TaskGroup *>::Iterator comm = taskgroups->begin();
         comm != taskgroups->end(); ++comm)
    {
        delete *comm;
        *comm = NULL;
    }
    delete taskgroups;

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

    delete dag_entries;
    delete dag_step_dict;


    for (QMap<int, Task *>::Iterator comm = tasks->begin();
         comm != tasks->end(); ++comm)
    {
        delete *comm;
        *comm = NULL;
    }
    delete taskgroups;
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

    if (!options.seedClusters)
    {
        QTime time = QTime::currentTime();
        qsrand((uint)time.msec());
        options.clusterSeed = qrand();
    }
    std::cout << "Clustering seed: " << options.clusterSeed << std::endl;

    metrics->append("Gnome");
    (*metric_units)["Gnome"] = "";
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
                (*part)->gnome->set_seed(options.clusterSeed);
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
            (*part)->gnome->set_seed(options.clusterSeed);
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
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list
         = part->events->begin();
         event_list != part->events->end(); ++event_list)
    {
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
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
    (*metric_units)["Partition"] = "";
    long long partition = 0;
    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list
             = (*part)->events->begin();
             event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
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

    // Partition - default - we assume base partitions done
    // at the converter stage, now we have to do a lot of merging
    if (!options.partitionByFunction)
    {
        traceTimer.start();
          // Merge communication
        std::cout << "Merging for messages..." << std::endl;
        mergeForMessages();
        traceElapsed = traceTimer.nsecsElapsed();
        std::cout << "Message Merge: ";
        gu_printTime(traceElapsed);
        std::cout << std::endl;
        std::cout << "Partitions = " << partitions->size() << std::endl;

          // Tarjan
        std::cout << "Merging cycles..." << std::endl;
        traceTimer.start();
        mergeCycles();
        traceElapsed = traceTimer.nsecsElapsed();
        std::cout << "Cycle Merge: ";
        gu_printTime(traceElapsed);
        std::cout << std::endl;
        std::cout << "Partitions = " << partitions->size() << std::endl;

        // Merge by call tree
      if (options.callerMerge)
      {
          std::cout << "Merging based on call tree..." << std::endl;
          traceTimer.start();
          set_dag_steps();
          mergeByCommonCaller();
          traceElapsed = traceTimer.nsecsElapsed();
          std::cout << "Caller Merge: ";
          gu_printTime(traceElapsed);
          std::cout << std::endl;

      }

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
    // Now handled in converter
    else
    {
        // std::cout << "Partitioning by phase..." << std::endl;
        //partitionByPhase();
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
            for (QMap<int, QList<CommEvent *> *>::Iterator event_list
                 = (*part)->events->begin();
                 event_list != (*part)->events->end(); ++event_list)
            {
                for (QList<CommEvent *>::Iterator evt
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
    (*metric_units)[metric_name] = metric_units->value(base_name);

    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list
             = (*part)->events->begin();
             event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<CommEvent *>::Iterator evt
                 = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                (*evt)->calculate_differential_metric(metric_name, base_name);
            }
        }
    }

}

// Calculates lateness per partition rather than global step
void Trace::calculate_partition_lateness()
{
    QList<QString> counterlist = QList<QString>();
    for (int i = 0; i < metrics->size(); i++)
        counterlist.append(metrics->at(i));

    QList<double> valueslist = QList<double>();

    QString p_late = "Lateness";
    metrics->append(p_late);
    (*metric_units)[p_late] = getUnits(units);


    for (int i = 0; i < counterlist.size(); i++)
    {
        metrics->append("Step " + counterlist[i]);
        metric_units->insert("Step " + counterlist[i], counterlist[i]);
        valueslist.append(0);
        valueslist.append(0);
    }

    unsigned long long int mintime, aggmintime;


    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        for (int i = (*part)->min_global_step;
             i <= (*part)->max_global_step; i += 2)
        {
            QList<CommEvent *> * i_list = new QList<CommEvent *>();

            for (QMap<int, QList<CommEvent *> *>::Iterator event_list
                 = (*part)->events->begin();
                 event_list != (*part)->events->end(); ++event_list)
            {
                for (QList<CommEvent *>::Iterator evt
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
            for (int j = 0; j < valueslist.size(); j++)
                valueslist[j] = DBL_MAX;
            for (QList<CommEvent *>::Iterator evt = i_list->begin();
                 evt != i_list->end(); ++evt)
            {
                if ((*evt)->exit < mintime)
                    mintime = (*evt)->exit;
                if ((*evt)->enter < aggmintime)
                    aggmintime = (*evt)->enter;
                for (int j = 0; j < counterlist.size(); j++)
                {
                    if ((*evt)->getMetric(counterlist[j]) < valueslist[2*j])
                        valueslist[2*j] = (*evt)->getMetric(counterlist[j]);
                    if ((*evt)->getMetric(counterlist[j],true) < valueslist[2*j+1])
                        valueslist[2*j+1] = (*evt)->getMetric(counterlist[j], true);
                }
            }

            // Set lateness;
            for (QList<CommEvent *>::Iterator evt = i_list->begin();
                 evt != i_list->end(); ++evt)
            {
                (*evt)->addMetric(p_late, (*evt)->exit - mintime,
                                  (*evt)->enter - aggmintime);
                double evt_time = (*evt)->exit - (*evt)->enter;
                double agg_time = (*evt)->enter;
                if ((*evt)->comm_prev)
                    agg_time = (*evt)->enter - (*evt)->comm_prev->exit;
                for (int j = 0; j < counterlist.size(); j++)
                {
                    (*evt)->addMetric("Step " + counterlist[j],
                                     (*evt)->getMetric(counterlist[j]) - valueslist[2*j],
                                     (*evt)->getMetric(counterlist[j], true) - valueslist[2*j+1]);
                    (*evt)->setMetric(counterlist[j],
                                      (*evt)->getMetric(counterlist[j]) / 1.0 / evt_time,
                                      (*evt)->getMetric(counterlist[j], true) / 1.0 / agg_time);
                }
            }
            delete i_list;
        }
    }

}

// Calculates lateness per global step
void Trace::calculate_lateness()
{
    metrics->append("G. Lateness");
    (*metric_units)["G. Lateness"] = getUnits(units);

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

        QList<CommEvent *> * i_list = new QList<CommEvent *>();
        for (QSet<Partition *>::Iterator part = active_partitions->begin();
             part != active_partitions->end(); ++part)
        {
            for (QMap<int, QList<CommEvent *> *>::Iterator event_list
                 = (*part)->events->begin();
                 event_list != (*part)->events->end(); ++event_list)
            {
                for (QList<CommEvent *>::Iterator evt
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
        for (QList<CommEvent *>::Iterator evt = i_list->begin();
             evt != i_list->end(); ++evt)
        {
            if ((*evt)->exit < mintime)
                mintime = (*evt)->exit;
            if ((*evt)->enter < aggmintime)
                aggmintime = (*evt)->enter;
        }

        // Set lateness
        for (QList<CommEvent *>::Iterator evt = i_list->begin();
             evt != i_list->end(); ++evt)
        {
            (*evt)->addMetric("G. Lateness", (*evt)->exit - mintime,
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
    if (options.advancedStepping)
    {
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
    } else {
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
            (*partition)->basic_step();
        }
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
    calculate_differential_lateness("D.G. Lateness", "G. Lateness");
    calculate_partition_lateness();
    calculate_differential_lateness("D. Lateness", "Lateness");

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Lateness Calculation: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;
}

// Adjacent (parent/child off by one dag leap) partitions are merged if along
// any task, they either share a least common caller or one's least common
// call is a child of the other's.
void Trace::mergeByCommonCaller()
{
    QSet<Partition *> * new_partitions = new QSet<Partition *>();
    QSet<Partition *> * current_partitions = new QSet<Partition *>();
    QSet<QSet<Partition *> *> * toDelete = new QSet<QSet<Partition *> *>();
    QSet<Partition *> near_children = QSet<Partition *>();
    QMap<Event *, int> * memo = new QMap<Event *, int>();

    // Let's start from the dag_entries, we'll remove from the current
    // set when we can't merge with our children, and then move the
    // children into the current set
    for (QList<Partition *>::Iterator part = dag_entries->begin();
         part != dag_entries->end(); ++part)
    {
        current_partitions->insert(*part);
    }

    while (!current_partitions->isEmpty())
    {
        // Next time through the while loop we use these
        QSet<Partition *> * next_partitions = new QSet<Partition *>();

        // Keep track of which aren't merging and thus need to be advanced
        QSet<Partition *> advance_set = QSet<Partition *>();

        // In each of our current partitions, we're going to check for mergers
        for (QSet<Partition *>::Iterator part = current_partitions->begin();
             part != current_partitions->end(); ++part)
        {
            // Find neighboring children, we only want to consider those
            // that are a single dag leap away so we don't cause merge funkiness
            near_children.clear();
            for (QSet<Partition *>::Iterator child = (*part)->children->begin();
                 child != (*part)->children->end(); ++child)
            {
                (*child)->calculate_dag_leap();
                if ((*child)->dag_leap == (*part)->dag_leap + 1)
                {
                    near_children.insert(*child);
                }
            }

            bool merging = false;
            QList<int> task_ids = (*part)->events->keys();
            QSet<Partition *> added = QSet<Partition *>();

            // Go through tasks and see which children should be merged
            // If the child has the same task as the partition, we see
            // what the least common caller of those events in each partition are
            // If they are related by subtree, we merge them.
            for (QList<int>::Iterator taskid = task_ids.begin();
                 taskid != task_ids.end(); ++taskid)
            {
                Event * part_caller = (*part)->least_common_caller(*taskid, memo);
                Event * child_caller;
                for (QSet<Partition *>::Iterator child = near_children.begin();
                     child != near_children.end(); ++child)
                {
                    if ((*child)->events->contains(*taskid))
                    {
                        child_caller = (*child)->least_common_caller(*taskid, memo);

                        // We have a match, put them in the same merge group
                        if (part_caller->same_subtree(child_caller))
                        {
                            added.insert((*child));
                            merging = true; // This one is merging

                            // Update group stuff for parent and child
                            (*part)->group->unite(*((*child)->group));
                            for (QSet<Partition *>::Iterator group_member
                                 = (*part)->group->begin();
                                 group_member != (*part)->group->end();
                                 ++group_member)
                            {
                                if (*group_member != *part)
                                {
                                    toDelete->insert((*group_member)->group);
                                    (*group_member)->group = (*part)->group;
                                }
                            } // Group stuff
                        } // Child matches
                    } // Child has Task ID
                } // Looking through near children

                // Now that we have completed this set of task ids, update near_children
                // If this comes empty, we are done with this partition
                near_children.subtract(added);
                added.clear();
                if (near_children.empty())
                    break;

            } // Task ID

            // We looked at all tasks and did not merge, so we're done with this one.
            // In the next round, we'll want to look at its children instead
            if (!merging)
                advance_set.insert(*part);
        } // Partition in current set

        // Now do the merger - we go through the leap and look at each
        // partition's group and mark anything we've already merged.
        for (QSet<Partition *>::Iterator group = current_partitions->begin();
             group != current_partitions->end(); ++group)
        {
            if ((*group)->leapmark) // We've already merged this
                continue;

            Partition * p = new Partition();
            int min_leap = (*group)->dag_leap;

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
                        (*(p->events))[*k] = new QList<CommEvent *>();
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
                if (next_partitions->contains(*partition))
                    next_partitions->remove(*partition);
            }

            // Update leapmarks
            for (QSet<Partition *>::Iterator group_member
                 = (*group)->group->begin();
                 group_member != (*group)->group->end(); ++group_member)
            {
                (*group_member)->leapmark = true;
            }

            p->sortEvents();
            new_partitions->insert(p);

            // Setup next_partitions to have any new partition that isn't
            // a singleton from advance_set. For advance_set we want
            // the near children as we're moving on
            if (advance_set.contains(*group))
            {
                for (QSet<Partition *>::Iterator child = p->children->begin();
                     child != p->children->end(); ++child)
                {
                    (*child)->calculate_dag_leap();
                    if (p->dag_leap + 1 == (*child)->dag_leap)
                        next_partitions->insert(*child);
                }
            }
            else
            {
                next_partitions->insert(p);
            }

        } // End current partition merging

        delete current_partitions;
        current_partitions = next_partitions;
    } // End Current Partition
    delete current_partitions;

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
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list
             = (*partition)->events->begin();
             event_list != (*partition)->events->end(); ++event_list)
        {
            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                (*evt)->partition = *partition;
            }
        }
    }
    set_dag_steps();

    delete new_partitions;
    delete memo;
}

// This is the most difficult to understand part of the algorithm and the code.
// At least that's consistent!
void Trace::mergeByLeap()
{
    int leap = 0;
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
        QSet<int> tasks = QSet<int>();
        QSet<Partition *> * next_leap = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator partition = current_leap->begin();
             partition != current_leap->end(); ++partition)
        {
            tasks += QSet<int>::fromList((*partition)->events->keys());
        }


        // If this leap doesn't have all the tasks we have to do something
        if (tasks.size() < num_tasks)
        {
            QSet<Partition *> * new_leap_parts = new QSet<Partition *>();
            QSet<int> added_tasks = QSet<int>();
            bool back_merge = false;
            for (QSet<Partition *>::Iterator partition = current_leap->begin();
                 partition != current_leap->end(); ++partition)
            {
                // Determine distances from parent and child partitions
                // that are 1 leap away
                unsigned long long int parent_distance = ULLONG_MAX;
                unsigned long long int child_distance = ULLONG_MAX;
                if ((*partition)->parents->size()
                     - (*(*partition)->parents).size() == 0
                    && (*partition)->parents->size() != 0)
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
                    for (QSet<Partition *>::Iterator child
                         = (*partition)->children->begin();
                         child != (*partition)->children->end(); ++child)
                    {
                        if ((*child)->dag_leap == (*partition)->dag_leap + 1
                             && ((QSet<int>::fromList((*child)->events->keys())
                                  - tasks)).size() > 0)
                        {
                            added_tasks += (QSet<int>::fromList((*child)->events->keys())
                                                                     - tasks);
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

            if (!back_merge && added_tasks.size() <= 0)
            {
                // Skip leap if we didn't add anything
                if (options.leapSkip)
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
                            (*(p->events))[*k] = new QList<CommEvent *>();
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
        } // End If Tasks Incomplete
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
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list
             = (*partition)->events->begin();
             event_list != (*partition)->events->end(); ++event_list)
        {
            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
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
                    if ((*child)->min_global_step <= spanMax
                        && !working_set->contains(*child))
                    {
                        toAdd->insert(*child);
                        if ((*child)->max_global_step > spanMax)
                        {
                            spanMax = (*child)->max_global_step;
                        }
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
                    (*(p->events))[*k] = new QList<CommEvent *>();
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
    std::cout << "New dag..." << std::endl;
    delete dag_entries;
    dag_entries = new QList<Partition *>();
    for (QSet<Partition *>::Iterator partition = new_partitions->begin();
         partition != new_partitions->end(); ++partition)
    {
        partitions->append(*partition);
        if ((*partition)->parents->size() <= 0)
            dag_entries->append(*partition);

        // Update event's reference just in case
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list
             = (*partition)->events->begin();
             event_list != (*partition)->events->end(); ++event_list)
        {
            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
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
    for(QMap<int, QList<CommEvent *> *>::Iterator event_list
        = part->events->begin();
        event_list != part->events->end(); ++event_list)
    {
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            QSet<Partition *> * parts = (*evt)->mergeForMessagesHelper();
            for (QSet<Partition *>::Iterator opart = parts->begin();
                 opart != parts->end(); ++opart)
            {
                to_merge->insert(*opart);
                if (!(*opart)->mark && !to_process->contains(*opart))
                    to_process->enqueue(*opart);
            }
            delete parts;
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

// Looping section of Tarjan algorithm
void Trace::strong_connect_loop(Partition * part, QStack<Partition *> * stack,
                                      QList<Partition *> * children, int cIndex,
                                      QStack<QSharedPointer<RecurseInfo> > * recurse,
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
            recurse->push(QSharedPointer<RecurseInfo>(new RecurseInfo(part, child, children, cIndex)));
            recurse->push(QSharedPointer<RecurseInfo>(new RecurseInfo(child, NULL, NULL, -1)));
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
    QStack<QSharedPointer<RecurseInfo> > * recurse = new QStack<QSharedPointer<RecurseInfo> >();
    recurse->push(QSharedPointer<RecurseInfo>(new RecurseInfo(partition, NULL, NULL, -1)));
    while (recurse->size() > 0)
    {
        QSharedPointer<RecurseInfo> ri = recurse->pop();

        if (ri->cIndex < 0)
        {
            ri->part->tindex = index;
            ri->part->lowlink = index;
            ++index;
            stack->push(ri->part);
            Q_ASSERT(ri->children == NULL);
            ri->children = new QList<Partition *>();
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
                    (*(p->events))[*k] = new QList<CommEvent *>();
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
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list
             = (*partition)->events->begin();
             event_list != (*partition)->events->end(); ++event_list)
        {
            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
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
// last events in each task pointer.
void Trace::set_partition_dag()
{
    dag_entries->clear();
    bool parent_flag;
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        parent_flag = false;
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list
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


// In progress: will be used for extra information on aggregate events
QList<Trace::FunctionPair> Trace::getAggregateFunctions(CommEvent * evt)
{
    unsigned long long int stoptime = evt->enter;
    unsigned long long int starttime = 0;
    if (evt->comm_prev)
        starttime = evt->comm_prev->exit;

    QVector<Event *> * proots = roots->at(evt->task);
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

Event * Trace::findEvent(int task, unsigned long long time)
{
    Event * found = NULL;
    for (QVector<Event *>::Iterator root = roots->at(task)->begin();
         root != roots->at(task)->end(); ++root)
    {
        found = (*root)->findChild(time);
        if (found)
            return found;
    }

    return found;
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
