#include "collectiveevent.h"
#include "clusterevent.h"

CollectiveEvent::CollectiveEvent(unsigned long long _enter,
                                 unsigned long long _exit,
                                 int _function, int _task, int _phase,
                                 CollectiveRecord *_collective)
    : CommEvent(_enter, _exit, _function, _task, _phase),
      collective(_collective)
{
}

CollectiveEvent::~CollectiveEvent()
{
    delete collective;
}

// We check mark so we only do this once per collective,
// We can do this here because we know that the mark isn't
// being used by partitioning later, as here we're partitioning
// by phase function.
void CollectiveEvent::fixPhases()
{
    if (!collective->mark)
    {
        int maxphase = 0;
        for (QList<CollectiveEvent *>::Iterator ce
             = collective->events->begin();
             ce != collective->events->end(); ++ce)
        {
            if ((*ce)->phase > maxphase)
                maxphase = (*ce)->phase;
        }
        for (QList<CollectiveEvent *>::Iterator ce
             = collective->events->begin();
             ce != collective->events->end(); ++ce)
        {
            (*ce)->phase = maxphase;
        }
        collective->mark = true;
    }
}

void CollectiveEvent::initialize_strides(QList<CommEvent *> * stride_events,
                                         QList<CommEvent *> * recv_events)
{
    Q_UNUSED(recv_events);
    stride_events->append(this);

    // The next one in the task is a stride child
    set_stride_relationships();
}

void CollectiveEvent::set_stride_relationships()
{
    CommEvent * task_next = comm_next;

    // while we have receives
    while (task_next && task_next->isReceive())
    {
        task_next = task_next->comm_next;
    }

    if (task_next && task_next->partition == partition)
    {
        // Add to everyone in the collective
        // as a parent. This will force the collective to be after
        // anything that happens before any of the collectives.
        for (QList<CollectiveEvent *>::Iterator ev
             = collective->events->begin();
             ev != collective->events->end(); ++ev)
        {
            task_next->stride_parents->insert(*ev);
            (*ev)->stride_children->insert(task_next);
        }
    }
}

QSet<Partition *> *CollectiveEvent::mergeForMessagesHelper()
{
    QSet<Partition *> * parts = new QSet<Partition *>();
    if (!collective->mark)
    {
        for (QList<CollectiveEvent *>::Iterator ev2
             = collective->events->begin();
             ev2 != collective->events->end(); ++ev2)
        {
            parts->insert((*ev2)->partition);
        }

        // Mark so we don't have to do the above again
        collective->mark = true;
    }
    return parts;
}

ClusterEvent * CollectiveEvent::createClusterEvent(QString metric, long long int divider)
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

    ce->setMetric(1, evt_metric, ClusterEvent::CE_EVENT_COMM,
                  ClusterEvent::CE_COMM_COLL, threshhold);
    ce->setMetric(1, agg_metric, ClusterEvent::CE_EVENT_AGG,
                  ClusterEvent::CE_COMM_COLL, aggthreshhold);

    return ce;
}

void CollectiveEvent::addToClusterEvent(ClusterEvent * ce, QString metric,
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


    ce->addMetric(1, evt_metric, ClusterEvent::CE_EVENT_COMM,
                  ClusterEvent::CE_COMM_COLL, threshhold);
    ce->addMetric(1, agg_metric, ClusterEvent::CE_EVENT_AGG,
                  ClusterEvent::CE_COMM_COLL, aggthreshhold);
}

QList<int> CollectiveEvent::neighborTasks()
{
    QList<int> neighbors = QList<int>();
    for (QList<CollectiveEvent *>::Iterator evt = collective->events->begin();
         evt != collective->events->end(); ++evt)
    {
        neighbors.append((*evt)->task);
    }
    neighbors.removeOne(task);
    return neighbors;
}

void CollectiveEvent::writeToOTF2(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap)
{
    // write the collective event
    OTF2_EvtWriter_MpiCollectiveBegin(writer,
                                      NULL,
                                      enter);

    OTF2_EvtWriter_MpiCollectiveEnd(writer,
                                    NULL,
                                    exit,
                                    collective->collective,
                                    collective->taskgroup,
                                    collective->root,
                                    0,
                                    0);

    // The rest as normal
    Event::writeToOTF2(writer, attributeMap);
}
