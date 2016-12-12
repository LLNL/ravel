#include "trace.h"

#include <iostream>
#include <fstream>
#include <QElapsedTimer>
#include <QTime>
#include <cmath>
#include <climits>
#include <cfloat>

#include "entity.h"
#include "event.h"
#include "commevent.h"
#include "p2pevent.h"
#include "collectiveevent.h"
#include "function.h"
#include "rpartition.h"
#include "importoptions.h"
#include "gnome.h"
#include "exchangegnome.h"
#include "entitygroup.h"
#include "otfcollective.h"
#include "ravelutils.h"
#include "primaryentitygroup.h"
#include "metrics.h"

Trace::Trace(int nt, int np)
    : name(""),
      fullpath(""),
      num_entities(nt),
      num_application_entities(nt), // for debug
      num_pes(np),
      units(-9),
      use_aggregates(true),
      totalTime(0), // for paper timing
      partitions(new QList<Partition *>()),
      metrics(new QList<QString>()),
      metric_units(new QMap<QString, QString>()),
      gnomes(new QList<Gnome *>()),
      options(NULL),
      functionGroups(new QMap<int, QString>()),
      functions(new QMap<int, Function *>()),
      primaries(NULL),
      processingElements(NULL),
      entitygroups(NULL),
      collective_definitions(NULL),
      collectives(NULL),
      collectiveMap(NULL),
      events(new QVector<QVector<Event *> *>(std::max(nt, np))),
      roots(new QVector<QVector<Event *> *>(std::max(nt, np))),
      mpi_group(-1),
      global_max_step(-1),
      dag_entries(new QList<Partition *>()),
      dag_step_dict(new QMap<int, QSet<Partition *> *>()),
      isProcessed(false),
      totalTimer(QElapsedTimer())
{
    for (int i = 0; i < std::max(nt, np); i++) {
        (*events)[i] = new QVector<Event *>();
    }

    for (int i = 0; i < std::max(nt, np); i++)
    {
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

    for (QMap<int, EntityGroup *>::Iterator comm = entitygroups->begin();
         comm != entitygroups->end(); ++comm)
    {
        delete *comm;
        *comm = NULL;
    }
    delete entitygroups;

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


    for (QMap<int, PrimaryEntityGroup *>::Iterator primary = primaries->begin();
         primary != primaries->end(); ++primary)
    {
        for (QList<Entity *>::Iterator entity = primary.value()->entities->begin();
             entity != primary.value()->entities->end(); ++entity)
        {
            delete *entity;
            *entity = NULL;
        }
        delete primary.value();
    }
    delete primaries;
}

void Trace::preprocess(ImportOptions * _options)
{
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    options = *_options;
    if (options.origin == ImportOptions::OF_CHARM)
        use_aggregates = false;

    totalTimer.start();
    partition();
    assignSteps();

    emit(startClustering());
    std::cout << "Gnomifying..." << std::endl;
    if (options.cluster)
        gnomify();

    qSort(partitions->begin(), partitions->end(),
          dereferencedLessThan<Partition>);
    addPartitionMetric(); // For debugging

    isProcessed = true;

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Structure Extraction: ");
    RavelUtils::gu_printTime(totalTime, "Total Algorithm Time: ");
}

void Trace::preprocessFromSaved()
{
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    // Sets partition-to-partition connectors and min/max steps

    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        (*partition)->fromSaved();
        if ((*partition)->max_global_step > global_max_step)
            global_max_step = (*partition)->max_global_step;
    }

    set_partition_dag();

    emit(startClustering());
    std::cout << "Gnomifying..." << std::endl;
    if (options.cluster)
        gnomify();

    qSort(partitions->begin(), partitions->end(),
          dereferencedLessThan<Partition>);

    isProcessed = true;

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Gnome/Cluster Etc: ");
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

    if (options.origin != ImportOptions::OF_SAVE_OTF2)
    {
        metrics->append("Gnome");
        (*metric_units)["Gnome"] = "";
    }

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
                if (options.origin != ImportOptions::OF_SAVE_OTF2)
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
            if (options.origin != ImportOptions::OF_SAVE_OTF2)
                setGnomeMetric(*part, -1);
            (*part)->gnome->preprocess();
        }
        total += stepPortion * num_steps;
        emit(updateClustering(total));
    }

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Gnomification/Clustering: ");
}

void Trace::setGnomeMetric(Partition * part, int gnome_index)
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
         = part->events->begin();
         event_list != part->events->end(); ++event_list)
    {
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            (*evt)->metrics->addMetric("Gnome", gnome_index, gnome_index);
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
        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
             = (*part)->events->begin();
             event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                (*evt)->metrics->addMetric("Partition", partition, partition);
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
        // Merge communication
        // Note at this part the partitions do not have parent/child
        // relationships among them. That is first set in the merging of cycles.
        print_partition_info("Merging for messages...", "1-pre-message", true);
        traceTimer.start();
        mergeForMessages();
        traceElapsed = traceTimer.nsecsElapsed();
        RavelUtils::gu_printTime(traceElapsed, "Message Merge: ");
        print_partition_info("", "2-post-message", true, true);


        // Tarjan
        print_partition_info("Merging cycles...", "3-mergecycle-before");
        traceTimer.start();
        mergeCycles();
        traceElapsed = traceTimer.nsecsElapsed();
        RavelUtils::gu_printTime(traceElapsed, "Cycle Merge: ");
        print_partition_info("", "4-mergecycle-after", true, true);


        if (options.origin == ImportOptions::OF_CHARM)
        {
            // We can do entry repair until merge cycles because
            // it requires a dag. Then we have to do another
            // merge cycles in case we have once again destroyed the dag
            print_partition_info("Merge for entries");
            traceTimer.start();
            mergeForEntryRepair();
            print_partition_info("", "5-post-entryrepair", true, true);

            print_partition_info("Second merge cycles...");
            mergeCycles();
            traceElapsed = traceTimer.nsecsElapsed();
            RavelUtils::gu_printTime(traceElapsed, "Repair + Next Cycle Merge: ");
            print_partition_info("", "6-post-entryrepair-cycle", true, true);


            // Shortcut that is unnecessary with our current codes
            // but makes the final merging much faster.
            print_partition_info("Merge for atomics");
            mergeForEntryRepair(false);
            print_partition_info("", "7-post-atomics", true);


            print_partition_info("Third merge cycles...");
            mergeCycles();
            print_partition_info("", "8-post-atomics-cycle", true, true);
        }

          // Merge by rank level [ later ]
        if (options.leapMerge)
        {
            print_partition_info("Merging of complete leaps...");
            traceTimer.start();
            set_dag_steps();
            if (options.origin == ImportOptions::OF_CHARM)
            {
                //mergePrimaryByLeap();
            }
            else
            {
                mergeByLeap();
            }
            traceElapsed = traceTimer.nsecsElapsed();
            RavelUtils::gu_printTime(traceElapsed, "Leap Merge: ");
        }
    }

    // Partition - given
    // Form partitions given in some way -- need to write options for this [ later ]
    else
    {
        // Most of this was done in otfconverter and thus we only need to setup the dag
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

    int per_step = 2;
    if (!use_aggregates)
        per_step = 1;
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
                // We +per_step because individual steps start at 0, so when we add 0,
                // we want it to be offset from the parent
                accumulated_step = std::max(accumulated_step,
                                            (*parent)->max_global_step + per_step);
            }

            // Skip since all parents haven't been handled
            if (!allParents)
                continue;

            // Set steps for the partition
            (*part)->max_global_step = per_step * ((*part)->max_step)
                                       + accumulated_step;
            (*part)->min_global_step = accumulated_step;
            (*part)->mark = false; // Using this to debug again

            // Set steps for partition events
            for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
                 = (*part)->events->begin();
                 event_list != (*part)->events->end(); ++event_list)
            {
                for (QList<CommEvent *>::Iterator evt
                     = (event_list.value())->begin();
                     evt != (event_list.value())->end(); ++evt)
                {
                    (*evt)->step *= per_step;
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
        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
             = (*part)->events->begin();
             event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<CommEvent *>::Iterator evt
                 = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                (*evt)->calculate_differential_metric(metric_name,
                                                      base_name,
                                                      use_aggregates);
            }
        }
    }

}

void Trace::calculate_partition_metrics()
{
    metrics->append("Imbalance");
    (*metric_units)["Imbalance"] = RavelUtils::getUnits(units);
    metrics->append("PE Imbalance");
    (*metric_units)["PE Imbalance"] = RavelUtils::getUnits(units);
    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        (*part)->calculate_imbalance(num_pes);
    }
}

// Calculates duration difference per partition rather than global step
void Trace::calculate_partition_duration()
{
    QString p_duration = "Duration";
    metrics->append(p_duration);
    (*metric_units)[p_duration] = RavelUtils::getUnits(units);

    unsigned long long int minduration;
    int per_step = 2;
    if (!use_aggregates)
        per_step = 1;

    int count = 0;
    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        count++;
        for (int i = (*part)->min_global_step;
             i <= (*part)->max_global_step; i += per_step)
        {
            QList<CommEvent *> * i_list = new QList<CommEvent *>();

            for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
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
            minduration = ULLONG_MAX;

            for (QList<CommEvent *>::Iterator evt = i_list->begin();
                 evt != i_list->end(); ++evt)
            {
                // The time a send takes is actually charged to the amount
                // of time between it and the last thing that happend, so it
                // essentially acts as a divisor for its caller.
                // Receives are instantaneous because anything that happened
                // before them may be on another PE or due to another entity.
                // We set the extents here -- they default to the extents of the
                // comm message, but we allow them to be longer for charm++
                if ((*evt)->isReceive())
                {
                    CommEvent * next = *evt;
                    while (next->true_next && next->true_next->caller == (*evt)->caller)
                    {
                        next = next->true_next;
                    }

                    // Either start at ourselves or at the end of the last comm
                    if (next != *evt)
                    {
                        (*evt)->extent_begin = next->exit;
                    }
                    else
                    {
                        (*evt)->extent_begin = (*evt)->enter;
                    }

                    (*evt)->extent_end = std::max(next->exit, (*evt)->caller->exit);

                }
                else
                {

                    if ((*evt)->true_prev && (*evt)->caller == (*evt)->true_prev->caller) // From last comm
                    {
                        (*evt)->extent_begin = (*evt)->true_prev->exit;
                    }
                    else
                    {
                        (*evt)->extent_begin = (*evt)->caller->enter;
                    }

                    // Check if we are last and there is a recv to blame
                    // If not, go all the way to the end
                    if (!(*evt)->caller->callees->first()->isReceive()
                          && (!(*evt)->true_next || (*evt)->true_next->caller != (*evt)->caller))
                    {
                        (*evt)->extent_end = (*evt)->caller->exit;
                    }
                    else
                    {
                        (*evt)->extent_end = (*evt)->exit;
                    }

                }
                if ((*evt)->extent_end - (*evt)->extent_begin < minduration)
                    minduration = (*evt)->extent_end - (*evt)->extent_begin;
            }

            for (QList<CommEvent *>::Iterator evt = i_list->begin();
                 evt != i_list->end(); ++evt)
            {
                (*evt)->metrics->addMetric(p_duration, ((*evt)->extent_end - (*evt)->extent_begin)
                                                       - minduration);
            }
            delete i_list;
        }
    }

}

// Calculates lateness per partition rather than global step
void Trace::calculate_partition_lateness()
{
    QList<QString> counterlist = QList<QString>();

    if (options.origin != ImportOptions::OF_CHARM)
    {
        for (int i = 0; i < metrics->size(); i++)
        {
            counterlist.append(metrics->at(i));
        }
    }

    QList<double> valueslist = QList<double>();

    QString p_late = "Lateness";
    metrics->append(p_late);
    (*metric_units)[p_late] = RavelUtils::getUnits(units);

    QString p_duration = "Duration";
    if (options.origin != ImportOptions::OF_CHARM)
    {
        metrics->append(p_duration);
        (*metric_units)[p_duration] = RavelUtils::getUnits(units);
    }

    for (int i = 0; i < counterlist.size(); i++)
    {
        metrics->append("Step " + counterlist[i]);
        metric_units->insert("Step " + counterlist[i], counterlist[i]);
        valueslist.append(0);
        valueslist.append(0);
    }

    unsigned long long int mintime, aggmintime, duration, aggduration,
        minduration, minaggduration;
    int per_step = 2;
    if (!use_aggregates)
        per_step = 1;


    int count = 0;
    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        count++;
        for (int i = (*part)->min_global_step;
             i <= (*part)->max_global_step; i += per_step)
        {
            // Events at the same step in this partition
            QList<CommEvent *> * i_list = new QList<CommEvent *>();

            for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
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

            // Find min duration
            minduration = ULLONG_MAX;
            minaggduration = ULLONG_MAX;


            // Set lateness;
            if (use_aggregates)
            {
                for (QList<CommEvent *>::Iterator evt = i_list->begin();
                     evt != i_list->end(); ++evt)
                {
                    if ((*evt)->exit < mintime)
                        mintime = (*evt)->exit;
                    if ((*evt)->enter < aggmintime)
                        aggmintime = (*evt)->enter;

                    duration = (*evt)->exit - (*evt)->enter;
                    aggduration = (*evt)->getAggDuration();
                    if (duration < minduration)
                        minduration = duration;

                    if (aggduration < minaggduration)
                        minaggduration = aggduration;


                    for (int j = 0; j < counterlist.size(); j++)
                    {
                        if ((*evt)->getMetric(counterlist[j]) < valueslist[per_step*j])
                            valueslist[per_step*j] = (*evt)->getMetric(counterlist[j]);
                        if ((*evt)->getMetric(counterlist[j],true) < valueslist[per_step*j+1])
                            valueslist[per_step*j+1] = (*evt)->getMetric(counterlist[j], true);
                    }
                }

                for (QList<CommEvent *>::Iterator evt = i_list->begin();
                     evt != i_list->end(); ++evt)
                {
                    (*evt)->metrics->addMetric(p_late, (*evt)->exit - mintime,
                                               (*evt)->enter - aggmintime);

                    if (options.origin != ImportOptions::OF_CHARM)
                    {
                        (*evt)->metrics->addMetric(p_duration,
                                                   (*evt)->exit - (*evt)->enter - minduration,
                                                   (*evt)->getAggDuration() - minaggduration);

                        double evt_time = (*evt)->exit - (*evt)->enter;
                        double agg_time = (*evt)->enter;
                        if ((*evt)->comm_prev)
                            agg_time = (*evt)->enter - (*evt)->comm_prev->exit;
                        for (int j = 0; j < counterlist.size(); j++)
                        {
                            (*evt)->metrics->addMetric("Step " + counterlist[j],
                                                       (*evt)->getMetric(counterlist[j]) - valueslist[per_step*j],
                                                       (*evt)->getMetric(counterlist[j], true) - valueslist[per_step*j+1]);
                            (*evt)->metrics->addMetric(counterlist[j],
                                                       (*evt)->getMetric(counterlist[j]) / 1.0 / evt_time,
                                                       (*evt)->getMetric(counterlist[j], true) / 1.0 / agg_time);
                        }
                    }
                }
            }
            else
            {
                for (QList<CommEvent *>::Iterator evt = i_list->begin();
                     evt != i_list->end(); ++evt)
                {
                    if ((*evt)->exit < mintime)
                        mintime = (*evt)->exit;

                    duration = (*evt)->exit - (*evt)->enter;
                    if (duration < minduration)
                        minduration = duration;

                    for (int j = 0; j < counterlist.size(); j++)
                    {
                        if ((*evt)->getMetric(counterlist[j]) < valueslist[per_step*j])
                            valueslist[per_step*j] = (*evt)->getMetric(counterlist[j]);
                    }

                }

                for (QList<CommEvent *>::Iterator evt = i_list->begin();
                     evt != i_list->end(); ++evt)
                {
                    (*evt)->metrics->addMetric(p_late, (*evt)->exit - mintime);

                    if (options.origin != ImportOptions::OF_CHARM)
                    {

                        (*evt)->metrics->addMetric(p_duration,
                                                   (*evt)->exit - (*evt)->enter - minduration);

                        double evt_time = (*evt)->exit - (*evt)->enter;
                        for (int j = 0; j < counterlist.size(); j++)
                        {
                            (*evt)->metrics->addMetric("Step " + counterlist[j],
                                                       (*evt)->getMetric(counterlist[j]) - valueslist[per_step*j]);
                            (*evt)->metrics->addMetric(counterlist[j],
                                                       (*evt)->getMetric(counterlist[j]) / 1.0 / evt_time);
                        }
                    }
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
    (*metric_units)["G. Lateness"] = RavelUtils::getUnits(units);
    metrics->append("Colorless");
    (*metric_units)["Colorless"] = "";

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
    int per_step = 2;
    if (!use_aggregates)
        per_step = 1;

    for (int i = 0; i <= global_max_step; i+= per_step)
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
            for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
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
        if (use_aggregates)
        {
            for (QList<CommEvent *>::Iterator evt = i_list->begin();
                 evt != i_list->end(); ++evt)
            {
                (*evt)->metrics->addMetric("G. Lateness", (*evt)->exit - mintime,
                                           (*evt)->enter - aggmintime);
                (*evt)->metrics->addMetric("Colorless", 1, 1);
            }
        }
        else
        {
            for (QList<CommEvent *>::Iterator evt = i_list->begin();
                 evt != i_list->end(); ++evt)
            {
                (*evt)->metrics->addMetric("G. Lateness", (*evt)->exit - mintime);
                (*evt)->metrics->addMetric("Colorless", 1, 1);
            }
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
                    if ((*child)->min_global_step == i + per_step)
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

// What was left ambiguous is now set in stone
void Trace::finalizeEntityEventOrder()
{
    std::cout << "Forcing event order per partition (charm)..." << std::endl;
    if (options.reorderReceives)
    {
        for (QList<Partition *>::Iterator part = partitions->begin();
             part != partitions->end(); ++part)
        {
            //if (!(*part)->runtime)
                (*part)->receive_reorder();
            (*part)->finalizeEntityEventOrder();
        }
    }
    else
    {
        for (QList<Partition *>::Iterator part = partitions->begin();
             part != partitions->end(); ++part)
        {
            (*part)->finalizeEntityEventOrder();
        }
    }
}

// Ensure entries broken between runtime and non-runtime partitions are repaired.
// We want to merge all child partitions with this property together, not just
// for overlaps.  This is because the parent is one partition and if we had
// not broken due to runtime, so would this be.
void Trace::mergeForEntryRepair(bool entries)
{
    print_partition_info("Repairing entries...");
    QString debug_step = "7";
    if (entries)
        debug_step = "5";

    // We will try to do this in order
    set_partition_dag();
    dag_entries->clear();
    int count = 0;
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        if ((*partition)->parents->size() <= 0)
            dag_entries->append(*partition);
        (*partition)->debug_name = count;
        (*partition)->new_partition = *partition;
        count++;
    }

    print_partition_info("Set dag steps for repairs...", debug_step + "-tracegraph-repairnames-nosteps");
    set_dag_steps();
    print_partition_info("", debug_step + "-tracegraph-repairname", true);

    // Now we want to merge everything in the same leap... but we treat the
    // partitions with only runtime chares in them differently
    int leap = 0;
    QSet<Partition *> * new_partitions = new QSet<Partition *>();
    QSet<Partition *> * current_leap = new QSet<Partition *>();
    QSet<QSet<Partition *> *> * toDelete = new QSet<QSet<Partition *> *>();
    QSet<Partition *> * partition_delete = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin();
         part != dag_entries->end(); ++part)
    {
        current_leap->insert(*part);
    }

    QSet<Partition *> * repairees = new QSet<Partition *>();

    while (!current_leap->isEmpty())
    {
        print_partition_info("", debug_step + "-tracegraph-repairnames-" + QString::number(leap), true);

        QSet<Partition *> * next_leap = new QSet<Partition *>();
        QSet<QSet<Partition *> *> * merge_groups = new QSet<QSet<Partition *> *>();
        for (QSet<Partition *>::Iterator partition = current_leap->begin();
             partition != current_leap->end(); ++partition)
        {
            // If we have a broken entry, that means we must have switched
            // between runtime and app. That means we can definitely merge
            // all children because they should be all runtime or all app.
            QSet<Partition *> * child_group = NULL;
            Partition * key_child = NULL;
            repairees->clear();
            if (entries)
                (*partition)->broken_entries(repairees);
            else
                (*partition)->stitched_atomics(repairees);

            if (repairees->size() < 2)
                continue;

            for (QSet<Partition *>::Iterator child = repairees->begin();
                 child != repairees->end(); ++child)
            {
                if (!child_group)
                {
                    child_group = (*child)->group;
                    key_child = *child;
                }
                else
                {
                    child_group->unite(*((*child)->group));
                    for (QSet<Partition *>::Iterator group_member
                         = child_group->begin();
                         group_member != child_group->end();
                         ++group_member)
                    {
                        if (*group_member != key_child)
                        {
                            // Check if added by a partition
                            if (merge_groups->contains((*group_member)->group))
                                merge_groups->remove((*group_member)->group);

                            // Prep for deletion
                            toDelete->insert((*group_member)->group);

                            // Update
                            (*group_member)->group = child_group;
                        }
                    } // Update group members

                } // unite child group

            } // check partition's children
            if (child_group)
            {
                merge_groups->insert(child_group);
            }

        } // check current leap

        // Now do the merger - we go through the leap and look at each
        // partition's group and mark anything we've already merged.
        for (QSet<QSet<Partition *> *>::Iterator group = merge_groups->begin();
             group != merge_groups->end(); ++group)
        {
            Partition * p = new Partition();
            p->debug_name = count;
            p->new_partition = p;
            count++;
            int min_leap = leap + 1;

            bool runtime = false;
            for (QSet<Partition *>::Iterator partition = (*group)->begin();
                 partition != (*group)->end(); ++partition)
            {
                runtime = runtime || (*partition)->runtime;
                if ((*partition)->min_atomic < p->min_atomic)
                    p->min_atomic = (*partition)->min_atomic;
                if ((*partition)->max_atomic > p->max_atomic)
                    p->max_atomic = (*partition)->max_atomic;

                partition_delete->insert(*partition);
                (*partition)->new_partition = p;

                min_leap = std::min((*partition)->dag_leap, min_leap);

                // Merge all the events into the new partition
                QList<unsigned long> keys = (*partition)->events->keys();
                for (QList<unsigned long>::Iterator k = keys.begin(); k != keys.end(); ++k)
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
                    if (!((*group)->contains(*child)))
                    {
                        p->children->insert(*child);
                        for (QSet<Partition *>::Iterator group_member
                             = (*group)->begin();
                             group_member != (*group)->end();
                             ++group_member)
                        {
                            if ((*child)->parents->contains(*group_member))
                            {
                                (*child)->parents->remove(*group_member);
                            }
                        }
                        (*child)->parents->insert(p);
                    }
                }

                for (QSet<Partition *>::Iterator parent
                     = (*partition)->parents->begin();
                     parent != (*partition)->parents->end(); ++parent)
                {
                    if (!((*group)->contains(*parent)))
                    {
                        p->parents->insert(*parent);
                        for (QSet<Partition *>::Iterator group_member
                             = (*group)->begin();
                             group_member != (*group)->end();
                             ++group_member)
                        {
                            if ((*parent)->children->contains(*group_member))
                            {
                                (*parent)->children->remove(*group_member);
                            }
                        }
                        (*parent)->children->insert(p);
                    }
                }

                // Update new partitions
                if (new_partitions->contains(*partition))
                    new_partitions->remove(*partition);
            }

            p->runtime = runtime;
            p->sortEvents();
            new_partitions->insert(p);
        }

        // Next leap
        for (QSet<Partition *>::Iterator partition
             = current_leap->begin();
             partition != current_leap->end(); ++partition)
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

        delete current_leap;
        delete merge_groups;
        current_leap = next_leap;
    } // End through current leap
    delete current_leap;
    delete repairees;

    // Delete all the old partition groups
    for (QSet<QSet<Partition *> *>::Iterator group = toDelete->begin();
         group != toDelete->end(); ++group)
    {
        delete *group;
    }
    delete toDelete;

    // Get rid of stuff we put in new partitions
    for (QSet<Partition *>::Iterator partition = partition_delete->begin();
         partition != partition_delete->end(); ++partition)
    {
        partitions->removeOne(*partition);
        delete *partition;
    }

    // Need to calculate new dag_entries
    dag_entries->clear();
    for (QSet<Partition *>::Iterator partition = new_partitions->begin();
         partition != new_partitions->end(); ++partition)
    {
        partitions->append(*partition);
        if ((*partition)->parents->size() <= 0)
            dag_entries->append(*partition);

        // Update event's reference just in case
        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
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

// Ordering between unordered sends and merging
void Trace::mergeForCharmLeaps()
{
    print_partition_info("Forcing partition dag of unordered sends...",
                          "9a-tracegraph-pretrue", true, true);

    // First things first, change parent/child relationships based on true_next/true_prev
    int count = 0;
    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        (*part)->debug_functions = functions;

        (*part)->semantic_children();
        print_partition_info("", "9a-tracegraph-postsemantic-" + QString::number((*part)->debug_name));

        (*part)->true_children();
        print_partition_info("", "9a-tracegraph-poststrue-" + QString::number((*part)->debug_name), true);

        if ((*part)->group->size() > 1)
            std::cout << "Error: Incorrect group state" << std::endl;
    }
    print_partition_info("", "9b-tracegraph-posttrue", true, true);

    // fix cycles we have created
    mergeCycles();
    verify_partitions();

    dag_entries->clear();
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        if ((*partition)->parents->size() <= 0)
            dag_entries->append(*partition);
    }

    print_partition_info("", "9c-tracegraph-middle-nostep");

    // Now that we have done that, reset the dag leaps
    set_dag_steps();

    print_partition_info("","9d-tracegraph-middle");


    // Now we want to merge everything in the same leap... but we treat the
    // partitions with only runtime chares in them differently
    int leap = 0;
    QSet<Partition *> * new_partitions = new QSet<Partition *>();
    QSet<Partition *> * current_leap = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin();
         part != dag_entries->end(); ++part)
    {
        current_leap->insert(*part);
        (*part)->leapmark = false;
        (*part)->new_partition = *part;
    }

    QSet<QSet<Partition *> *> * to_remove = new QSet<QSet<Partition *> *>();

    // Problem is here -- instead of comparing all parts to all parts, need
    // to make use of the group status
    while (!current_leap->isEmpty())
    {
        QSet<Partition *> * next_leap = new QSet<Partition *>();

        // For each leap, we want to merge all groups that belong together
        // due to overlapping entities. However, we treat partitions that have
        // application entities (and possibly runtime ones) different from
        // those that have only runtime entities.
        for (QSet<Partition *>::Iterator part = current_leap->begin();
             part != current_leap->end(); ++part)
        {
            for (QSet<Partition *>::Iterator other = current_leap->begin();
                 other != current_leap->end(); ++other)
            {
                if ((*part)->group->contains(*other))
                    continue;

                if ((*part)->mergable(*other))
                {

                    // Update group stuff for parent and child
                    (*part)->group->unite(*((*other)->group));
                    for (QSet<Partition *>::Iterator group_member
                         = (*part)->group->begin();
                         group_member != (*part)->group->end();
                         ++group_member)
                    {
                        if (*group_member != *part)
                        {
                            to_remove->insert((*group_member)->group);
                            (*group_member)->group = (*part)->group;
                        }
                    } // Group stuff

                }
            }
        }

        // Now that the groups have been set up, we move to the next leap to create groups there
        // Set up the next leap
        for (QSet<Partition *>::Iterator part = current_leap->begin();
             part != current_leap->end(); ++part)
        {
            if ((*part)->dag_leap != leap)
                continue;

            for (QSet<Partition *>::Iterator child = (*part)->children->begin();
                 child != (*part)->children->end(); ++child)
            {
                (*child)->calculate_dag_leap();
                if ((*child)->dag_leap == leap + 1)
                    next_leap->insert(*child);
            }
        }

        leap++;

        delete current_leap;
        current_leap = next_leap;
    } // End Leap While
    delete current_leap;

    // Now we go through all our groups and merge them.
    // Now do the merger - we go through the leap and look at each
    // partition's group and mark anything we've already merged.
    for (QList<Partition *>::Iterator group = partitions->begin();
         group != partitions->end(); ++group)
    {
        if ((*group)->leapmark) // We've already merged this
            continue;

        Partition * p = new Partition();
        p->new_partition = p;
        int min_leap = (*group)->dag_leap;

        bool runtime = false;
        for (QSet<Partition *>::Iterator partition
             = (*group)->group->begin();
             partition != (*group)->group->end(); ++partition)
        {
            min_leap = std::min((*partition)->dag_leap, min_leap);
            runtime = runtime || (*partition)->runtime;
            if ((*partition)->min_atomic < p->min_atomic)
                p->min_atomic = (*partition)->min_atomic;
            if ((*partition)->max_atomic > p->max_atomic)
                p->max_atomic = (*partition)->max_atomic;

            (*partition)->new_partition = p;

            // Merge all the events into the new partition
            QList<unsigned long> keys = (*partition)->events->keys();
            for (QList<unsigned long>::Iterator k = keys.begin(); k != keys.end(); ++k)
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
            p->runtime = runtime;


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

    } // End current partition merging


    // Delete all the old partition groups
    for (QSet<QSet<Partition *> *>::Iterator group = to_remove->begin();
        group != to_remove->end(); ++group)
    {
        delete *group;
    }
    delete to_remove;

    // Delete all old partitions and add new ones
    for (QList<Partition *>::Iterator partition = partitions->begin();
        partition != partitions->end(); ++partition)
    {
        if (!(new_partitions->contains(*partition)))
            delete *partition;
    }
    delete partitions;
    partitions = new QList<Partition *>();
    count = 0;
    for (QSet<Partition *>::Iterator partition = new_partitions->begin();
        partition != new_partitions->end(); ++partition)
    {
        partitions->append(*partition);
        if (debug)
        {
            (*partition)->debug_name = count;
            count++;
        }
    }
}



//Sweep through by leap ordering partitions that have entity overlaps
void Trace::forcePartitionDag()
{
    print_partition_info("Enforcing partition DAG...");
    int leap = 0;
    QSet<Partition *> to_remove = QSet<Partition *>();
    QSet<Partition *> * current_leap = new QSet<Partition *>();
    for (QList<Partition *>::Iterator part = dag_entries->begin();
         part != dag_entries->end(); ++part)
    {
        current_leap->insert(*part);
    }

    Partition * earlier, *later;

    while (!current_leap->isEmpty())
    {
        QSet<Partition *> * next_leap = new QSet<Partition *>();

        for (QSet<Partition *>::Iterator part = current_leap->begin();
             part != current_leap->end(); ++part)
        {
            // We have already dealt with this one and/or moved it forward
            if ((*part)->dag_leap != leap)
                continue;

            for (QSet<Partition *>::Iterator other = current_leap->begin();
                 other != current_leap->end(); ++other)
            {
                // Cases in which we skip this one
                if (*other == *part)
                    continue;
                if ((*other)->dag_leap != leap)
                    continue;

                QSet<unsigned long> overlap_entities = (*part)->entity_overlap(*other);
                if (!overlap_entities.isEmpty())
                {
                    // Now we have to figure out which one comes before the other
                    // We then create a parent/child relationship
                    // - We should be careful if they both have the same parents
                    // so that the correct ordering of that is preserved but only
                    // parents that have an overlap
                    // - We set the dag_leap as a mark so we know to move on
                    // - We set the dag_leap again later when we are doing
                    // parent/child lookups in order to set the next_leap
                    earlier = (*part)->earlier_partition(*other, overlap_entities);
                    later = *other;
                    if (earlier == *other)
                    {
                        later = *part;
                    }


                    // Find overlapping parents that need to be removed
                    for (QSet<Partition *>::Iterator parent = later->parents->begin();
                         parent != later->parents->end(); ++parent)
                    {
                        // This parent is not shared
                        if (!earlier->parents->contains(*parent))
                            continue;

                        // Now test if parent has a entity overlap, if so stage it
                        // for removal
                        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator entitylist
                             = (*parent)->events->begin();
                             entitylist != (*parent)->events->end(); ++entitylist)
                        {
                            if (overlap_entities.contains(entitylist.key()))
                            {
                                to_remove.insert(*parent);
                                continue;
                            }
                        }
                    }

                    // Now remove the overlaps that we found
                    for (QSet<Partition *>::Iterator parent = to_remove.begin();
                         parent != to_remove.end(); ++parent)
                    {
                        (*parent)->children->remove(later);
                        later->parents->remove(*parent);
                    }

                    // Now set up earlier vs. later relationship
                    later->parents->insert(earlier);
                    earlier->children->insert(later);
                    later->dag_leap = leap + 1;
                }
            }
        }

        // Set up the next leap
        for (QSet<Partition *>::Iterator part = current_leap->begin();
             part != current_leap->end(); ++part)
        {
            if ((*part)->dag_leap != leap)
                continue;

            for (QSet<Partition *>::Iterator child = (*part)->children->begin();
                 child != (*part)->children->end(); ++child)
            {
                (*child)->calculate_dag_leap();
                if ((*child)->dag_leap == leap + 1)
                {
                    next_leap->insert(*child);
                }
            }
        }

        leap++;

        delete current_leap;
        current_leap = next_leap;
    } // End Leap While
    delete current_leap;

    // Need to calculate new dag_entries
    dag_entries->clear();
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        if ((*partition)->parents->size() <= 0)
        {
            dag_entries->append(*partition);
        }
    }

    // Need to finally fix partitions that should parent each other but don't
    // e.g. those that don't have send-relations  but should have happens before
    // in order to force this dag. We start from the back.
    set_dag_steps();
    leap -= 1; // Should be the last leap
    QMap<unsigned long, int> entity_to_last_leap = QMap<unsigned long, int>();

    int found_leap;
    QSet<Partition *> * search_leap;
    QSet<unsigned long> found_entities = QSet<unsigned long>();
    QSet<int> found_leaps = QSet<int>();
    QSet<unsigned long> seen_entities = QSet<unsigned long>();
    while (leap >= 0)
    {
        current_leap = dag_step_dict->value(leap);
        seen_entities.clear();
        for (QSet<Partition *>::Iterator part = current_leap->begin();
             part != current_leap->end(); ++part)
        {
            // Let's test for entities! If we're okay, we need not do anything
            QList<unsigned long> myentities = (*part)->events->keys();
            for (QList<unsigned long>::Iterator t = myentities.begin(); t != myentities.end(); ++t)
            {
                seen_entities.insert(*t);
            }

            QSet<unsigned long> missing = (*part)->check_entity_children();
            if (missing.isEmpty())
                continue;

            found_leaps.clear();
            for (QSet<unsigned long>::Iterator element = missing.begin();
                 element != missing.end(); ++element)
            {
                if (entity_to_last_leap.contains(*element))
                {
                    found_leap = entity_to_last_leap[*element];
                    found_leaps.insert(found_leap);
                }
            }


            QList<int> leap_list = found_leaps.toList();
            qSort(leap_list);
            for (QList<int>::Iterator next_leap = leap_list.begin();
                 next_leap != leap_list.end(); ++next_leap)
            {
                found_entities.clear();
                search_leap = dag_step_dict->value(*next_leap);
                // Go through the leap setting the links whenever missing entities are found
                for (QSet<Partition *>::Iterator spart = search_leap->begin();
                     spart != search_leap->end(); ++spart)
                {
                    if ((missing & (*spart)->events->keys().toSet()).size() != 0)
                    {
                        (*part)->children->insert(*spart);
                        (*spart)->parents->insert(*part);
                        found_entities.unite((*spart)->events->keys().toSet());
                    }
                }
                missing.subtract(found_entities);
            }
        }
        // Update the last leap of chares at this leap to this one
        for (QSet<unsigned long>::Iterator t = seen_entities.begin();
             t != seen_entities.end(); ++t)
        {
            entity_to_last_leap[*t] = leap;
        }
        leap--;
    }
}

// Iterates through all partitions and sets the steps
void Trace::assignSteps()
{
    // Step
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    print_partition_info("Setting DAG leaps...", "", true);
    set_dag_steps();
    print_partition_info("", "", true);

    if (options.origin == ImportOptions::OF_CHARM)
    {

        print_partition_info("Merging for Charm leaps", "9-tracegraph-before");
        traceTimer.start();
        mergeForCharmLeaps();
        print_partition_info("Setting up DAG...", "9X-postcharmleap-preverify", true, true);

        set_dag_entries();
        set_dag_steps();
        print_partition_info("", "10-tracegraph-charmleap", true);

        mergeCycles();
        traceElapsed = traceTimer.nsecsElapsed();
        RavelUtils::gu_printTime(traceElapsed, "Charm Leap Merge: ");
        print_partition_info("", "10X-tracegraph-cyclepostleap", true, true);

        traceTimer.start();
        forcePartitionDag(); // overlap due to runtime versus application
        traceElapsed = traceTimer.nsecsElapsed();
        RavelUtils::gu_printTime(traceElapsed, "Enforcing Partition Dag: ");
        print_partition_info("", "11-tracegraph-after", true);

        finalizeEntityEventOrder();
    }
    else if (options.reorderReceives)
    {
        print_partition_info("Re-ordering out-of-order receives");
        for (QList<Partition *>::Iterator part = partitions->begin();
             part != partitions->end(); ++part)
        {
            (*part)->receive_reorder_mpi();
            (*part)->finalizeEntityEventOrder();

        }
    }

    print_partition_info("Assigning local steps");
    traceTimer.start();
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
            (*partition)->debug_name = currentIter;
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
            (*partition)->debug_name = currentIter;
            (*partition)->basic_step();
        }
    }
    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Local Stepping: ");
    print_partition_info("Setting global steps", "12-tracegraph-named");

    traceTimer.start();
    set_global_steps();

    if (options.globalMerge) {
        print_partition_info("Merging global steps");
        mergeGlobalSteps();
    }

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Global Stepping: ");
    totalTime += totalTimer.nsecsElapsed();
    RavelUtils::gu_printTime(totalTime, "Total Algorithm Time: ");
    print_partition_info("Calculating step-wise metrics", "13-dag-global", false, true);

    // Calculate Step metrics
    traceTimer.start();

    if (options.origin == ImportOptions::OF_CHARM)
    {
        calculate_partition_duration();
        calculate_partition_metrics();
    }
    else
    {
        calculate_partition_lateness();
        calculate_differential_lateness("D. Lateness", "Lateness");
        calculate_lateness();
        calculate_differential_lateness("D.G. Lateness", "G. Lateness");
    }

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Metrics Calculation: ");
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
        QSet<unsigned long> entities = QSet<unsigned long>();
        QSet<Partition *> * next_leap = new QSet<Partition *>();
        for (QSet<Partition *>::Iterator partition = current_leap->begin();
             partition != current_leap->end(); ++partition)
        {
            entities += QSet<unsigned long>::fromList((*partition)->events->keys());
        }


        // If this leap doesn't have all the entities we have to do something
        if (entities.size() < num_entities)
        {
            QSet<Partition *> * new_leap_parts = new QSet<Partition *>();
            QSet<unsigned long> added_entities = QSet<unsigned long>();
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
                             && ((QSet<unsigned long>::fromList((*child)->events->keys())
                                  - entities)).size() > 0)
                        {
                            added_entities += (QSet<unsigned long>::fromList((*child)->events->keys())
                                                                     - entities);
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

            if (!back_merge && added_entities.size() <= 0)
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
                    QList<unsigned long> keys = (*partition)->events->keys();
                    for (QList<unsigned long>::Iterator k = keys.begin(); k != keys.end(); ++k)
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
        } // End If Entities Incomplete
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
    dag_entries->clear();
    for (QSet<Partition *>::Iterator partition = new_partitions->begin();
         partition != new_partitions->end(); ++partition)
    {
        partitions->append(*partition);
        if ((*partition)->parents->size() <= 0)
            dag_entries->append(*partition);

        // Update event's reference just in case
        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
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
            QList<unsigned long> keys = (*partition)->events->keys();
            for (QList<unsigned long>::Iterator k = keys.begin(); k != keys.end(); ++k)
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
    dag_entries->clear();
    for (QSet<Partition *>::Iterator partition = new_partitions->begin();
         partition != new_partitions->end(); ++partition)
    {
        partitions->append(*partition);
        if ((*partition)->parents->size() <= 0)
            dag_entries->append(*partition);

        // Update event's reference just in case
        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
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
    for(QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
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
    std::cout << "Setting partition dag..." << std::endl;
    set_partition_dag();

    std::cout << "Tarjan... " << std::endl;
    QList<QList<Partition *> *> * components = tarjan();
    emit(updatePreprocess(43, "Merging cycles..."));

    std::cout << "Merging from the cycles..." << std::endl;
    mergePartitions(components);
    emit(updatePreprocess(45, "Merging cycles..."));
}


// Input: A list of partition lists. Each partition list should be merged
// into a single partition. This updates parent/child relationships so
// there is no need to reset the dag.
void Trace::mergePartitions(QList<QList<Partition *> *> * components) {
    QElapsedTimer traceTimer;
    qint64 traceElapsed;
    traceTimer.start();

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
        bool runtime = false;
        for (QList<Partition *>::Iterator partition = (*component)->begin();
             partition != (*component)->end(); ++partition)
        {
            (*partition)->new_partition = p;
            runtime = runtime || (*partition)->runtime;
            if ((*partition)->min_atomic < p->min_atomic)
                p->min_atomic = (*partition)->min_atomic;
            if ((*partition)->max_atomic > p->max_atomic)
                p->max_atomic = (*partition)->max_atomic;

            // Merge all the events into the new partition
            QList<unsigned long> keys = (*partition)->events->keys();
            for (QList<unsigned long>::Iterator k = keys.begin(); k != keys.end(); ++k)
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

        p->runtime = runtime;
        merged->append(p);
    }

    // Now that we have all the merged partitions, figure out parents/children
    // between them, and sort the event lists and such
    // Note we could just set the event partition and then use
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
        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
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

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Partition Merge: ");
}

// Determine the sources of the dag
void Trace::set_dag_entries()
{
    // Need to calculate new dag_entries
    dag_entries->clear();
    for (QList<Partition *>::Iterator partition = partitions->begin();
         partition != partitions->end(); ++partition)
    {
        if ((*partition)->parents->size() <= 0)
            dag_entries->append(*partition);

        // Update event's reference just in case
        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
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
}


// Set all the parents/children in the partition by checking where first and
// last events in each entity pointer.
void Trace::set_partition_dag()
{
    dag_entries->clear();
    bool parent_flag;
    if (options.origin == ImportOptions::OF_CHARM)
    {
        for (QList<Partition *>::Iterator partition = partitions->begin();
             partition != partitions->end(); ++partition)
        {
            parent_flag = false;

            for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
                 = (*partition)->events->begin();
                 event_list != (*partition)->events->end(); ++event_list)
            {
                // For charm we have to look at every entry and weird stuff
                // happens with order and comm_next/comm_prev so we have to
                // check for self insertion
                for (QList<CommEvent *>::Iterator evt = event_list.value()->begin();
                     evt != event_list.value()->end(); ++evt)
                {
                    if ((*evt)->comm_prev && (*evt)->comm_prev->partition != *partition)
                    {
                        (*partition)->parents->insert((*evt)->comm_prev->partition);
                        (*evt)->comm_prev->partition->children->insert(*partition);
                        parent_flag = true;
                    }
                    if ((*evt)->true_prev && (*evt)->true_prev->partition != *partition
                            && (*evt)->atomic - 1 == (*evt)->true_prev->atomic
                            && (*evt)->true_prev->partition->max_atomic < (*evt)->atomic)
                    {
                        (*partition)->parents->insert((*evt)->true_prev->partition);
                        (*evt)->true_prev->partition->children->insert(*partition);
                        parent_flag = true;
                    }

                    if ((*evt)->comm_next && (*evt)->comm_next->partition != *partition)
                    {
                        (*partition)->children->insert((*evt)->comm_next->partition);
                        (*evt)->comm_next->partition->parents->insert(*partition);
                    }
                    if((*evt)->true_next && (*evt)->true_next->partition != *partition
                       && (*evt)->atomic + 1 == (*evt)->true_next->atomic
                       && (*evt)->true_next->partition->min_atomic > (*evt)->atomic)
                    {
                        (*partition)->children->insert((*evt)->true_next->partition);
                        (*evt)->true_next->partition->parents->insert(*partition);
                    }
                }

            }

            if (!parent_flag)
                dag_entries->append(*partition);
        }
    }
    else
    {
        for (QList<Partition *>::Iterator partition = partitions->begin();
             partition != partitions->end(); ++partition)
        {
            parent_flag = false;

            for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
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

    clear_dag_step_dict();
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

    QVector<Event *> * proots = roots->at(evt->entity);
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

// Find the smallest event in a timeline that contains the given time
Event * Trace::findEvent(int entity, unsigned long long time)
{
    Event * found = NULL;
    for (QVector<Event *>::Iterator root = roots->at(entity)->begin();
         root != roots->at(entity)->end(); ++root)
    {
        found = (*root)->findChild(time);
        if (found)
            return found;
    }

    return found;
}

void Trace::clear_dag_step_dict()
{
    for (QMap<int, QSet<Partition *> *>::Iterator set = dag_step_dict->begin();
         set != dag_step_dict->end(); ++set)
    {
        delete set.value();
    }
    dag_step_dict->clear();
}

// For debugging
// Ensure partitions have the following properties:
// 1) Partition only contains its own events
// 2) Partitions with runtime events are marked as runtime partitions
// 3) Partitions are not their own parents
void Trace::verify_partitions()
{
    return;
    std::cout << "Verifying " << partitions->size() << " partitions." << std::endl;
    for (QList<Partition *>::Iterator part = partitions->begin();
         part != partitions->end(); ++part)
    {
        if (!(*part)->verify_members())
        {
            std::cout << "BROKEN PARTITION" << std::endl;
            return;
        }
        if (!(*part)->verify_runtime(num_application_entities))
        {
            std::cout << "RUNTIME MISMATCH" << std::endl;
            return;
        }
        if (!(*part)->verify_parents())
        {
            std::cout << "SELF PARENT" << std::endl;
            return;
        }
    }
}

// use GraphViz to see partition graph for debugging
void Trace::output_graph(QString filename, bool byparent)
{
    std::cout << "Writing: " << filename.toStdString().c_str() << std::endl;
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
        graph << "\n  " << QString::number((*partition)->min_global_step).toStdString().c_str();
        graph << " - " << QString::number((*partition)->max_global_step).toStdString().c_str();
        graph << ", gv: " << (*partition)->gvid.toStdString().c_str();
        graph << ", ne: " << (*partition)->num_events();
        graph << ", leap: " << (*partition)->dag_leap;
        graph << ", name: " << (*partition)->debug_name;
        if (options.origin == ImportOptions::OF_CHARM)
        {
            graph << (*partition)->get_callers(functions).toStdString().c_str();
        }
        if (options.origin == ImportOptions::OF_CHARM && !(*partition)->verify_members())
        {
            graph << "\n BROKEN";
            graph << "\"";
            graph << " color=red";
        }
        else
        {
            graph << "\"";
        }
        graph << "];\n";
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

// Print debug information
void Trace::print_partition_info(QString message,
                                 QString graph_name,
                                 bool partition_verify,
                                 bool partition_count)
{
    if (message.length() > 0)
        std::cout << message.toStdString().c_str() << std::endl;
    if (debug)
    {
        if (partition_verify)
            verify_partitions();

        if (graph_name.length() > 0)
            output_graph("../debug-output/" + graph_name + ".dot");

        if (partition_count)
          std::cout << "Partitions = " << partitions->size() << std::endl;
    }
}
