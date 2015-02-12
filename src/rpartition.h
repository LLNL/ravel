//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Ravel.
// Written by Kate Isaacs, kisaacs@acm.org, All rights reserved.
// LLNL-CODE-663885
//
// For details, see https://scalability-llnl.github.io/ravel
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
#ifndef RPARTITION_H
#define RPARTITION_H

#include <QString>
#include <QList>
#include <QSet>
#include <QVector>
#include <QMap>

class Gnome;
class Event;
class CommEvent;
class ClusterTask;

class Partition
{
public:
    Partition();
    ~Partition();
    void addEvent(CommEvent * e);
    void deleteEvents();
    void sortEvents();
    void step();
    void basic_step();

    // Based on step
    bool operator<(const Partition &);
    bool operator>(const Partition &);
    bool operator<=(const Partition &);
    bool operator>=(const Partition &);
    bool operator==(const Partition &);

    void fromSaved();

     // Time gap between partitions
    unsigned long long int distance(Partition * other);

    // For common caller merge
    Event * least_common_caller(int taskid, QMap<Event *, int> * memo = NULL);

     // For leap merge - which children can we merge to
    void calculate_dag_leap();
    QString generate_process_string(); // For debugging

     // When we're merging, find what this merged into
    Partition * newest_partition();
    int num_events();

    // Core partition information, events per process and step summary
    QMap<int, QList<CommEvent *> *> * events;
    int max_step;
    int max_global_step;
    int min_global_step;
    int dag_leap;

    // For message merge
    bool mark;

    // For dag / Tarjan / merging
    QSet<Partition *> * parents;
    QSet<Partition *> * children;
    QSet<Partition *> * old_parents;
    QSet<Partition *> * old_children;
    Partition * new_partition;
    int tindex;
    int lowlink;

    // For leap merge
    bool leapmark;
    QSet<Partition *> * group;

    // For graph drawing
    QString gvid;

    // For gnome and clustering
    // Cluster_vectors simplify the coding of distance calculations
    // between processes but don't aid performance so may be wasteful
    Gnome * gnome;
    int gnome_type;
    QVector<ClusterTask *> * cluster_tasks;
    void makeClusterVectors(QString metric);
    QMap<int, QVector<long long int> *> * cluster_vectors;
    QMap<int, int> * cluster_step_starts;

    bool debug_mark;

private:
    // Stepping logic -- probably want to rewrite
    int set_stride_dag(QList<CommEvent *> *stride_events);

    QList<CommEvent *> * free_recvs;

};

#endif // RPARTITION_H
