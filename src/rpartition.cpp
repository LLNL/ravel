#include "rpartition.h"
#include <iostream>
#include <fstream>
#include <climits>

#include "event.h"
#include "commevent.h"
#include "collectiverecord.h"
#include "clustertask.h"
#include "general_util.h"
#include "message.h"
#include "p2pevent.h"

Partition::Partition()
    : events(new QMap<int, QList<CommEvent *> *>),
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
      group(new QSet<Partition *>()),
      gvid(""),
      gnome(NULL),
      gnome_type(0),
      cluster_tasks(new QVector<ClusterTask *>()),
      cluster_vectors(new QMap<int, QVector<long long int> *>()),
      cluster_step_starts(new QMap<int, int>()),
      debug_mark(false),
      debug_name(-1),
      free_recvs(NULL)
{
    group->insert(this); // We are always in our own group
}

Partition::~Partition()
{
    for (QMap<int, QList<CommEvent *> *>::Iterator eitr = events->begin();
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
    for (QMap<int, QList<CommEvent *> *>::Iterator eitr = events->begin();
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
    if (events->contains(e->task))
    {
        ((*events)[e->task])->append(e);
    }
    else
    {
        (*events)[e->task] = new QList<CommEvent *>();
        ((*events)[e->task])->append(e);
    }
}

void Partition::sortEvents(){
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        qSort((event_list.value())->begin(), (event_list.value())->end(),
              dereferencedLessThan<CommEvent>);
    }
}


// The minimum over all tasks of the time difference between the last event
// in one partition and the first event in another, per task
unsigned long long int Partition::distance(Partition * other)
{
    unsigned long long int dist = ULLONG_MAX;
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
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
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        /*if (event_list.value()->first()->comm_prev)
            parents->insert(event_list.value()->first()->comm_prev->partition);
        if (event_list.value()->last()->comm_next)
            children->insert(event_list.value()->first()->comm_next->partition);*/
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

Event * Partition::least_common_caller(int taskid, QMap<Event *, int> * memo)
{
    QList<CommEvent *> * evts = events->value(taskid);
    if (evts->size() > 1)
    {
        Event * evt1, * evt2;
        evt1 = evts->first();
        for (int i = 1; i < evts->size(); i++)
        {
            evt2 = evts->at(i);
            evt1 = evt1->least_common_caller(evt2);
            if (!evt1)
               break;
        }
        return evt1;
    }
    else
    {
        evts->first()->least_multiple_caller(memo);
    }
}


// True the partition... anything we a true_next that is not us we will set as
// our child partition.
void Partition::true_children()
{
    for (QMap<int, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            if ((*evt)->true_next && (*evt)->true_next->partition != this)
            {
                children->insert((*evt)->true_next->partition);
                Partition * p = (*evt)->true_next->partition;
                (*evt)->true_next->partition->parents->insert(this);
            }
        }
    }
}

bool Partition::mergable(Partition * other)
{
    if (runtime != other->runtime)
        return false;

    bool overlap = false;
    for (QMap<int, QList<CommEvent *> *>::Iterator tasklist = events->begin();
         tasklist != events->end(); ++tasklist)
    {
        if (other->events->contains(tasklist.key()))
            overlap = true;
    }
    return overlap;
}

QSet<int> Partition::task_overlap(Partition * other)
{
    QSet<int> overlap = QSet<int>();
    for (QMap<int, QList<CommEvent *> *>::Iterator tasklist = events->begin();
         tasklist != events->end(); ++tasklist)
    {
        // We can't compare
        if (other->events->contains(tasklist.key()))
            overlap.insert(tasklist.key());
    }

    return overlap;
}

// Figure out which partition comes before the other. This
// assumes that there is at least one task of overlap.
// If we can find a caller (comm_next/comm_prev) ordering,
// we will use that. Otherwise, we will take a vote of
// which has the earliest earlier events per task.
// Presumable we won't have the latter happening since that
// would be set up in order by earlier stuff and hopefully
// any cycles would have been found their earlier as well.
Partition * Partition::earlier_partition(Partition * other, QSet<int> overlap_tasks)
{
    // Counts for which one has the earlier earliest event
    int me = 0, them = 0;

    for (QSet<int>::Iterator task = overlap_tasks.begin();
         task != overlap_tasks.end(); ++task)
    {
        // Now let's just do the voting and avoid the comm/prev/next
        // thing for now because we believe it already taken care of
        if (events->value(*task)->first()->enter
            < other->events->value(*task)->first()->enter)
        {
            me++;
        }
        else
        {
            them++;
        }
    }

    if (me > them)
        return this;
    else
        return other;
}

// Set up comm_next/comm_prev to be the order in the event_list
// In the future, we may change this order around based on other things.
// Note this will break the comm_next/comm_prev relationships between
// partitions, but by the time this is used it shouldn't matter.
void Partition::finalizeTaskEventOrder()
{
    CommEvent * prev;
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        prev = NULL;
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            (*evt)->stride = -1;
            (*evt)->last_stride = NULL;
            (*evt)->comm_prev = prev;
            if (prev)
                prev->comm_next = *evt;
            prev = *evt;
        }
    }
}

void Partition::receive_reorder_mpi()
{
    // Partitions always start with some sends that have no previous parent
    // We will start these and set stride.
    QMap<int, QList<CommEvent *> *> * stride_map = new QMap<int, QList<CommEvent *> *>();
    stride_map->insert(0, new QList<CommEvent *>());
    int max_stride = 0;
    int current_stride = 0;
    int my_stride = 0;
    bool sendflag = false; // we have run into a send
    CommEvent * local_evt = NULL;

    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
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
            local_evt = *evt;
            my_stride = 1;
            sendflag = false;
            if ((*evt)->isReceive())
                sendflag = true;

            // Go along recv until we find the send(s). The stride of the
            // send(s) is the max along the recv. Note this does a lot of
            // backtracking though and could take a long time.

            while (local_evt)  // here we assume the local_evt has a stride already
            {
                if (local_evt->comm_next && local_evt->comm_next->partition == this
                        && (!sendflag || !local_evt->comm_next->isReceive()))
                {
                    if (!local_evt->comm_next->isReceive())
                    {
                        if (local_evt->comm_next->stride < (*evt)->stride + my_stride)
                        {
                            if (local_evt->comm_next->stride >= 0)
                                stride_map->value(local_evt->comm_next->stride)->removeOne(local_evt->comm_next);
                            local_evt->comm_next->stride = (*evt)->stride + my_stride;
                            local_evt->comm_next->last_stride = *evt;
                            if (!stride_map->contains(local_evt->comm_next->stride))
                                stride_map->insert(local_evt->comm_next->stride,
                                                   new QList<CommEvent *>());
                            stride_map->value(local_evt->comm_next->stride)->append(local_evt->comm_next);

                            if (max_stride < local_evt->comm_next->stride)
                                max_stride = local_evt->comm_next->stride;
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

                (*evt)->set_reorder_strides(stride_map, 1);
            }


        } // End looping through events at this stride

        // Update this
        current_stride++;

    } // End stride increasing

    // Now that we have strides, sort them by stride
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        qSort(event_list.value()->begin(), event_list.value()->end(),
              eventStrideLessThan);
    }

    // Finally clean up stride_map
    for (QMap<int, QList<CommEvent *> *>::Iterator lst = stride_map->begin();
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
    QMap<int, QList<CommEvent *> *> * stride_map = new QMap<int, QList<CommEvent *> *>();
    stride_map->insert(0, new QList<CommEvent *>());
    int max_stride = 0;
    int current_stride = 0;
    int my_stride = 0;
    CommEvent * local_evt = NULL;

    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
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
            local_evt = *evt;
            my_stride = 1;

            // Handle all the events under the common caller
            // (which are those that follow through comm_next in this case without
            // changing the partition)
            // This tells us the maximum stride for this entry method
            // Once we hit a send, we only continue forward for sends
            while (local_evt)  // here we assume the local_evt has a stride already
            {
                if (local_evt->comm_next && local_evt->comm_next->partition == this)
                {
                    local_evt->comm_next->stride = local_evt->stride + 1;
                    local_evt->comm_next->last_stride = local_evt;

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

                    local_evt->set_reorder_strides(stride_map, my_stride);
                } // Handled send

                if (local_evt->comm_next)
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
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        qSort(event_list.value()->begin(), event_list.value()->end(),
              eventStrideLessThan);
    }

    // Finally clean up stride_map
    for (QMap<int, QList<CommEvent *> *>::Iterator lst = stride_map->begin();
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
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            (*evt)->initialize_basic_strides(collectives);
        }
    }

    // Set up stride graph
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
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
    int task;
    CommEvent * evt;
    QList<int> tasks = events->keys();
    QMap<int, CommEvent*> next_step = QMap<int, CommEvent*>();
    for (int i = 0; i < tasks.size(); i++)
    {
        if ((*events)[tasks[i]]->size() > 0) {
            next_step[tasks[i]] = (*events)[tasks[i]]->at(0);
        }
        else
            next_step[tasks[i]] = NULL;
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
            for (int i = 0; i < tasks.size(); i++)
            {
                task = tasks[i];
                evt = next_step[task];

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
                next_step[task] = evt;
                if (evt && evt->stride < 0)
                {
                    at_stride = false;
                }
            }
        }

        // Now we know that the stride should be at max_step + 1
        // So set all of those
        bool increaseMax = false;
        for (int i = 0; i < tasks.size(); i++)
        {
            task = tasks[i];
            evt = next_step[task];

            if (evt && evt->stride == stride)
            {
                evt->step = max_step + 1;
                if (evt->comm_next && evt->comm_next->partition == this)
                    next_step[task] = evt->comm_next;
                else
                    next_step[task] = NULL;

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
        for (int i = 0; i < tasks.size(); i++)
        {
            task = tasks[i];
            evt = next_step[task];

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
            next_step[task] = evt;
            // We still need to keep going through this
            if (evt)
                not_done = true;
        }
    }

    // Now that we have finished, we should also have a correct max_step
    // for this task.
}

void Partition::step()
{
    // Build send+collective graph
    // Send dependencies go right through their receives until they find a send
    // Collectives are dependent to the rest of the collective set
    // We use stride_parents & stride_children to create this graph
    // We set up by looking for children only and having the parents set
    // the children links

    //std::cout << "   Init strides" << std::endl;
    QList<CommEvent *> * stride_events = new QList<CommEvent *>();
    QList<CommEvent *> * recv_events = new QList<CommEvent *>();
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            (*evt)->initialize_strides(stride_events, recv_events);
        }
    }

    //std::cout << "   Build stride graph" << std::endl;
    // Set strides
    int max_stride = set_stride_dag(stride_events);
    delete stride_events;

    //std::cout << "   Update stride boundaries" << std::endl;
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
    //std::cout << "   Inflate receives" << std::endl;
    max_step = -1;
    int task;
    CommEvent * evt;
    QList<int> tasks = events->keys();
    QMap<int, CommEvent*> next_step = QMap<int, CommEvent*>();
    for (int i = 0; i < tasks.size(); i++)
    {
        if ((*events)[tasks[i]]->size() > 0) {
            next_step[tasks[i]] = (*events)[tasks[i]]->at(0);
        }
        else
            next_step[tasks[i]] = NULL;
    }

    for (int stride = 0; stride <= max_stride; stride++)
    {
        // Step the sends that come before this stride
        for (int i = 0; i < tasks.size(); i++)
        {
            task = tasks[i];
            evt = next_step[task];

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
                    // (If last_stride is its task-previous, then
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
            next_step[task] = evt;
        }

        // Now we know that the stride should be at max_step + 1
        // So set all of those
        bool increaseMax = false;
        for (int i = 0; i < tasks.size(); i++)
        {
            task = tasks[i];
            evt = next_step[task];

            if (evt && evt->stride == stride)
            {
                evt->step = max_step + 1;
                if (evt->comm_next && evt->comm_next->partition == this)
                    next_step[task] = evt->comm_next;
                else
                    next_step[task] = NULL;

                increaseMax = true;
            }
        }
        if (increaseMax)
            max_step++;
    }

    // Now handle all of the left over recvs
    for (int i = 0; i < tasks.size(); i++)
    {
        task = tasks[i];
        evt = next_step[task];

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
    // for this task.
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

void Partition::makeClusterVectors(QString metric)
{
    // Clean up old
    for (QMap<int, QVector<long long int> *>::Iterator itr =  cluster_vectors->begin();
         itr != cluster_vectors->end(); ++itr)
    {
        delete itr.value();
    }
    for (QVector<ClusterTask *>::Iterator itr
         = cluster_tasks->begin(); itr != cluster_tasks->end(); ++itr)
    {
        delete *itr;
    }
    cluster_tasks->clear();
    cluster_vectors->clear();
    cluster_step_starts->clear();


    // Create a ClusterTask for each task and in each set metric_events
    // so it fills in the missing steps with the previous metric value.
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        QVector<long long int> * metric_vector = new QVector<long long int>();
        (*cluster_vectors)[event_list.key()] = metric_vector;
        long long int last_value = 0;
        int last_step = (event_list.value())->at(0)->step;
        (*cluster_step_starts)[event_list.key()] = last_step;
        ClusterTask * cp = new ClusterTask(event_list.key(), last_step);
        cluster_tasks->append(cp);
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

QString Partition::generate_process_string()
{
    QString ps = "";
    bool first = true;
    for (QMap<int, QList<CommEvent *> *>::Iterator itr = events->begin();
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
    for (QMap<int, QList<CommEvent *> *>::Iterator itr = events->begin();
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

// use GraphViz to see partition graph for debugging
void Partition::output_graph(QString filename)
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

    QMap<int, QString> tasks = QMap<int, QString>();

    int id = 0;
    for (QMap<int, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        tasks.insert(evtlist.key(), QString::number(id));
        graph2 << indent.toStdString().c_str() << tasks.value(evtlist.key()).toStdString().c_str();
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
            graph << "t=" << QString::number((*evt)->task).toStdString().c_str();\
            graph << ", p=" << QString::number((*evt)->pe).toStdString().c_str();
            graph << ", s: " << QString::number((*evt)->step).toStdString().c_str();
            graph << ", e: " << QString::number((*evt)->exit).toStdString().c_str();
            graph << "\"];\n";

            graph2 << indent.toStdString().c_str() << (*evt)->gvid.toStdString().c_str();
            graph2 << " [label=\"";
            graph2 << "t=" << QString::number((*evt)->task).toStdString().c_str();\
            graph2 << ", p=" << QString::number((*evt)->pe).toStdString().c_str();
            graph2 << ", s: " << QString::number((*evt)->step).toStdString().c_str();
            graph2 << ", e: " << QString::number((*evt)->exit).toStdString().c_str();
            graph2 << "\"];\n";
            ++id;
        }
    }


    CommEvent * prev = NULL;
    for (QMap<int, QList<CommEvent *> *>::Iterator evtlist = events->begin();
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
                graph2 << indent.toStdString().c_str() << tasks.value(evtlist.key()).toStdString().c_str();
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
