#include "p2pevent.h"
#include "commbundle.h"
#include "message.h"
#include "clusterevent.h"

P2PEvent::P2PEvent(unsigned long long _enter, unsigned long long _exit,
                   int _function, int _task, int _phase,
                   QVector<Message *> *_messages)
    : CommEvent(_enter, _exit, _function, _task, _phase),
      subevents(NULL),
      messages(_messages),
      is_recv(false)
{
}

P2PEvent::P2PEvent(QList<P2PEvent *> * _subevents)
    : CommEvent(_subevents->first()->enter, _subevents->last()->exit,
                _subevents->first()->function, _subevents->first()->task,
                _subevents->first()->phase),
      subevents(_subevents),
      messages(new QVector<Message *>()),
      is_recv(_subevents->first()->is_recv)
{
    this->depth = _subevents->first()->depth;

    // Take over submessages
    for (QList<P2PEvent *>::Iterator evt = _subevents->begin();
         evt != subevents->end(); ++evt)
    {
        for (QVector<Message *>::Iterator msg = (*evt)->messages->begin();
             msg != (*evt)->messages->end(); ++msg)
        {
            if (is_recv)
                (*msg)->receiver = this;
            else
                (*msg)->sender = this;
            messages->append(*msg);
        }
    }

    // Aggregate existing metrics
    P2PEvent * first = _subevents->first();
    for (QMap<QString, MetricPair *>::Iterator counter = first->metrics->begin();
         counter != first->metrics->end(); ++ counter)
    {
        QString name = counter.key();
        unsigned long long metric = 0, agg = 0;
        for (QList<P2PEvent *>::Iterator evt = _subevents->begin();
             evt != subevents->end(); ++evt)
        {
            metric += (*evt)->getMetric(name);
            agg += (*evt)->getMetric(name, true);
        }
        addMetric(name, metric, agg);
    }
}

P2PEvent::~P2PEvent()
{
    for (QVector<Message *>::Iterator itr = messages->begin();
         itr != messages->end(); ++itr)
    {
            delete *itr;
            *itr = NULL;
    }
    delete messages;

    if (subevents)
        delete subevents;
}

bool P2PEvent::isReceive()
{
    return is_recv;
}

void P2PEvent::fixPhases()
{
    for (QVector<Message *>::Iterator msg
         = messages->begin();
         msg != messages->end(); ++msg)
    {
         if ((*msg)->sender->phase > phase)
             phase = (*msg)->sender->phase;
         else if ((*msg)->sender->phase < phase)
             (*msg)->sender->phase = phase;
         if ((*msg)->receiver->phase > phase)
            phase = (*msg)->receiver->phase;
         else if ((*msg)->receiver->phase < phase)
             (*msg)->receiver->phase = phase;
    }
}

void P2PEvent::calculate_differential_metric(QString metric_name,
                                             QString base_name)
{
    long long max_parent = getMetric(base_name, true);
    long long max_agg_parent = 0;
    if (comm_prev)
        max_agg_parent = (comm_prev->getMetric(base_name));

    if (is_recv)
    {
        for (QVector<Message *>::Iterator msg
             = messages->begin();
             msg != messages->end(); ++msg)
        {
            if ((*msg)->sender->getMetric(base_name) > max_parent)
                max_parent = (*msg)->sender->getMetric(base_name);
        }
    }

    addMetric(metric_name,
              std::max(0.,
                       getMetric(base_name)- max_parent),
              std::max(0.,
                       getMetric(base_name, true)- max_agg_parent));
}


void P2PEvent::initialize_strides(QList<CommEvent *> * stride_events,
                                         QList<CommEvent *> * recv_events)
{
    if (!is_recv)
    {
        stride_events->append(this);

        // The next one in the task is a stride child
        set_stride_relationships(this);

        // Follow messages to their receives and then along
        // the new task to find more stride children
        for (QVector<Message *>::Iterator msg = messages->begin();
             msg != messages->end(); ++msg)
        {
            set_stride_relationships((*msg)->receiver);
        }
    }
    else // Setup receives
    {
        recv_events->append(this);
        if (comm_prev && comm_prev->partition == partition)
            last_send = comm_prev;
        // Set last_send based on task
        while (last_send && last_send->isReceive())
        {
            last_send = last_send->comm_prev;
        }
        if (last_send && last_send->partition != partition)
            last_send = NULL;

        next_send = comm_next;
        // Set next_send based on task
        while (next_send && next_send->isReceive())
        {
            next_send = next_send->comm_next;
        }
        if (next_send && next_send->partition != partition)
            next_send = NULL;
    }
}


void P2PEvent::set_stride_relationships(CommEvent * base)
{
    CommEvent * task_next = base->comm_next;

    // while we have receives
    while (task_next && task_next->isReceive())
    {
        task_next = task_next->comm_next;
    }

    if (task_next && task_next->partition == partition)
    {
        stride_children->insert(task_next);
        task_next->stride_parents->insert(this);
    }
}

void P2PEvent::update_strides()
{
    if (!is_recv)
        return;

    // Iterate through sends of this recv and check what
    // their strides are to update last_send and next_send
    // to be the tightest boundaries.
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
    {
        if (!last_send
                || (*msg)->sender->stride > last_send->stride)
        {
            last_send = (*msg)->sender;
        }
    }
}

QSet<Partition *> * P2PEvent::mergeForMessagesHelper()
{
    QSet<Partition *> * parts = new QSet<Partition *>();
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
    {
        parts->insert((*msg)->receiver->partition);
        parts->insert((*msg)->sender->partition);
    }
    return parts;
}

ClusterEvent * P2PEvent::createClusterEvent(QString metric, long long int divider)
{
    long long evt_metric = getMetric(metric);
    long long agg_metric = getMetric(metric, true);
    ClusterEvent::Threshhold threshhold = ClusterEvent::CE_THRESH_HIGH;
    if (evt_metric < divider)
        threshhold = ClusterEvent::CE_THRESH_LOW;
    ClusterEvent::Threshhold aggthreshhold = ClusterEvent::CE_THRESH_HIGH;
    if (agg_metric < divider)
        aggthreshhold = ClusterEvent::CE_THRESH_LOW;

    ClusterEvent * ce = new ClusterEvent(step);
    ClusterEvent::CommType commtype = ClusterEvent::CE_COMM_SEND;
    if (is_recv && messages->size() > 1)
    {
        commtype = ClusterEvent::CE_COMM_WAITALL;
        ce->waitallrecvs += messages->size();
    }
    else if (is_recv)
    {
        commtype = ClusterEvent::CE_COMM_RECV;
    }
    else if (messages->size() > 1)
    {
        commtype = ClusterEvent::CE_COMM_ISEND;
        ce->isends += messages->size();
    }

    ce->setMetric(1, evt_metric, ClusterEvent::CE_EVENT_COMM,
                  commtype, threshhold);
    ce->setMetric(1, agg_metric, ClusterEvent::CE_EVENT_AGG,
                  commtype, aggthreshhold);

    return ce;
}

void P2PEvent::addToClusterEvent(ClusterEvent * ce, QString metric,
                                 long long int divider)
{
    long long evt_metric = getMetric(metric);
    long long agg_metric = getMetric(metric, true);
    ClusterEvent::Threshhold threshhold = ClusterEvent::CE_THRESH_HIGH;
    if (evt_metric < divider)
        threshhold = ClusterEvent::CE_THRESH_LOW;
    ClusterEvent::Threshhold aggthreshhold = ClusterEvent::CE_THRESH_HIGH;
    if (agg_metric < divider)
        aggthreshhold = ClusterEvent::CE_THRESH_LOW;

    ClusterEvent::CommType commtype = ClusterEvent::CE_COMM_SEND;
    if (is_recv && messages->size() > 1)
    {
        commtype = ClusterEvent::CE_COMM_WAITALL;
        ce->waitallrecvs += messages->size();
    }
    else if (is_recv)
    {
        commtype = ClusterEvent::CE_COMM_RECV;
    }
    else if (messages->size() > 1)
    {
        commtype = ClusterEvent::CE_COMM_ISEND;
        ce->isends += messages->size();
    }

    ce->addMetric(1, evt_metric, ClusterEvent::CE_EVENT_COMM,
                  commtype, threshhold);
    ce->addMetric(1, agg_metric, ClusterEvent::CE_EVENT_AGG,
                  commtype, aggthreshhold);
}

void P2PEvent::addComms(QSet<CommBundle *> * bundleset)
{
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
        bundleset->insert(*msg);
}

QList<int> P2PEvent::neighborTasks()
{
    QSet<int> neighbors = QSet<int>();
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
    {
        neighbors.insert((*msg)->receiver->task);
        neighbors.insert((*msg)->sender->task);
    }
    neighbors.remove(task);
    return neighbors.toList();
}

void P2PEvent::writeToOTF2(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap)
{
    if (subevents && subevents->size() > 1)
    {
        for (QList<P2PEvent *>::Iterator sub = subevents->begin();
             sub != subevents->end(); ++sub)
        {
            (*sub)->writeToOTF2(writer, attributeMap);
        }
    }

    // If this event has subevents, the receiver or sender of the message will still
    // only match "this"
    // We do not need to do the IsendComplete portion as we only use that for the
    // automatic waitall merge which should be contained in the phase attribute
    // So we can get away with just using send/recv here instead of worrying about isend/irecv
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
    {
         if ((*msg)->receiver == this)
         {
             OTF2_EvtWriter_MpiRecv(writer,
                                    NULL,
                                    (*msg)->recvtime,
                                    (*msg)->sender->task,
                                    (*msg)->taskgroup,
                                    0,
                                    0);
         }

         if ((*msg)->sender == this)
         {
            OTF2_EvtWriter_MpiSend(writer,
                                   NULL,
                                   (*msg)->sendtime,
                                   (*msg)->receiver->task,
                                   (*msg)->taskgroup,
                                   0,
                                   0);
         }
    }

    // The rest as normal
    Event::writeToOTF2(writer, attributeMap);
}
