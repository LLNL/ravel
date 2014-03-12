#include "partitioncluster.h"
#include <iostream>

PartitionCluster::PartitionCluster(int member, QList<Event *> * elist, QString metric, long long int divider)
    : open(false),
      drawnOut(false),
      max_distance(0),
      max_metric(LLONG_MIN),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QList<int>()),
      events(new QList<ClusterEvent *>()),
      extents(QRect())
{
    members->append(member);
    for (QList<Event *>::Iterator evt = elist->begin(); evt != elist->end(); ++evt)
    {
        long long evt_metric = (*evt)->getMetric(metric);
        long long agg_metric = (*evt)->getMetric(metric, true);
        if (evt_metric > max_metric)
            max_metric = evt_metric;
        int nsend = 0, nrecv = 0;
        ClusterEvent * ce = new ClusterEvent((*evt)->step);

        for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
            if ((*msg)->sender == *evt)
                nsend += 1;
            else
                nrecv += 1;

        if (evt_metric < divider)
        {
            if (nsend)
                ce->setMetric(nsend, evt_metric, ClusterEvent::COMM, ClusterEvent::SEND, ClusterEvent::LOW);
            if (nrecv > 1)
            {
                ce->setMetric(1, evt_metric, ClusterEvent::COMM, ClusterEvent::WAITALL, ClusterEvent::LOW);
                ce->waitallrecvs = nrecv;
            }
            else if (nrecv)
                ce->setMetric(nrecv, evt_metric, ClusterEvent::COMM, ClusterEvent::RECV, ClusterEvent::LOW);
        }
        else
        {
            if (nsend)
                ce->setMetric(nsend, evt_metric, ClusterEvent::COMM, ClusterEvent::SEND, ClusterEvent::HIGH);
            if (nrecv > 1)
            {
                ce->setMetric(1, evt_metric, ClusterEvent::COMM, ClusterEvent::WAITALL, ClusterEvent::HIGH);
                ce->waitallrecvs = nrecv;
            }
            else if (nrecv)
                ce->setMetric(nrecv, evt_metric, ClusterEvent::COMM, ClusterEvent::RECV, ClusterEvent::HIGH);

        }
        if (agg_metric < divider)
        {
            ce->setMetric(1, agg_metric, ClusterEvent::AGG, ClusterEvent::SEND, ClusterEvent::LOW);
        }
        else
        {
            ce->setMetric(1, agg_metric, ClusterEvent::AGG, ClusterEvent::SEND, ClusterEvent::HIGH);
        }

        events->append(ce);
    }
}

PartitionCluster::PartitionCluster(long long int distance, PartitionCluster * c1, PartitionCluster * c2)
    : open(false),
      max_distance(distance),
      max_metric(std::max(c1->max_metric, c2->max_metric)),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QList<int>()),
      events(new QList<ClusterEvent *>()),
      extents(QRect())
{
    c1->parent = this;
    c2->parent = this;
    members->append(*(c1->members));
    members->append(*(c2->members));
    // Children are ordered by their max_metric
    if (c1->max_metric > c2->max_metric)
    {
        children->append(c2);
        children->append(c1);
    }
    else
    {
        children->append(c1);
        children->append(c2);
    }


    int index1 = 0, index2 = 0;
    ClusterEvent * evt1 = c1->events->at(0), * evt2 = c2->events->at(0);
    while (evt1 && evt2)
    {
        if (evt1->step == evt2->step) // If they're equal, add their distance
        {
            events->append(new ClusterEvent(evt1->step, evt1, evt2));

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
    while (evt1)
    {
        events->append(new ClusterEvent(*evt1));
        ++index1;
        if (index1 < c1->events->size())
            evt1 = c1->events->at(index1);
        else
            evt1 = NULL;
    }
    while (evt2)
    {
        events->append(new ClusterEvent(*evt2));
        ++index2;
        if (index2 < c2->events->size())
            evt2 = c2->events->at(index2);
        else
            evt2 = NULL;
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

int PartitionCluster::max_open_depth()
{
    if (!open)
        return 0;

    int max = 0;
    for (QList<PartitionCluster *>::Iterator child = children->begin(); child != children->end(); ++child)
        max = std::max(max, (*child)->max_open_depth() + 1);
    return max;

}

bool PartitionCluster::leaf_open()
{
    if (members->size() == 1)
        return true;

    bool leaf = false;
    for (QList<PartitionCluster *>::Iterator child = children->begin(); child != children->end(); ++child)
        leaf = leaf || (*child)->leaf_open();
    return leaf;
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
