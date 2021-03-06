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
#include "rpartition.h"
#include <iostream>
#include <fstream>
#include <climits>

#include "event.h"
#include "commevent.h"
#include "collectiverecord.h"
#include "clusterentity.h"
#include "ravelutils.h"
#include "message.h"
#include "p2pevent.h"
#include "function.h"
#include "metrics.h"
#include "gnome.h"

#include "trace.h"

Partition::Partition()
    : events(new QMap<unsigned long, QList<CommEvent *> *>),
      max_step(-1),
      max_global_step(-1),
      min_global_step(-1),
      dag_leap(-1),
      runtime(false),
      mark(false),
      parents(new QSet<Partition *>()),
      children(new QSet<Partition *>()),
      old_parents(new QSet<Partition *>()),
      old_children(new QSet<Partition *>()),
      new_partition(NULL),
      tindex(-1),
      lowlink(-1),
      leapmark(false),
      group(new QSet<Partition *>()), // delete in trace or turn to smart pointer
      min_atomic(INT_MAX),
      max_atomic(-1),
      metrics(new Metrics()),
      gvid(""),
      gnome(NULL),
      gnome_type(0),
      cluster_entities(new QVector<ClusterEntity *>()),
      cluster_vectors(new QMap<int, QVector<long long int> *>()),
      cluster_step_starts(new QMap<int, int>()),
      debug_mark(false),
      debug_name(-1),
      debug_functions(NULL),
      free_recvs(NULL)
{
    group->insert(this); // We are always in our own group
}

Partition::~Partition()
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator eitr = events->begin();
         eitr != events->end(); ++eitr)
    {
        // Don't necessarily delete events as they are saved by merging
        delete eitr.value();
    }
    delete events;

    delete parents;
    delete children;
    delete old_parents;
    delete old_children;
    delete metrics;
    delete gnome;

    for (QMap<int, QVector<long long int> *>::Iterator itr = cluster_vectors->begin();
         itr != cluster_vectors->end(); ++itr)
    {
        delete itr.value();
    }
    delete cluster_vectors;
    delete cluster_step_starts;
}

// Call when we are sure we want to delete events held in this partition
// (e.g. destroying the trace)
void Partition::deleteEvents()
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator eitr = events->begin();
         eitr != events->end(); ++eitr)
    {
        for (QList<CommEvent *>::Iterator itr = (eitr.value())->begin();
             itr != (eitr.value())->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
    }
}

bool Partition::operator<(const Partition &partition)
{
    return min_global_step < partition.min_global_step;
}

bool Partition::operator>(const Partition &partition)
{
    return min_global_step > partition.min_global_step;
}

bool Partition::operator<=(const Partition &partition)
{
    return min_global_step <= partition.min_global_step;
}

bool Partition::operator>=(const Partition &partition)
{
    return min_global_step >= partition.min_global_step;
}

bool Partition::operator==(const Partition &partition)
{
    return min_global_step == partition.min_global_step;
}


// Does not order, just places them at the end
void Partition::addEvent(CommEvent * e)
{
    if (events->contains(e->entity))
    {
        ((*events)[e->entity])->append(e);
    }
    else
    {
        (*events)[e->entity] = new QList<CommEvent *>();
        ((*events)[e->entity])->append(e);
    }
}

void Partition::sortEvents(){
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        qSort((event_list.value())->begin(), (event_list.value())->end(),
              dereferencedLessThan<CommEvent>);
    }
}


// The minimum over all entities of the time difference between the last event
// in one partition and the first event in another, per entity
unsigned long long int Partition::distance(Partition * other)
{
    unsigned long long int dist = ULLONG_MAX;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        if (other->events->contains(event_list.key()))
        {
            CommEvent * my_last = (event_list.value())->last();
            CommEvent * other_first = ((*(other->events))[event_list.key()])->first();
            if (other_first->enter > my_last->exit)
                dist = std::min(dist, other_first->enter - my_last->exit);
            else
            {
                CommEvent * my_first = (event_list.value())->first();
                CommEvent * other_last = ((*(other->events))[event_list.key()])->last();
                dist = std::min(dist, my_first->enter - other_last->exit);
            }
        }
    }
    return dist;
}

void Partition::fromSaved()
{
    // Make sure events are sorted
    sortEvents();

    // Parent/Children handled in trace, this will just set min/max steps
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        if (event_list.value()->first()->step < min_global_step || min_global_step < 0)
            min_global_step = event_list.value()->first()->step;
        if (event_list.value()->last()->step > max_global_step)
            max_global_step = event_list.value()->last()->step;
    }
}


// Requires parent's dag leaps to be up to date
void Partition::calculate_dag_leap()
{
    dag_leap = 0;
    for (QSet<Partition *>::Iterator parent = parents->begin();
         parent != parents->end(); ++parent)
    {
        dag_leap = std::max(dag_leap, (*parent)->dag_leap + 1);
    }

}

void Partition::set_atomics()
{

}

// Set children based on semantic heuristics like comm_next/comm_prev (though
// that should have been done already) and atomic order.
// For atomic order, we just need to know the number since we know by our
// events list are by entity so we know all atomic numbers belong to the
// same entity.
void Partition::semantic_children()
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            // comm_next/comm_prev
            if ((*evt)->comm_next && (*evt)->comm_next->partition != this)
            {
                Partition * p = (*evt)->comm_next->partition;
                children->insert(p);
                p->parents->insert(this);
            }

            // atomic - check if true_next is an atomic difference
            if ((*evt)->true_next && (*evt)->true_next->partition != this)
            {
                if ((*evt)->atomic + 1 == (*evt)->true_next->atomic
                    && (*evt)->atomic == max_atomic)
                {
                    Partition * p = (*evt)->true_next->partition;
                    children->insert(p);
                    p->parents->insert(this);
                }
            }
        }
    }
}


// True the partition... anything we a true_next that is not us we will set as
// our child partition.
void Partition::true_children()
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            // Let's try this only with sends as they carry more meaning
            // whereas receives can come late, especially if we need to collect
            // a bunch of receives before something happens
            // We also only want to do this with 'first' sends, thouse
            // without a comm_prev
            if (!(*evt)->isReceive() && ((*evt)->comm_prev == NULL
                                         || (*evt)->comm_prev->partition != this)
               )
            {
                CommEvent * tmp = (*evt)->true_next;
                while (tmp)
                {
                    if (!tmp->isReceive() && tmp->comm_prev == NULL)
                    {
                        if (tmp->partition != this)
                        {
                            Partition * p = tmp->partition;
                            children->insert(p);
                            p->parents->insert(this);
                        }
                        break;
                    }
                    tmp = tmp->true_next;
                }
            }

            // But let's also do it in the case of a receive followed immediately by an atomic?
            else if ((*evt)->isReceive() && (*evt)->true_next && (*evt)->true_next->atomic >= 0
                     && !(*evt)->true_next->isReceive() && (*evt)->true_next->partition != this
                     && (*evt)->true_next->comm_prev == NULL)
            {
                Partition * p = (*evt)->true_next->partition;
                children->insert(p);
                p->parents->insert(this);
            }
        }
    }
}

// Check if these are mergable in that they are the same from a runtime
// inclusion standpoint and they have entity overlap.
bool Partition::mergable(Partition * other)
{
    if (runtime != other->runtime)
        return false;

    bool overlap = false;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator entitylist = events->begin();
         entitylist != events->end(); ++entitylist)
    {
        if (other->events->contains(entitylist.key()))
            overlap = true;
    }
    return overlap;
}

// Check if this partition shares a broken entry with one of its children.
// The only ones with broken entries should be the child partitions.
bool Partition::broken_entry(Partition * child)
{
    CommEvent * evt = NULL;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        if (evtlist.value()->size() == 0)
            continue;
        for (int i = evtlist.value()->size() - 1; i >= 0; i--)
        {
            // The event comm_next must point to the child partition
            // and the callers must be the same.
            // Note we may want to re-write this whole bit later to
            // just find and merge all children.
            evt = evtlist.value()->at(i);
            if (evt->comm_next && evt->comm_next->partition == child
                && evt->caller == evt->comm_next->caller)
            {
                return true;
            }
        }
    }
    return false;
}

void Partition::broken_entries(QSet<Partition *> * repairees)
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            if ((*evt)->comm_next && (*evt)->comm_next->partition->newest_partition() != this
                && (*evt)->caller == (*evt)->comm_next->caller
                && runtime != (*evt)->comm_next->partition->newest_partition()->runtime)
            {
                repairees->insert((*evt)->comm_next->partition->newest_partition());
            }
        }
    }
}

void Partition::stitched_atomics(QSet<Partition *> * stitchees)
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            if ((*evt)->true_next && (*evt)->true_next->partition->newest_partition() != this
                && (*evt)->atomic == max_atomic
                && (*evt)->atomic + 1 == (*evt)->true_next->atomic)
            {
                stitchees->insert((*evt)->true_next->partition->newest_partition());
            }
        }
    }
}

QSet<unsigned long> Partition::check_entity_children()
{
    QSet<unsigned long> entity_children = events->keys().toSet();
    for (QSet<Partition *>::Iterator child = children->begin();
         child != children->end(); ++child)
    {
        entity_children.subtract((*child)->events->keys().toSet());
        if (entity_children.isEmpty())
            return entity_children;
    }
    return entity_children;
}

// Find entity overlaps between partitions.
QSet<unsigned long> Partition::entity_overlap(Partition * other)
{
    QSet<unsigned long> overlap = QSet<unsigned long>();
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator entitylist = events->begin();
         entitylist != events->end(); ++entitylist)
    {
        // We can't compare
        if (other->events->contains(entitylist.key()))
            overlap.insert(entitylist.key());
    }

    return overlap;
}

// Figure out which partition comes before the other. This
// assumes that there is at least one entity of overlap.
// If we can find a caller (comm_next/comm_prev) ordering,
// we will use that. Otherwise, we will take a vote of
// which has the earliest earlier events per entity.
// Presumable we won't have the latter happening since that
// would be set up in order by earlier stuff and hopefully
// any cycles would have been found their earlier as well.
//
// Actually, what probably matters most are the partition beginning events
// so we're only going to count those at the beginning of each partition
// e.g. things that are sends where the comm_prev is non-existent or
// is in a different partition.
// We also want to take PE into account since some will share a PE.
// Then we can probably compare them even if they're different entities.
Partition * Partition::earlier_partition(Partition * other, QSet<unsigned long> overlap_entities)
{
    // Counts for which one has the earlier earliest event
    unsigned long me = 0, them = 0, me_both = 0, them_both = 0;
    bool mine_first, theirs_first;

    QMap<unsigned long, QList<CommEvent *> *> by_pe = QMap<unsigned long, QList<CommEvent *> *>();

    for (QSet<unsigned long>::Iterator entity = overlap_entities.begin();
         entity != overlap_entities.end(); ++entity)
    {
        // Now let's just do the voting and avoid the comm/prev/next
        // thing for now because we believe it already taken care of
        CommEvent * mine = events->value(*entity)->first();
        CommEvent * theirs = other->events->value(*entity)->first();
        if (!by_pe.contains(mine->pe))
            by_pe.insert(mine->pe, new QList<CommEvent *>());
        if (!by_pe.contains(theirs->pe))
            by_pe.insert(theirs->pe, new QList<CommEvent *>());

        by_pe.value(mine->pe)->append(mine);
        by_pe.value(theirs->pe)->append(theirs);

        mine_first = false;
        theirs_first = false;
        if (!mine->isReceive() || !(mine->comm_prev)
            || mine->comm_prev->partition != this)
        {
            mine_first = true;
        }
        if (!theirs->isReceive() || !(theirs->comm_prev)
            || theirs->comm_prev->partition != this)
        {
            theirs_first = true;
        }

        // We only count if at least one of ours is first
        if (mine_first && theirs_first)
        {
            (mine->enter < theirs->enter) ? me_both++ : them_both++;
        }
        else if (mine_first)
        {
            me++;
        }
        else if (theirs_first)
        {
            them++;
        }

    }

    // Well we can't do this by entity, so let's do this by pe
    int me_pe = 0, them_pe = 0;
    if (me_both == them_both)
    {
        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator pe_list = by_pe.begin();
             pe_list != by_pe.end(); ++pe_list)
        {
            qSort(pe_list.value()->begin(), pe_list.value()->end());
            if (pe_list.value()->first()->partition == this)
                me_pe++;
            else
                them_pe++;
        }
    }

    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator pe_list = by_pe.begin();
         pe_list != by_pe.end(); ++pe_list)
    {
        delete pe_list.value();
    }

    if (me_both > them_both)
        return this;
    else if (them_both > me_both)
        return other;

    if (me_pe > them_pe)
        return this;
    else if (them_pe > me_pe)
        return other;

    if (me > them)
        return this;
    else
        return other;
}

// Set up comm_next/comm_prev to be the order in the event_list
// In the future, we may change this order around based on other things.
// Note this will break the comm_next/comm_prev relationships between
// partitions, but by the time this is used it shouldn't matter.
void Partition::finalizeEntityEventOrder()
{
    CommEvent * prev;

    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        prev = NULL;
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            (*evt)->comm_prev = prev;
            if (prev)
                prev->comm_next = *evt;
            prev = *evt;
        }
        prev->comm_next = NULL;
    }
}

void Partition::receive_reorder_mpi()
{
    // Partitions always start with some sends that have no previous parent
    // We will start these and set stride.
    QMap<unsigned long, QList<CommEvent *> *> * stride_map = new QMap<unsigned long, QList<CommEvent *> *>();
    stride_map->insert(0, new QList<CommEvent *>());
    int max_stride = 0;
    int current_stride = 0;
    int my_stride = 0;
    bool sendflag = false; // we have run into a send
    CommEvent * local_evt = NULL;

    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        if (!event_list.value()->first()->isReceive())
        {
            event_list.value()->first()->stride = 0;
            event_list.value()->first()->last_stride = event_list.value()->first();
            stride_map->value(0)->append(event_list.value()->first());
        }
    }

    // We insert receives into the stride_map at their given stride;
    while (current_stride <= max_stride)
    {
        // No events at this stride.
        if (!stride_map->contains(current_stride))
        {
            current_stride++;
            continue;
        }

        // Go through all the events at this stride.
        QList<CommEvent *> * stride_events = stride_map->value(current_stride);
        for (QList<CommEvent *>::Iterator evt = stride_events->begin();
             evt != stride_events->end(); ++evt)
        {
            local_evt = (*evt)->comm_next;
            my_stride = 1;
            sendflag = false;
            if ((*evt)->isReceive())
                sendflag = true;

            // Go along recv until we find the send(s). The stride of the
            // send(s) is the max along the recv. Note this does a lot of
            // backtracking though and could take a long time.

            while (local_evt)  // here we assume the local_evt has a stride already
            {
                if (local_evt->partition == this
                    && (!sendflag || !local_evt->isReceive()))
                {
                    if (!local_evt->isReceive())
                    {
                        if (local_evt->stride < (*evt)->stride + my_stride)
                        {
                            if (local_evt->stride >= 0)
                                stride_map->value(local_evt->stride)->removeOne(local_evt);
                            else if (local_evt->stride == current_stride)
                                std::cout << "Error: Incorrect stride" << std::endl;

                            local_evt->stride = (*evt)->stride + my_stride;
                            local_evt->last_stride = *evt;
                            if (!stride_map->contains(local_evt->stride))
                                stride_map->insert(local_evt->stride,
                                                   new QList<CommEvent *>());
                            stride_map->value(local_evt->stride)->append(local_evt);

                            if (max_stride < local_evt->stride)
                                max_stride = local_evt->stride;
                        }

                        my_stride++;
                        sendflag = true;
                    }

                    local_evt = local_evt->comm_next;
                }
                else
                {
                    local_evt = NULL;
                }
            } // End looping through common caller


            // Handle recvs for this send
            if (!(*evt)->isReceive())
            {
                if (max_stride < (*evt)->stride + 1)
                    max_stride = (*evt)->stride + 1;

                (*evt)->set_reorder_strides(stride_map, 1, NULL, current_stride);
            }


        } // End looping through events at this stride

        // Update this
        current_stride++;
    } // End stride increasing

    // Now that we have strides, sort them by stride
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        qSort(event_list.value()->begin(), event_list.value()->end(),
              CommEvent::eventStrideLessThanMPI);
    }

    // Finally clean up stride_map
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator lst = stride_map->begin();
         lst != stride_map->end(); ++lst)
    {
        delete lst.value();
    }
    delete stride_map;
}

void Partition::receive_reorder()
{
    // Partitions always start with some sends that have no previous parent
    // We will start these and set stride.
    QMap<unsigned long, QList<CommEvent *> *> * stride_map = new QMap<unsigned long, QList<CommEvent *> *>();
    stride_map->insert(0, new QList<CommEvent *>());
    int max_stride = 0;
    int current_stride = 0;
    int my_stride = 0;
    CommEvent * local_evt = NULL;

    int count = 0;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        for (QList<CommEvent *>::Iterator evt = event_list.value()->begin();
             evt != event_list.value()->end(); ++evt)
        {
            count++;
            if (!(*evt)->isReceive() && ((*evt)->comm_prev == NULL
                                         || (*evt)->comm_prev->partition != this))
            {
                (*evt)->stride = 0;
                (*evt)->last_stride = (*evt);
                stride_map->value(0)->append(*evt);
            }
        }
    }

    // We insert receives into the stride_map at their given stride;
    while (current_stride <= max_stride)
    {
        // No events at this stride.
        if (!stride_map->contains(current_stride))
        {
            current_stride++;
            continue;
        }

        // Go through all the events at this stride.
        QList<CommEvent *> * stride_events = stride_map->value(current_stride);
        for (QList<CommEvent *>::Iterator evt = stride_events->begin();
             evt != stride_events->end(); ++evt)
        {
            local_evt = *evt;
            my_stride = 1;
            if (local_evt->stride != current_stride)
                continue;
            Q_ASSERT(local_evt->stride == current_stride);

            // Handle all the events under the common caller
            // (which are those that follow through comm_next in this case without
            // changing the partition)
            // This tells us the maximum stride for this entry method
            // Once we hit a send, we only continue forward for sends
            while (local_evt)  // here we assume the local_evt has a stride already
            {
                if (local_evt->comm_next && local_evt->comm_next->partition == this)
                {
                    if (local_evt->comm_next->stride < local_evt->stride + 1)
                    {
                        local_evt->comm_next->stride = local_evt->stride + 1;
                    }
                    local_evt->comm_next->last_stride = *evt;

                    if (max_stride < local_evt->stride + 1)
                        max_stride = local_evt->stride + 1;

                    my_stride++;

                    local_evt = local_evt->comm_next;
                }
                else
                {
                    local_evt = NULL;
                }
            } // End looping through common caller


            // Now that we know the max stride, we loop through again to handle recvs
            local_evt = *evt;
            while (local_evt)
            {
                if (!local_evt->isReceive()) // We have a send - update matching recvs
                {
                    if (max_stride < local_evt->stride + my_stride)
                        max_stride = local_evt->stride + my_stride;

                    local_evt->set_reorder_strides(stride_map, my_stride, NULL, current_stride);
                } // Handled send

                if (local_evt->comm_next && local_evt->comm_next->partition == this)
                {
                    local_evt = local_evt->comm_next;
                }
                else
                {
                    local_evt = NULL;
                }
            } // End looping through common caller


        } // End looping through events at this stride

        // Update this
        current_stride++;

    } // End stride increasing

    // Now that we have strides, sort them by stride
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        qSort(event_list.value()->begin(), event_list.value()->end(),
              CommEvent::eventStrideLessThan);
    }

    // Finally clean up stride_map
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator lst = stride_map->begin();
         lst != stride_map->end(); ++lst)
    {
        delete lst.value();
    }
    delete stride_map;
}

void Partition::basic_step()
{
    // Find collectives / mark as strides
    QSet<CollectiveRecord *> * collectives = new QSet<CollectiveRecord *>();
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            (*evt)->initialize_basic_strides(collectives);
        }
    }

    // Set up stride graph
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            (*evt)->update_basic_strides();
        }
    }

    // Set stride values
    int current_stride, max_stride = 0;
    QList<CollectiveRecord *> toDelete = QList<CollectiveRecord *>();
    while (collectives->size())
    {
        // Set the stride values if possible
        for (QSet<CollectiveRecord *>::Iterator cr = collectives->begin();
             cr != collectives->end(); ++cr)
        {
            current_stride = (*cr)->set_basic_strides();
            if (current_stride > max_stride)
                max_stride = current_stride;
            if (current_stride)
                toDelete.append(*cr);
        }

        // Delete ones we found
        for (QList<CollectiveRecord *>::Iterator cr = toDelete.begin();
             cr != toDelete.end(); ++cr)
        {
            collectives->remove(*cr);
        }
    }
    delete collectives;

    // Inflate P2P Events between collectives
    max_step = -1;
    int entity;
    CommEvent * evt;
    QList<unsigned long> entities = events->keys();
    QMap<unsigned long, CommEvent*> next_step = QMap<unsigned long, CommEvent*>();
    for (unsigned long i = 0; i < entities.size(); i++)
    {
        if ((*events)[entities[i]]->size() > 0) {
            next_step[entities[i]] = (*events)[entities[i]]->at(0);
        }
        else
            next_step[entities[i]] = NULL;
    }

    // Start from one since that's where our strides start
    bool move_forward, at_stride = false;
    for (int stride = 1; stride <= max_stride; stride++)
    {
        // Step the P2P Events before this stride. Note that we
        // will have to make progress slowly to ensure that
        // we respect send/recv semantics
        while (!at_stride)
        {
            at_stride = true;
            for (int i = 0; i < entities.size(); i++)
            {
                entity = entities[i];
                evt = next_step[entity];

                // We are not at_stride
                if (!(evt && evt->stride == stride))
                {
                    move_forward = true;
                    // and have all their parents taken care of
                    while (move_forward)
                    {
                        // Now we move forward as we can with these non-stride events
                        // that fall between the previous stride and i
                        if (evt && evt->stride < 0
                            && (!evt->last_stride || evt->last_stride->stride < stride)
                            && evt->next_stride && evt->next_stride->stride == stride)
                        {
                            // We can move forward also if our parents are taken care of
                            if (evt->calculate_local_step())
                            {
                                if (evt->step > max_step)
                                    max_step = evt->step;

                                if (evt->comm_next && evt->comm_next->partition == this)
                                    evt = evt->comm_next;
                                else
                                    evt = NULL;
                            }
                            else
                            {
                                move_forward = false;
                            }

                        }
                        else
                        {
                            move_forward = false;
                        }
                    }
                }

                // Save where we are
                next_step[entity] = evt;
                if (evt && evt->stride < 0)
                {
                    at_stride = false;
                }
            }
        }

        // Now we know that the stride should be at max_step + 1
        // So set all of those
        bool increaseMax = false;
        for (int i = 0; i < entities.size(); i++)
        {
            entity = entities[i];
            evt = next_step[entity];

            if (evt && evt->stride == stride)
            {
                evt->step = max_step + 1;
                if (evt->comm_next && evt->comm_next->partition == this)
                    next_step[entity] = evt->comm_next;
                else
                    next_step[entity] = NULL;

                increaseMax = true;
            }
        }
        if (increaseMax)
            max_step++;
    }

    // Now handle all of the left events
    // This could possibly be folded into the main stride loop depending
    // on how we treat the next_stride when not existent.
    bool not_done = true;
    while (not_done)
    {
        not_done = false;
        for (int i = 0; i < entities.size(); i++)
        {
            entity = entities[i];
            evt = next_step[entity];

            move_forward = true;
            // and have all their parents taken care of
            while (move_forward)
            {
                // Now we move forward as we can with these non-stride events
                // that fall between the previous stride and i
                if (evt && evt->partition == this)
                {
                    // We can move forward also if our parents are taken care of
                    if (evt->calculate_local_step())
                    {
                        if (evt->step > max_step)
                            max_step = evt->step;

                        if (evt->comm_next && evt->comm_next->partition == this)
                            evt = evt->comm_next;
                        else
                            evt = NULL;
                    }
                    else
                    {
                        move_forward = false;
                    }

                }
                else
                {
                    move_forward = false;
                }
            }
            // Save where we are
            next_step[entity] = evt;
            // We still need to keep going through this
            if (evt && evt->partition == this)
                not_done = true;
        }
    }

    // Now that we have finished, we should also have a correct max_step
    // for this entity.
}

void Partition::step()
{
    // Build send+collective graph
    // Send dependencies go right through their receives until they find a send
    // Collectives are dependent to the rest of the collective set
    // We use stride_parents & stride_children to create this graph
    // We set up by looking for children only and having the parents set
    // the children links

    QList<CommEvent *> * stride_events = new QList<CommEvent *>();
    QList<CommEvent *> * recv_events = new QList<CommEvent *>();
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            (*evt)->initialize_strides(stride_events, recv_events);
        }
    }

    // Set strides
    int max_stride = set_stride_dag(stride_events);
    delete stride_events;

    //. Find recv stride boundaries based on dependencies
    for (QList<CommEvent *>::Iterator recv = recv_events->begin();
         recv != recv_events->end(); ++recv)
    {
        (*recv)->update_strides();
    }
    delete recv_events;

    // Inflate receives (need not check them for happened-before as their
    // dependencies are built into the stride graph).
    // This may be somewhat similar to restep/finalize... but slightly
    // different so look into that.
    max_step = -1;
    unsigned long entity;
    CommEvent * evt;
    QList<unsigned long> entities = events->keys();
    QMap<unsigned long, CommEvent*> next_step = QMap<unsigned long, CommEvent*>();
    for (int i = 0; i < entities.size(); i++)
    {
        if ((*events)[entities[i]]->size() > 0) {
            next_step[entities[i]] = (*events)[entities[i]]->at(0);
        }
        else
            next_step[entities[i]] = NULL;
    }

    for (int stride = 0; stride <= max_stride; stride++)
    {
        // Step the sends that come before this stride
        for (int i = 0; i < entities.size(); i++)
        {
            entity = entities[i];
            evt = next_step[entity];

            // We want recvs that can be set at this stride and are blocking
            // the current send strides from being sent. That means the
            // last_stride has a stride less than this one and
            // if that the next_stride exists and is this one (otherwise
            // it wouldn't be blocking the step procedure)
            // For recvs, last_stride must exist
            while (evt && evt->stride < 0
                   && evt->last_stride->stride < stride
                   && evt->next_stride && evt->next_stride->stride == stride)
            {
                if (evt->comm_prev && evt->comm_prev->partition == this)
                    // It has to go after its previous event but it also
                    // has to go after any of its sends. The maximum
                    // step of any of its sends will be in last_stride.
                    // (If last_stride is its entity-previous, then
                    // it will be covered by comm_prev).
                    evt->step = 1 + std::max(evt->comm_prev->step,
                                              evt->last_stride->step);
                else
                    evt->step = 1 + evt->last_stride->step;

                if (evt->step > max_step)
                    max_step = evt->step;

                if (evt->comm_next && evt->comm_next->partition == this)
                    evt = evt->comm_next;
                else
                    evt = NULL;
            }

            // Save where we are
            next_step[entity] = evt;
        }

        // Now we know that the stride should be at max_step + 1
        // So set all of those
        bool increaseMax = false;
        for (int i = 0; i < entities.size(); i++)
        {
            entity = entities[i];
            evt = next_step[entity];

            if (evt && evt->stride == stride)
            {
                evt->step = max_step + 1;
                if (evt->comm_next && evt->comm_next->partition == this)
                    next_step[entity] = evt->comm_next;
                else
                    next_step[entity] = NULL;

                increaseMax = true;
            }
        }
        if (increaseMax)
            max_step++;
    }

    // Now handle all of the left over recvs
    for (unsigned long i = 0; i < entities.size(); i++)
    {
        entity = entities[i];
        evt = next_step[entity];

        // We only want things in the current partition
        while (evt && evt->partition == this)
        {
            if (evt->comm_prev && evt->comm_prev->partition == this)
                evt->step = 1 + std::max(evt->comm_prev->step,
                                          evt->last_stride->step);
            else
                evt->step = 1 + evt->last_stride->step;

            if (evt->step > max_step)
                max_step = evt->step;
            evt = evt->comm_next;
        }
    }

    // Now that we have finished, we should also have a correct max_step
    // for this entity.
}

int Partition::set_stride_dag(QList<CommEvent *> * stride_events)
{
    QSet<CommEvent *> * current_events = new QSet<CommEvent*>();
    QSet<CommEvent *> * next_events = new QSet<CommEvent *>();
    // Find first stride
    for (QList<CommEvent *>::Iterator evt = stride_events->begin();
         evt != stride_events->end(); ++evt)
    {
        if ((*evt)->stride_parents->isEmpty())
        {
            (*evt)->stride = 0;
            for (QSet<CommEvent *>::Iterator child = (*evt)->stride_children->begin();
                 child != (*evt)->stride_children->end(); ++child)
            {
                current_events->insert(*child);
            }
        }
    }

    bool parentFlag;
    int stride;
    int max_stride = 0;
    while (!current_events->isEmpty())
    {
        for (QSet<CommEvent *>::Iterator evt = current_events->begin();
             evt != current_events->end(); ++evt)
        {
            parentFlag = true;
            stride = -1;
            for (QSet<CommEvent *>::Iterator parent = (*evt)->stride_parents->begin();
                 parent != (*evt)->stride_parents->end(); ++parent)
            {
                if ((*parent)->stride < 0)
                {
                    next_events->insert(*parent);
                    parentFlag = false;
                    break;
                }
                else
                {
                    stride = std::max(stride, (*parent)->stride);
                }
            }

            if (!parentFlag)
                continue;

            (*evt)->stride = stride + 1; // 1 over the max parent
            if ((*evt)->stride > max_stride)
                max_stride = (*evt)->stride;

            // Add children to next_events
            for (QSet<CommEvent *>::Iterator child = (*evt)->stride_children->begin();
                 child != (*evt)->stride_children->end(); ++child)
            {
                if ((*child)->stride < 0)
                    next_events->insert(*child);
            }
        }

        delete current_events;
        current_events = next_events;
        next_events = new QSet<CommEvent *>();
    }

    delete current_events;
    delete next_events;
    return max_stride;
}

// Calculate both imbalance-based metrics
// These measure the difference in time taken on the PEs for the partition
void Partition::calculate_imbalance(int num_pes)
{
    QList<unsigned long long> durations = QList<unsigned long long>();
    for (int i = 0; i < num_pes; i++)
        durations.append(0);

    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            durations[(*evt)->pe] += (*evt)->extent_end - (*evt)->extent_begin;
        }
    }

    unsigned long long min_duration = ULLONG_MAX;
    unsigned long long max_duration = 0;
    for (int i = 0; i < num_pes; i++)
    {
        if (durations[i] < min_duration)
            min_duration = durations[i];
        if (durations[i] > max_duration)
            max_duration = durations[i];
    }

    unsigned long long imbalance = max_duration - min_duration;
    metrics->addMetric("Imbalance", imbalance, imbalance);

    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            imbalance = durations[(*evt)->pe] - min_duration;
            (*evt)->metrics->addMetric("PE Imbalance", imbalance, imbalance);
        }
    }
}

void Partition::makeClusterVectors(QString metric)
{
    // Clean up old
    for (QMap<int, QVector<long long int> *>::Iterator itr =  cluster_vectors->begin();
         itr != cluster_vectors->end(); ++itr)
    {
        delete itr.value();
    }
    for (QVector<ClusterEntity *>::Iterator itr
         = cluster_entities->begin(); itr != cluster_entities->end(); ++itr)
    {
        delete *itr;
    }
    cluster_entities->clear();
    cluster_vectors->clear();
    cluster_step_starts->clear();


    // Create a ClusterEntity for each entity and in each set metric_events
    // so it fills in the missing steps with the previous metric value.
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        QVector<long long int> * metric_vector = new QVector<long long int>();
        (*cluster_vectors)[event_list.key()] = metric_vector;
        long long int last_value = 0;
        int last_step = (event_list.value())->at(0)->step;
        (*cluster_step_starts)[event_list.key()] = last_step;
        ClusterEntity * cp = new ClusterEntity(event_list.key(), last_step);
        cluster_entities->append(cp);
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            while ((*evt)->step > last_step + 2)
            {
                // Fill in the previous known value
                metric_vector->append(last_value);
                cp->metric_events->append(last_value);
                last_step += 2;
            }

            // Fill in our value
            last_step = (*evt)->step;
            last_value = (*evt)->getMetric(metric);
            metric_vector->append(last_value);
            cp->metric_events->append(last_value);
        }
        while (last_step <= max_global_step)
        {
            // We're out of steps but fill in the rest
            metric_vector->append(last_value);
            cp->metric_events->append(last_value);
            last_step += 2;
        }
    }
}

// String giving process IDs involved in this partition
QString Partition::generate_process_string()
{
    QString ps = "";
    bool first = true;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator itr = events->begin();
         itr != events->end(); ++itr)
    {
        if (!first)
            ps += ", ";
        else
            first = false;
        ps += QString::number(itr.key());
    }
    return ps;
}


int Partition::num_events()
{
    int count = 0;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator itr = events->begin();
         itr != events->end(); ++itr)
        count += (*itr)->length();
    return count;
}

// Find what partition this one has been merged into
Partition * Partition::newest_partition()
{
    Partition * p = this;
    while (p != p->new_partition)
        p = p->new_partition;
    return p;
}

// Verify each event in the partition actually belongs to the partition
bool Partition::verify_members()
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            if ((*evt)->partition != this)
            {
                return false;
            }
        }
    }
    return true;
}

// Verify that if the partition has runtime events, it is a runtime partition
bool Partition::verify_runtime(int runtime_id)
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        if (evtlist.key() >= runtime_id && !runtime)
            return false;
    }
    return true;
}

// Verify partition is not its own parent
bool Partition::verify_parents()
{
    for (QSet<Partition *>::Iterator parent = parents->begin();
         parent != parents->end(); ++parent)
    {
        if (*parent == this)
            return false;
    }
    return true;
}

QString Partition::get_callers(QMap<int, Function *> * functions)
{
    QSet<QString> callers = QSet<QString>();
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            callers.insert(functions->value((*evt)->caller->function)->name);
        }
    }

    QList<QString> caller_list = callers.toList();
    qSort(caller_list);
    QString str = "";
    for (QList<QString>::Iterator caller = caller_list.begin();
         caller != caller_list.end(); ++caller)
    {
        str += "\n ";
        str += *caller;
    }
    return str;
}

// use GraphViz to see partition graph for debugging
void Partition::output_graph(QString filename, Trace * trace)
{
    std::ofstream graph;
    graph.open(filename.toStdString().c_str());
    std::ofstream graph2;
    QString graph2string = filename + ".eventlist";
    graph2.open(graph2string.toStdString().c_str());

    QString indent = "     ";

    graph << "digraph {\n";
    graph << indent.toStdString().c_str() << "graph [bgcolor=transparent];\n";
    graph << indent.toStdString().c_str() << "node [label=\"\\N\"];\n";

    graph2 << "digraph {\n";
    graph2 << indent.toStdString().c_str() << "graph [bgcolor=transparent];\n";
    graph2 << indent.toStdString().c_str() << "node [label=\"\\N\"];\n";

    QMap<int, QString> entities = QMap<int, QString>();

    int id = 0;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        entities.insert(evtlist.key(), QString::number(id));
        graph2 << indent.toStdString().c_str() << entities.value(evtlist.key()).toStdString().c_str();
        graph2 << " [label=\"";
        graph2 << "t=" << QString::number(evtlist.key()).toStdString().c_str();\
        graph2 << "\"];\n";
        id++;
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            (*evt)->gvid = QString::number(id);
            graph << indent.toStdString().c_str() << (*evt)->gvid.toStdString().c_str();
            graph << " [label=\"";
            graph << "t=" << QString::number((*evt)->entity).toStdString().c_str();\
            graph << ", p=" << QString::number((*evt)->pe).toStdString().c_str();
            graph << ", s: " << QString::number((*evt)->step).toStdString().c_str();
            graph << ", e: " << QString::number((*evt)->exit).toStdString().c_str();
            graph << ", a: " << QString::number((*evt)->add_order).toStdString().c_str();
            if ((*evt)->caller)
                graph << "\n " << trace->functions->value((*evt)->caller->function)->name.toStdString().c_str();
            graph << "\"];\n";

            graph2 << indent.toStdString().c_str() << (*evt)->gvid.toStdString().c_str();
            graph2 << " [label=\"";
            graph2 << "t=" << QString::number((*evt)->entity).toStdString().c_str();\
            graph2 << ", p=" << QString::number((*evt)->pe).toStdString().c_str();
            graph2 << ", s: " << QString::number((*evt)->step).toStdString().c_str();
            graph2 << ", e: " << QString::number((*evt)->exit).toStdString().c_str();
            graph2 << ", a: " << QString::number((*evt)->add_order).toStdString().c_str();
            if ((*evt)->caller)
                graph2 << "\n " << trace->functions->value((*evt)->caller->function)->name.toStdString().c_str();
            graph2 << "\"];\n";
            ++id;
        }
    }


    CommEvent * prev = NULL;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        prev = NULL;
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            if (prev)
            {
                graph2 << indent.toStdString().c_str() << prev->gvid.toStdString().c_str();
                graph2 << " -> " << (*evt)->gvid.toStdString().c_str() << ";\n";
            }
            else
            {
                graph2 << indent.toStdString().c_str() << entities.value(evtlist.key()).toStdString().c_str();
                graph2 << " -> " << (*evt)->gvid.toStdString().c_str() << ";\n";
            }
            if ((*evt)->comm_next && (*evt)->comm_next->partition == this)
            {
                graph << "edge [color=red];\n";
                graph << indent.toStdString().c_str() << (*evt)->gvid.toStdString().c_str();
                graph << " -> " << (*evt)->comm_next->gvid.toStdString().c_str() << ";\n";
            }
            if ((*evt)->isP2P() && !(*evt)->isReceive())
            {
                QVector<Message *> * messages = (*evt)->getMessages();
                for (QVector<Message *>::Iterator msg = messages->begin();
                     msg != messages->end(); ++msg)
                {
                    graph << "edge [color=black];\n";
                    graph << indent.toStdString().c_str() << (*evt)->gvid.toStdString().c_str();
                    graph << " -> " << (*msg)->receiver->gvid.toStdString().c_str() << ";\n";
                }
            }
            prev = *evt;
        }
    }

    graph << "}";
    graph.close();

    graph2 << "}";
    graph2.close();
}
