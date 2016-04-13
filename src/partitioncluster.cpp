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
#include "partitioncluster.h"
#include <iostream>
#include <climits>

#include "clusterevent.h"
#include "clusterentity.h"
#include "event.h"
#include "commevent.h"

// Start an empty cluster
PartitionCluster::PartitionCluster(int num_steps, int start,
                                   long long int _divider)
    : startStep(start),
      max_entity(-1),
      open(false),
      drawnOut(false),
      max_distance(0),
      max_metric(LLONG_MIN),
      divider(_divider),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QList<int>()),
      events(new QList<ClusterEvent *>()),
      extents(QRect()),
      cluster_vector(new QVector<long long int>()),
      clusterStart(-1)
{
    for (int i = 0; i < num_steps; i+= 2)
        events->append(new ClusterEvent(startStep + i));
}

// cluster_vectors already have previous metric in steps not represented in
// this cluster, so they can be used directly from their starting points to
// calculate distance
long long int PartitionCluster::distance(PartitionCluster * other)
{

    int num_matches = cluster_vector->size() - clusterStart;
    long long int total_difference = 0;
    if (clusterStart < other->clusterStart)
    {
        num_matches = other->cluster_vector->size() - other->clusterStart;
        for (int i = 0; i < other->cluster_vector->size(); i++)
            total_difference += (cluster_vector->at(i)
                                 - other->cluster_vector->at(i))
                                * (cluster_vector->at(i)
                                   - other->cluster_vector->at(i));
    }
    else
    {
        for (int i = 0; i < cluster_vector->size(); i++)
            total_difference += (other->cluster_vector->at(i)
                                 - cluster_vector->at(i))
                                * (other->cluster_vector->at(i)
                                   - cluster_vector->at(i));
    }
    if (num_matches <= 0)
        return LLONG_MAX;
    return total_difference / num_matches;
}

// Using the events that may skip steps, this makes the continuous
// cluster_vector. We may want to remove this for memory concerns
void PartitionCluster::makeClusterVectors()
{
    cluster_vector->clear();
    long long int value, last_value = -1;
    for (int i = 0; i < events->size(); i++)
    {
        ClusterEvent * ce = events->at(i);
        if (ce->getCount() == 0 && last_value >= 0)
        {
            cluster_vector->append(last_value);
        }
        else if (ce->getCount() > 0)
        {
            value = ce->getMetric() / ce->getCount();
            last_value = value;
            cluster_vector->append(value);
            if (clusterStart < 0)
                clusterStart = i;
        }
    }
}


// Add another entity to this cluster using the events in elist, the given
// metric and the entity encapsulated by ClusterEntity
long long int PartitionCluster::addMember(ClusterEntity * cp,
                                          QList<CommEvent *> * elist,
                                          QString metric)
{
    members->append(cp->entity);
    long long int max_evt_metric = 0;
    for (QList<CommEvent *>::Iterator evt = elist->begin();
         evt != elist->end(); ++evt)
    {
        long long evt_metric = (*evt)->getMetric(metric);
        if (evt_metric > max_metric)
        {
            max_metric = evt_metric;
            max_entity = cp->entity;
        }
        if (evt_metric > max_evt_metric)
            max_evt_metric = evt_metric;

        ClusterEvent * ce = events->at(((*evt)->step - startStep) / 2);
        (*evt)->addToClusterEvent(ce, metric, divider);
    }

    return max_evt_metric;
}

// Start a cluster with a single member
PartitionCluster::PartitionCluster(int member, QList<CommEvent *> *elist,
                                   QString metric, long long int _divider)
    : startStep(elist->at(0)->step),
      max_entity(member),
      open(false),
      drawnOut(false),
      max_distance(0),
      max_metric(LLONG_MIN),
      divider(_divider),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QList<int>()),
      events(new QList<ClusterEvent *>()),
      extents(QRect()),
      cluster_vector(new QVector<long long int>()),
      clusterStart(-1)
{
    members->append(member);
    for (QList<CommEvent *>::Iterator evt = elist->begin();
         evt != elist->end(); ++evt)
    {
        long long evt_metric = (*evt)->getMetric(metric);
        if (evt_metric > max_metric)
            max_metric = evt_metric;

        events->append((*evt)->createClusterEvent(metric, divider));
    }
}

// Create a new cluster as a merger of two existing clusters
PartitionCluster::PartitionCluster(long long int distance,
                                   PartitionCluster * c1,
                                   PartitionCluster * c2)
    : startStep(std::min(c1->startStep, c2->startStep)),
      max_entity(c1->max_entity),
      open(false),
      max_distance(distance),
      max_metric(std::max(c1->max_metric, c2->max_metric)),
      divider(c1->divider),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QList<int>()),
      events(new QList<ClusterEvent *>()),
      extents(QRect()),
      cluster_vector(new QVector<long long int>()),
      clusterStart(-1)
{
    c1->parent = this;
    c2->parent = this;
    members->append(*(c1->members));
    members->append(*(c2->members));
    if (c2->max_metric > c1->max_metric)
        max_entity = c2->max_entity;

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

// delete_tree() should be called from the root and then only the
// root needs to be deleted directly.
PartitionCluster::~PartitionCluster()
{
    delete children;
    delete members;

    for (QList<ClusterEvent *>::Iterator evt = events->begin();
         evt != events->end(); ++evt)
    {
        delete *evt;
        *evt = NULL;
    }
    delete events;
    delete cluster_vector;
}

void PartitionCluster::delete_tree()
{
    for (QList<PartitionCluster *>::Iterator child = children->begin();
         child != children->end(); ++child)
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
    for (QList<PartitionCluster *>::Iterator child = children->begin();
         child != children->end(); ++child)
    {
        max = std::max(max, (*child)->max_depth() + 1);
    }
    return max;
}

// Max tree depth counting only open nodes
int PartitionCluster::max_open_depth()
{
    if (!open)
        return 0;

    int max = 0;
    for (QList<PartitionCluster *>::Iterator child = children->begin();
         child != children->end(); ++child)
    {
        max = std::max(max, (*child)->max_open_depth() + 1);
    }
    return max;

}

// Check if children are true leaves since they might not be
// if we're doing hierarchical clusters on pre-clustered data
bool PartitionCluster::leaf_open()
{
    if (members->size() == 1)
        return true;

    bool leaf = false;
    for (QList<PartitionCluster *>::Iterator child = children->begin();
         child != children->end(); ++child)
    {
        leaf = leaf || (*child)->leaf_open();
    }
    return leaf;
}

// How many clusters under me (including myself) are open/visible
int PartitionCluster::visible_clusters()
{
    if (!open)
        return 1;

    int visible = 0;
    for (QList<PartitionCluster *>::Iterator child = children->begin();
         child != children->end(); ++child)
    {
        visible += (*child)->visible_clusters();
    }
    return visible;
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
    std::cout << indent.toStdString().c_str()
              << memberString().toStdString().c_str() << std::endl;
    QString myindent = indent + "   ";
    for (QList<PartitionCluster *>::Iterator child = children->begin();
         child != children->end(); ++child)
    {
        (*child)->print(myindent);
    }
}

// Collapse this cluster and all its children from the vis
void PartitionCluster::close()
{
    open = false;
    for (QList<PartitionCluster *>::Iterator child = children->begin();
         child != children->end(); ++child)
    {
        (*child)->close();
    }
}
