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
#ifndef COMMEVENT_H
#define COMMEVENT_H

#include "event.h"
#include <QString>
#include <QList>
#include <QSet>
#include <QMap>

class Partition;
class ClusterEvent;
class CommBundle;
class Message;
class CollectiveRecord;

class CommEvent : public Event
{
public:
    CommEvent(unsigned long long _enter, unsigned long long _exit,
              int _function, int _task, int _phase);
    ~CommEvent();

    void addMetric(QString name, double event_value,
                   double aggregate_value = 0);
    void setMetric(QString name, double event_value,
                   double aggregate_value = 0);
    bool hasMetric(QString name);
    double getMetric(QString name, bool aggregate = false);

    virtual int comm_count(QMap<Event *, int> *memo = NULL)=0;
    bool isCommEvent() { return true; }
    virtual bool isP2P() { return false; }
    virtual bool isReceive() { return false; }
    virtual bool isCollective() { return false; }
    virtual void writeOTF2Leave(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap);

    virtual void fixPhases()=0;
    virtual void calculate_differential_metric(QString metric_name,
                                               QString base_name);
    virtual void initialize_strides(QList<CommEvent *> * stride_events,
                                    QList<CommEvent *> * recv_events)=0;
    virtual void update_strides() { return; }
    virtual void initialize_basic_strides(QSet<CollectiveRecord *> * collectives)=0;
    virtual void update_basic_strides()=0;
    virtual bool calculate_local_step()=0;

    virtual ClusterEvent * createClusterEvent(QString metric, long long divider)=0;
    virtual void addToClusterEvent(ClusterEvent * ce, QString metric,
                                   long long divider)=0;

    virtual void addComms(QSet<CommBundle *> * bundleset)=0;
    virtual QList<int> neighborTasks()=0;
    virtual QVector<Message *> * getMessages() { return NULL; }
    virtual CollectiveRecord * getCollective() { return NULL; }

    virtual QSet<Partition *> * mergeForMessagesHelper()=0;

    class MetricPair {
    public:
        MetricPair(double _e, double _a)
            : event(_e), aggregate(_a) {}

        double event; // value at event
        double aggregate; // value at prev. aggregate event
    };

    QMap<QString, MetricPair *> * metrics; // Lateness or Counters etc

    Partition * partition;
    CommEvent * comm_next;
    CommEvent * comm_prev;

    // Used in stepping procedure
    CommEvent * last_stride;
    CommEvent * next_stride;
    QList<CommEvent *> * last_recvs;
    int last_step;
    QSet<CommEvent *> * stride_parents;
    QSet<CommEvent *> * stride_children;
    int stride;

    int step;
    int phase;

};

#endif // COMMEVENT_H
