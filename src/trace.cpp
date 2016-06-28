//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Ravel.
// Written by Kate Isaacs, kisaacs@acm.org, All rights reserved.
// LLNL-CODE-663885
//
// For details, see https://github.com/scalability-llnl/ravel
// Please also see the LICENSE file for our notice and the LGPL.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//////////////////////////////////////////////////////////////////////////////
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
#include "entitygroup.h"
#include "otfcollective.h"
#include "ravelutils.h"
#include "primaryentitygroup.h"
#include "metrics.h"

Trace::Trace(int nt, int np)
    : name(""),
      fullpath(""),
      num_entities(nt),
      num_pes(np),
      units(-9),
      totalTime(0), // for paper timing
      partitions(new QList<Partition *>()),
      metrics(new QList<QString>()),
      metric_units(new QMap<QString, QString>()),
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
      max_time(-1),
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

void Trace::preprocess()
{
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    options = *_options;

    totalTimer.start();
    partition();
    assignSteps();

    emit(startClustering());

    qSort(partitions->begin(), partitions->end(),
          dereferencedLessThan<Partition>);
    addPartitionMetric(); // For debugging

    isProcessed = true;

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(totalTime, "Total Algorithm Time: ");
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

    // Merge communication
    // Note at this part the partitions do not have parent/child
    // relationships among them. That is first set in the merging of cycles.
    print_partition_info("Merging for messages...", "1-pre-message", true);
    traceTimer.start();
    mergeForMessages();
    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Message Merge: ");
    print_partition_info("", "2-post-message", true, true);
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
        graph << "\n  " << QString::number((*partition)->min_time).toStdString().c_str();
        graph << " - " << QString::number((*partition)->max_time).toStdString().c_str();
        graph << ", gv: " << (*partition)->gvid.toStdString().c_str();
        graph << ", ne: " << (*partition)->num_events();
        graph << ", name: " << (*partition)->debug_name;
        graph << "\"";
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
                                 bool partition_count)
{
    if (message.length() > 0)
        std::cout << message.toStdString().c_str() << std::endl;
    if (debug)
    {
        if (graph_name.length() > 0)
            output_graph("../debug-output/" + graph_name + ".dot");

        if (partition_count)
          std::cout << "Partitions = " << partitions->size() << std::endl;
    }
}
