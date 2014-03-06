#include "partitioncluster.h"
#include <iostream>

PartitionCluster::PartitionCluster(int member, QList<Event *> * elist, QString metric, long long int divider)
    : open(false),
      max_distance(0),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QList<int>()),
      events(new QList<ClusterEvent *>()),
      extents(QRect())
{
    members->append(member);
    for (QList<Event *>::Iterator evt = elist->begin(); evt != elist->end(); ++evt)
    {
        long long low = 0, high = 0, agg_low = 0, agg_high = 0;
        long long evt_metric = (*evt)->getMetric(metric);
        long long agg_metric = (*evt)->getMetric(metric, true);
        int nsend = 0, nrecv = 0;
        if (evt_metric < divider)
            low = evt_metric;
        else
            high = evt_metric;
        if (agg_metric < divider)
            agg_low = agg_metric;
        else
            agg_high = agg_metric;
        for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
            if ((*msg)->sender == *evt)
                nsend += 1;
            else
                nrecv += 1;
        events->append(new ClusterEvent((*evt)->step, low, high, high ? 0 : 1, high ? 1 : 0,
                                        agg_low, agg_high, agg_high ? 0 : 1, agg_high ? 0 : 1,
                                        nsend, nrecv));
    }
}

PartitionCluster::PartitionCluster(long long int distance, PartitionCluster * c1, PartitionCluster * c2)
    : open(false),
      max_distance(distance),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QList<int>()),
      events(new QList<ClusterEvent *>()),
      extents(QRect())
{
    c1->parent = this;
    c2->parent = this;
    children->append(c1);
    children->append(c2);
    members->append(*(c1->members));
    members->append(*(c2->members));

    int index1 = 0, index2 = 0;
    ClusterEvent * evt1 = c1->events->at(0), * evt2 = c2->events->at(0);
    while (evt1 && evt2)
    {
        if (evt1->step == evt2->step) // If they're equal, add their distance
        {
            events->append(new ClusterEvent(evt1->step, evt1->low_metric + evt2->low_metric,
                                            evt1->high_metric + evt2->high_metric,
                                            evt1->num_low + evt2->num_low, evt1->num_high + evt2->num_high,
                                            evt1->agg_low + evt2->agg_low, evt1->agg_high + evt2->agg_high,
                                            evt1->num_agg_low + evt2->num_agg_low,
                                            evt1->num_agg_high + evt2->num_agg_high,
                                            evt1->num_send + evt2->num_send, evt1->num_recv + evt2->num_recv));

            // Increment both event lists now
            ++index2;
            if (index2 < c2->events->size())
                evt2 = c2->events->at(index2);
            else
                evt2 = NULL;
            ++index1;
            if (index1 < c1->events->size())
                evt1 = c1->events->at(index1);
            else
                evt1 = NULL;
        } else if (evt1->step > evt2->step) { // If not, add lower one and increment
            events->append(new ClusterEvent(*evt2));
            ++index2;
            if (index2 < c2->events->size())
                evt2 = c2->events->at(index2);
            else
                evt2 = NULL;
        } else {
            events->append(new ClusterEvent(*evt1));
            ++index1;
            if (index1 < c1->events->size())
                evt1 = c1->events->at(index1);
            else
                evt1 = NULL;
        }
    }
}

PartitionCluster::~PartitionCluster()
{
    delete children;
    delete members;

    for (QList<ClusterEvent *>::Iterator evt = events->begin(); evt != events->end(); ++evt)
    {
        delete *evt;
        *evt = NULL;
    }
    delete events;
}

void PartitionCluster::delete_tree()
{
    for (QList<PartitionCluster *>::Iterator child = children->begin(); child != children->end(); ++child)
    {
        (*child)->delete_tree();
        delete *child;
    }
}

PartitionCluster * PartitionCluster::get_root()
{
    if (parent)
        return parent->get_root();

    return this;
}

PartitionCluster * PartitionCluster::get_closed_root()
{
    if (parent && !parent->open)
        return parent->get_closed_root();

    return this;
}

int PartitionCluster::max_depth()
{
    int max = 0;
    for (QList<PartitionCluster *>::Iterator child = children->begin(); child != children->end(); ++child)
        max = std::max(max, (*child)->max_depth() + 1);
    return max;
}

QString PartitionCluster::memberString()
{
    QString ms = "[ ";
    if (!members->isEmpty())
    {
        ms += QString::number(members->at(0));
        for (int i = 1; i < members->size(); i++)
            ms += ", " + QString::number(members->at(i));
    }
    ms += " ]";
    return ms;
}

void PartitionCluster::print(QString indent)
{
    std::cout << indent.toStdString().c_str() << memberString().toStdString().c_str() << std::endl;
    QString myindent = indent + "   ";
    for (QList<PartitionCluster *>::Iterator child = children->begin(); child != children->end(); ++child)
        (*child)->print(myindent);
}

void PartitionCluster::close()
{
    open = false;
    for (QList<PartitionCluster *>::Iterator child = children->begin(); child != children->end(); ++child)
        (*child)->close();
}
