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
#ifndef PARTITIONCLUSTER_H
#define PARTITIONCLUSTER_H

#include <QRect>
#include <QString>
#include <QSet>
#include <QList>

class CommEvent;
class ClusterEntity;
class PartitionCluster;
class ClusterEvent;

// Cluster node of a hierarchical clustering, contains a subset of a partition
class PartitionCluster
{
public:
    PartitionCluster(int num_steps, int start, long long _divider = LLONG_MAX);
    PartitionCluster(int member, QList<CommEvent *> *elist, QString metric,
                     long long int _divider = LLONG_MAX);
    PartitionCluster(long long int distance, PartitionCluster * c1,
                     PartitionCluster * c2);
    ~PartitionCluster();
    long long int addMember(ClusterEntity * cp, QList<CommEvent *> *elist,
                            QString metric);
    long long int distance(PartitionCluster * other);
    void makeClusterVectors();

    // Call at root before deconstructing
    void delete_tree();

    // Tree info, also used for vis
    PartitionCluster * get_root();
    PartitionCluster * get_closed_root();
    int max_depth();
    int max_open_depth();
    bool leaf_open();
    int visible_clusters();
    void close();

    // For debug
    QString memberString();
    void print(QString indent = "");



    int startStep;
    int max_entity;
    bool open; // Whether children are drawn
    bool drawnOut;
    long long int max_distance; // max within cluster distance
    long long int max_metric;
    long long int divider; // For threshholding (currently unused)
    PartitionCluster * parent;
    QList<PartitionCluster *> * children;
    QList<int> * members; // Entities in the cluster
    QList<ClusterEvent *> * events; // Represented events
    QRect extents; // Where it was drawn

    // Vector that has previous 'lateness' filling in steps without events
    QVector<long long int> * cluster_vector;
    int clusterStart; // Starting step of the cluster_vector
};

#endif // PARTITIONCLUSTER_H
