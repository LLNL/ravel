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
#ifndef CLUSTERTASK_H
#define CLUSTERTASK_H

#include <QVector>

class ClusterTask
{
public:
    ClusterTask();
    ClusterTask(int _t, int _step);
    ClusterTask(const ClusterTask& other);
    ~ClusterTask();

    int task;
    int startStep; // What step our metric_events starts at

    // Representative vector of events for clustering. This is contiguous
    // so all missing steps should be filled in with their previous lateness
    // value by whoever builds this ClusterTask
    QVector<long long int> * metric_events;

    ClusterTask& operator+(const ClusterTask &);
    ClusterTask& operator/(const int);
    ClusterTask& operator=(const ClusterTask &);

    double calculateMetricDistance(const ClusterTask& other) const;

};

#endif // CLUSTERTASK_H
