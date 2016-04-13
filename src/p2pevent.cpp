#include "p2pevent.h"
#include "commbundle.h"
#include "message.h"
#include "clusterevent.h"
#include "metrics.h"
#include "commdrawinterface.h"
#include <iostream>

P2PEvent::P2PEvent(unsigned long long _enter, unsigned long long _exit,
                   int _function, int _entity, int _pe, int _phase,
                   QVector<Message *> *_messages)
    : CommEvent(_enter, _exit, _function, _entity, _pe, _phase),
      subevents(NULL),
      messages(_messages),
      is_recv(false)
{
}

P2PEvent::P2PEvent(QList<P2PEvent *> * _subevents)
    : CommEvent(_subevents->first()->enter, _subevents->last()->exit,
                _subevents->first()->function, _subevents->first()->entity,
                _subevents->first()->pe, _subevents->first()->phase),
      subevents(_subevents),
      messages(new QVector<Message *>()),
      is_recv(_subevents->first()->is_recv)
{
    this->depth = subevents->first()->depth;

    // Take over submessages & caller/callee relationships
    for (QList<P2PEvent *>::Iterator evt = subevents->begin();
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

        callees->append(*evt);

        (*evt)->caller = this;
    }

    // Aggregate existing metrics
    P2PEvent * first = _subevents->first();
    QList<QString> names = first->metrics->getMetricList();
    for (QList<QString>::Iterator counter = names.begin();
         counter != names.end(); ++counter)
    {
        unsigned long long metric = 0, agg = 0;
        for (QList<P2PEvent *>::Iterator evt = subevents->begin();
             evt != subevents->end(); ++evt)
        {
            metric += (*evt)->getMetric(*counter);
            agg += (*evt)->getMetric(*counter, true);
        }
        metrics->addMetric(*counter, metric, agg);
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

bool P2PEvent::operator<(const P2PEvent &event)
{
    if (enter == event.enter)
    {
        if (add_order == event.add_order)
        {
            if (is_recv)
                return true;
            else
                return false;
        }
        return add_order < event.add_order;
    }
    return enter < event.enter;
}

bool P2PEvent::operator>(const P2PEvent &event)
{
    if (enter == event.enter)
    {
        if (add_order == event.add_order)
        {
            if (is_recv)
                return false;
            else
                return true;
        }
        return add_order > event.add_order;
    }
    return enter > event.enter;
}

bool P2PEvent::operator<=(const P2PEvent &event)
{
    if (enter == event.enter)
    {
        if (add_order == event.add_order)
        {
            if (is_recv)
                return true;
            else
                return false;
        }
        return add_order <= event.add_order;
    }
    return enter <= event.enter;
}

bool P2PEvent::operator>=(const P2PEvent &event)
{
    if (enter == event.enter)
    {
        if (add_order == event.add_order)
        {
            if (is_recv)
                return false;
            else
                return true;
        }
        return add_order >= event.add_order;
    }
    return enter >= event.enter;
}

bool P2PEvent::operator==(const P2PEvent &event)
{
    return enter == event.enter
            && add_order == event.add_order
            && is_recv == event.is_recv;
}


bool P2PEvent::isReceive() const
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

bool P2PEvent::calculate_local_step()
{
    int temp_step = -1;
    if (comm_prev && comm_prev->partition == partition)
    {
        if (comm_prev->step >= 0)
            temp_step = comm_prev->step + 1;
    }
    else
    {
        temp_step = 0;
    }

    if (is_recv)
    {
        for (QVector<Message *>::Iterator msg
             = messages->begin();
             msg != messages->end(); ++msg)
        {
            if ((*msg)->sender->step < 0)
            {
                return false;
            }
            else if ((*msg)->sender->step >= temp_step)
            {
                temp_step = (*msg)->sender->step + 1;
            }
        }
    }

    if (temp_step >= 0)
    {
        step = temp_step;
        return true;
    }

    return false;
}

void P2PEvent::calculate_differential_metric(QString metric_name,
                                             QString base_name, bool aggregates)
{
    long long max_parent = 0, max_agg_parent = 0;
    if (aggregates) // If we have aggregates, the aggregate is the prev
    {
        max_parent = getMetric(base_name, true);
        if (comm_prev)
            max_agg_parent = comm_prev->getMetric(base_name);
    }
    else if (comm_prev) // Otheriwse, just look at the previous if it exists
    {
        max_parent = comm_prev->getMetric(base_name);
    }


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

    if (aggregates)
        metrics->addMetric(metric_name,
                           std::max(0.,
                                    getMetric(base_name)- max_parent),
                           std::max(0.,
                                    getMetric(base_name, true)- max_agg_parent));
    else
        metrics->addMetric(metric_name,
                           std::max(0.,
                                    getMetric(base_name)- max_parent));
}

void P2PEvent::initialize_basic_strides(QSet<CollectiveRecord *> * collectives)
{
    Q_UNUSED(collectives);
    // Do nothing
}

// If the stride value is less than zero, we know it isn't
// part of a stride and thus in this case is a send or recv
void P2PEvent::update_basic_strides()
{
    if (comm_prev && comm_prev->partition == partition)
        last_stride = comm_prev;

    // Set last_stride based on entity
    while (last_stride && last_stride->stride < 0)
    {
        last_stride = last_stride->comm_prev;
    }
    if (last_stride && last_stride->partition != partition)
        last_stride = NULL;

    next_stride = comm_next;
    // Set next_stride based on entity
    while (next_stride && next_stride->stride < 0)
    {
        next_stride = next_stride->comm_next;
    }
    if (next_stride && next_stride->partition != partition)
        next_stride = NULL;
}

void P2PEvent::initialize_strides(QList<CommEvent *> * stride_events,
                                  QList<CommEvent *> * recv_events)
{
    if (!is_recv)
    {
        stride_events->append(this);

        // The next one in the entity is a stride child
        set_stride_relationships(this);

        // Follow messages to their receives and then along
        // the new entity to find more stride children
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
            last_stride = comm_prev;
        // Set last_stride based on entity
        while (last_stride && last_stride->isReceive())
        {
            last_stride = last_stride->comm_prev;
        }
        if (last_stride && last_stride->partition != partition)
            last_stride = NULL;

        next_stride = comm_next;
        // Set next_stride based on entity
        while (next_stride && next_stride->isReceive())
        {
            next_stride = next_stride->comm_next;
        }
        if (next_stride && next_stride->partition != partition)
            next_stride = NULL;
    }
}


void P2PEvent::set_stride_relationships(CommEvent * base)
{
    CommEvent * entity_next = base->comm_next;

    // while we have receives
    while (entity_next && entity_next->isReceive())
    {
        entity_next = entity_next->comm_next;
    }

    if (entity_next && entity_next->partition == partition)
    {
        stride_children->insert(entity_next);
        entity_next->stride_parents->insert(this);
    }
}

void P2PEvent::update_strides()
{
    if (!is_recv)
        return;

    // Iterate through sends of this recv and check what
    // their strides are to update last_stride and next_stride
    // to be the tightest boundaries.
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
    {
        if (!last_stride
                || (*msg)->sender->stride > last_stride->stride)
        {
            last_stride = (*msg)->sender;
        }
    }
}

void P2PEvent::set_reorder_strides(QMap<int, QList<CommEvent *> *> * stride_map,
                                   int offset, CommEvent *last, int debug)
{
    Q_UNUSED(last);
    offset = 1;
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
    {
        (*msg)->receiver->stride = stride + offset;
        //std::cout << "Setting stride " << (*msg)->receiver->stride << " for entity " << (*msg)->receiver->entity << " with add order " << (*msg)->receiver->add_order << std::endl;

        if (!stride_map->contains(stride + offset))
        {
            stride_map->insert(stride + offset,
                               new QList<CommEvent *>());
        }
        Q_ASSERT(stride + offset != stride);
        Q_ASSERT(stride + offset != debug);
        stride_map->value(stride + offset)->append((*msg)->receiver);

        // Last stride to self for ordering
        (*msg)->receiver->last_stride = (*msg)->receiver; //this;
        /*if ((*msg)->receiver->comm_prev)
        {
            CommEvent * prev = (*msg)->receiver->comm_prev;
            while (prev && !prev->isReceive() && prev->partition == partition)
                prev = prev->comm_prev;

            if (prev && prev->partition == partition && prev->isReceive())
                (*msg)->receiver->last_stride = prev;
        }*/

        // next stride for tie-breaking so we know what entity it is
        (*msg)->receiver->next_stride = this;
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

void P2PEvent::track_delay(QPainter *painter, CommDrawInterface *vis)
{
    vis->drawDelayTracking(painter, this);
}

// What is the cause of the delay, this sender, or the pe_prev
// that is the input parameter?
CommEvent * P2PEvent::compare_to_sender(CommEvent * prev)
{
    if (!is_recv)
        return prev;

    unsigned long long max_time = 0;
    CommEvent * sender = NULL;
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
    {
        if ((*msg)->sender->caller && (*msg)->sender->caller->exit > max_time)
        {
            max_time = (*msg)->sender->caller->exit;
            sender = (*msg)->sender;
        }
        else if ((*msg)->sender->exit > max_time)
        {
            max_time = (*msg)->sender->exit;
            sender = (*msg)->sender;
        }
    }

    if (!prev)
        return sender;

    if (max_time > prev->exit)
        return sender;

    return prev;
}

void P2PEvent::addComms(QSet<CommBundle *> * bundleset)
{
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
        bundleset->insert(*msg);
}

QList<int> P2PEvent::neighborEntities()
{
    QSet<int> neighbors = QSet<int>();
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
    {
        neighbors.insert((*msg)->receiver->entity);
        neighbors.insert((*msg)->sender->entity);
    }
    neighbors.remove(entity);
    return neighbors.toList();
}

// Here between the enter and leave we know we have no children as normal,
// but we do have subevents. So we want to write those as the correct time.
void P2PEvent::writeToOTF2(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap)
{
    writeOTF2Enter(writer);

    if (subevents)
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
                                    (*msg)->sender->entity,
                                    (*msg)->entitygroup,
                                    (*msg)->tag,
                                    (*msg)->size);
         }
         else if (!subevents) // Write if you are the true isend
         {
            OTF2_EvtWriter_MpiSend(writer,
                                   NULL,
                                   (*msg)->sendtime,
                                   (*msg)->receiver->entity,
                                   (*msg)->entitygroup,
                                   (*msg)->tag,
                                   (*msg)->size);
         }


    }

    writeOTF2Leave(writer, attributeMap);
}
