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
#ifndef COLLECTIVEEVENT_H
#define COLLECTIVEEVENT_H

#include "commevent.h"
#include "collectiverecord.h"

class CollectiveEvent : public CommEvent
{
public:
    CollectiveEvent(unsigned long long _enter, unsigned long long _exit,
                    int _function, int _entity, int _pe, int _phase,
                    CollectiveRecord * _collective);
    ~CollectiveEvent();

    // We count the collective as two since it serves as both the beginning
    // and ending of some sort of communication while P2P communication
    // events are either the begin (send) or the end (recv)
    int comm_count(QMap<Event *, int> *memo = NULL) { Q_UNUSED(memo); return 2; }

    bool isP2P() { return false; }
    bool isReceive() { return false; }
    virtual bool isCollective() { return true; }
    void fixPhases();
    void initialize_strides(QList<CommEvent *> * stride_events,
                            QList<CommEvent *> * recv_events);
    void initialize_basic_strides(QSet<CollectiveRecord *> *collectives);
    void update_basic_strides();
    bool calculate_local_step();
    void writeToOTF2(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap);

    void addComms(QSet<CommBundle *> * bundleset) { bundleset->insert(collective); }
    QList<int> neighborEntities();
    CollectiveRecord * getCollective() { return collective; }
    QSet<Partition *> * mergeForMessagesHelper();

    ClusterEvent * createClusterEvent(QString metric, long long divider);
    void addToClusterEvent(ClusterEvent * ce, QString metric,
                           long long divider);

    CollectiveRecord * collective;

private:
    void set_stride_relationships();
};

#endif // COLLECTIVEEVENT_H
