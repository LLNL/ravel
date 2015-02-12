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
#include "clustertask.h"
#include <float.h>

ClusterTask::ClusterTask(int _t, int _step)
    : task(_t),
      startStep(_step),
      metric_events(new QVector<long long int>())
{
}

ClusterTask::ClusterTask()
    : task(0),
      startStep(0),
      metric_events(new QVector<long long int>())
{

}

ClusterTask::ClusterTask(const ClusterTask & other)
    : task(other.task),
      startStep(other.startStep),
      metric_events(new QVector<long long int>())
{
    for (int i = 0; i < other.metric_events->size(); i++)
        metric_events->append(other.metric_events->at(i));
}

ClusterTask::~ClusterTask()
{
    delete metric_events;
}

// Distance between this ClusterTask and another. Since metric_events fills
// in the missing steps with the previous value, we can just go straight
// through from the startStep of the shorter one.
double ClusterTask::calculateMetricDistance(const ClusterTask& other) const
{
    int num_matches = metric_events->size();
    double total_difference = 0;
    int offset = 0;
    if (metric_events->size() && other.metric_events->size())
        if (startStep < other.startStep)
        {
            num_matches = other.metric_events->size();
            offset = metric_events->size() - other.metric_events->size();
            for (int i = 0; i < other.metric_events->size(); i++)
                total_difference += (metric_events->at(offset + i)
                                    - other.metric_events->at(i))
                                    * (metric_events->at(offset + i)
                                    - other.metric_events->at(i));
        }
        else
        {
            offset = other.metric_events->size() - metric_events->size();
            for (int i = 0; i < metric_events->size(); i++)
                total_difference += (other.metric_events->at(offset + i)
                                    - metric_events->at(i))
                                    * (other.metric_events->at(offset + i)
                                    - metric_events->at(i));
        }
    if (num_matches <= 0)
        return DBL_MAX;
    return total_difference / num_matches;
}

// Adds the metric_events of the second ClusterTask, ignores
// any possible task this is represented (task field in classs)
ClusterTask& ClusterTask::operator+(const ClusterTask & other)
{
    int offset = 0;
    if (metric_events->size() && other.metric_events->size())
    {
        if (startStep < other.startStep)
        {
            offset = metric_events->size() - other.metric_events->size();
            for (int i = 0; i < other.metric_events->size(); i++)
                (*metric_events)[offset + i] += other.metric_events->at(i);
        }
        else
        {
            offset = other.metric_events->size() - metric_events->size();
            for (int i = 0; i < metric_events->size(); i++)
                (*metric_events)[i] += other.metric_events->at(offset + i);
            for (int i = offset - 1; i >= 0; i--)
                metric_events->prepend(other.metric_events->at(i));
            startStep = other.startStep;
        }
    }
    else if (other.metric_events->size())
    {
        startStep = other.startStep;
        for (int i = 0; i < other.metric_events->size(); i++)
            metric_events->append(other.metric_events->at(i));
    }


    return *this;
}

ClusterTask& ClusterTask::operator/(const int divisor)
{
    for (int i = 0; i < metric_events->size(); i++)
    {
        (*metric_events)[i] /= divisor;
    }
    return *this;
}

ClusterTask& ClusterTask::operator=(const ClusterTask & other)
{
    task = other.task;
    startStep = other.startStep;
    metric_events->clear();
    for (int i = 0; i < other.metric_events->size(); i++)
        metric_events->append(other.metric_events->at(i));
    return *this;
}
