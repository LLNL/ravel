#include "rpartition.h"
#include <iostream>
#include <climits>

#include "event.h"
#include "commevent.h"
#include "collectiverecord.h"
#include "clustertask.h"
#include "general_util.h"

Partition::Partition()
    : events(new QMap<int, QList<CommEvent *> *>),
      max_step(-1),
      max_global_step(-1),
      min_global_step(-1),
      dag_leap(-1),
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
