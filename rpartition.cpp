#include "rpartition.h"
#include <iostream>

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
      cluster_processes(new QVector<ClusterProcess *>()),
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
    if (events->contains(e->process))
    {
        ((*events)[e->process])->append(e);
    }
    else
    {
        (*events)[e->process] = new QList<CommEvent *>();
        ((*events)[e->process])->append(e);
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


// The minimum over all processes of the time difference between the last event
// in one partition and the first event in another, per process
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
            if (!(*evt)->isReceive())
            {
                stride_events->append(*evt);

                // The next one in the process is a stride child
                find_stride_child(*evt, *evt);

                // Follow messages to their receives and then along
                // the new process to find more stride children
                QVector<Message *> *msgs = (*evt)->getMessages();
                if (msgs && !(*evt)->isReceive())
                    for (QVector<Message *>::Iterator msg = msgs->begin();
                         msg != msgs->end(); ++msg)
                    {
                        find_stride_child(*evt, (*msg)->receiver);
                    }
            }
            else // Setup receives
            {
                recv_events->append(*evt);
                if ((*evt)->comm_prev && (*evt)->comm_prev->partition == this)
                    (*evt)->last_send = (*evt)->comm_prev;
                // Set last_send based on process
                while ((*evt)->last_send && (*evt)->isReceive())
                {
                    (*evt)->last_send = (*evt)->last_send->comm_prev;
                }
                if ((*evt)->last_send && (*evt)->last_send->partition != this)
                    (*evt)->last_send = NULL;

                (*evt)->next_send = (*evt)->comm_next;
                // Set next_send based on process
                while ((*evt)->next_send && (*evt)->isReceive())
                {
                    (*evt)->next_send = (*evt)->next_send->comm_next;
                }
                if ((*evt)->next_send && (*evt)->next_send->partition != this)
                    (*evt)->next_send = NULL;
            }
        }
    }

    // Set strides
    int max_stride = set_stride_dag(stride_events);
    delete stride_events;

    //. Find recv stride boundaries based on dependencies
    for (QList<CommEvent *>::Iterator recv = recv_events->begin();
         recv != recv_events->end(); ++recv)
    {
        // Iterate through sends of this recv and check what
        // their strides are to update last_send and next_send
        // to be the tightest boundaries.
        QVector<Message *> * msgs = (*recv)->getMessages();
        if (msgs)
            for (QVector<Message *>::Iterator msg = msgs->begin();
                 msg != msgs->end(); ++msg)
            {
                if (!(*recv)->last_send
                        || (*msg)->sender->stride > (*recv)->last_send->stride)
                {
                    (*recv)->last_send = (*msg)->sender;
                }
            }
    }
    delete recv_events;

    // Inflate receives (need not check them for happened-before as their
    // dependencies are built into the stride graph).
    // This may be somewhat similar to restep/finalize... but slightly
    // different so look into that.
    max_step = -1;
    int process;
    CommEvent * evt;
    QList<int> processes = events->keys();
    QMap<int, CommEvent*> next_step = QMap<int, CommEvent*>();
    for (int i = 0; i < processes.size(); i++)
    {
        if ((*events)[processes[i]]->size() > 0) {
            next_step[processes[i]] = (*events)[processes[i]]->at(0);
        }
        else
            next_step[processes[i]] = NULL;
    }

    for (int stride = 0; stride <= max_stride; stride++)
    {
        // Step the sends that come before this stride
        for (int i = 0; i < processes.size(); i++)
        {
            process = processes[i];
            evt = next_step[process];

            // We want recvs that can be set at this stride and are blocking
            // the current send strides from being sent. That means the
            // last_send has a stride less than this one and
            // if that the next_send exists and is this one (otherwise
            // it wouldn't be blocking the step procedure)
            // For recvs, last_send must exist
            while (evt && evt->stride < 0
                   && evt->last_send->stride < stride
                   && evt->next_send && evt->next_send->stride == stride)
            {
                if (evt->comm_prev && evt->comm_prev->partition == this)
                    // It has to go after its previous event but it also
                    // has to go after any of its sends. The maximum
                    // step of any of its sends will be in last_send.
                    // (If last_send is its process-previous, then
                    // it will be covered by comm_prev).
                    evt->step = 1 + std::max(evt->comm_prev->step,
                                              evt->last_send->step);
                else
                    evt->step = 1 + evt->last_send->step;

                if (evt->step > max_step)
                    max_step = evt->step;

                if (evt->comm_next && evt->comm_next->partition == this)
                    evt = evt->comm_next;
                else
                    evt = NULL;
            }

            // Save where we are
            next_step[process] = evt;
        }

        // Now we know that the stride should be at max_step + 1
        // So set all of those
        bool increaseMax = false;
        for (int i = 0; i < processes.size(); i++)
        {
            process = processes[i];
            evt = next_step[process];

            if (evt && evt->stride == stride)
            {
                evt->step = max_step + 1;
                if (evt->comm_next && evt->comm_next->partition == this)
                    next_step[process] = evt->comm_next;
                else
                    next_step[process] = NULL;

                increaseMax = true;
            }
        }
        if (increaseMax)
            max_step++;
    }

    // Now handle all of the left over recvs
    for (int i = 0; i < processes.size(); i++)
    {
        process = processes[i];
        evt = next_step[process];

        // We only want things in the current partition
        while (evt && evt->partition == this)
        {
            if (evt->comm_prev && evt->comm_prev->partition == this)
                evt->step = 1 + std::max(evt->comm_prev->step,
                                          evt->last_send->step);
            else
                evt->step = 1 + evt->last_send->step;

            if (evt->step > max_step)
                max_step = evt->step;
            evt = evt->comm_next;
        }
    }

    // Now that we have finished, we should also have a correct max_step
    // for this process.
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

// Helper function for building stride graph, finds the next send
// or collective event along a process from the parameter evt
void Partition::find_stride_child(CommEvent *base, CommEvent *evt)
{
    CommEvent * process_next = evt->comm_next;

    // while we have receives
    while (process_next && process_next->isReceive())
    {
        process_next = process_next->comm_next;
    }

    if (process_next && process_next->partition == this)
    {
        process_next->set_stride_relationships(base);
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
    for (QVector<ClusterProcess *>::Iterator itr
         = cluster_processes->begin(); itr != cluster_processes->end(); ++itr)
    {
        delete *itr;
    }
    cluster_processes->clear();
    cluster_vectors->clear();
    cluster_step_starts->clear();


    // Create a ClusterProcess for each process and in each set metric_events
    // so it fills in the missing steps with the previous metric value.
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        QVector<long long int> * metric_vector = new QVector<long long int>();
        (*cluster_vectors)[event_list.key()] = metric_vector;
        long long int last_value = 0;
        int last_step = (event_list.value())->at(0)->step;
        (*cluster_step_starts)[event_list.key()] = last_step;
        ClusterProcess * cp = new ClusterProcess(event_list.key(), last_step);
        cluster_processes->append(cp);
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
