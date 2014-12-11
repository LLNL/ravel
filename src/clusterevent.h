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
#ifndef CLUSTEREVENT_H
#define CLUSTEREVENT_H

class ClusterEvent
{
public:
    ClusterEvent(int _step);

    ClusterEvent(const ClusterEvent& copy);
    ClusterEvent(int _step, const ClusterEvent * copy1,
                 const ClusterEvent * copy2);

    enum EventType { CE_EVENT_COMM, CE_EVENT_AGG };
    enum CommType { CE_COMM_SEND, CE_COMM_ISEND, CE_COMM_RECV,
                    CE_COMM_WAITALL, CE_COMM_COLL, CE_COMM_ALL };
    enum Threshhold { CE_THRESH_LOW, CE_THRESH_HIGH, CE_THRESH_BOTH };

    void setMetric(int count, long long int value,
                   EventType etype = CE_EVENT_COMM,
                   CommType ctype = CE_COMM_SEND,
                   Threshhold thresh = CE_THRESH_LOW);
    void addMetric(int count, long long int value,
                   EventType etype = CE_EVENT_COMM,
                   CommType ctype = CE_COMM_SEND,
                   Threshhold thresh = CE_THRESH_LOW);
    long long int getMetric(EventType etype = CE_EVENT_COMM,
                            CommType ctype = CE_COMM_ALL,
                            Threshhold thresh = CE_THRESH_BOTH);
    int getCount(EventType etype = CE_EVENT_COMM,
                 CommType ctype = CE_COMM_ALL,
                 Threshhold thresh = CE_THRESH_BOTH);

    int step;
    int waitallrecvs; // How many individual receives there are
    int isends;
    long long int metric[2][5][2]; // [COMM/AGG] [SEND/RECV/WAITALL/COLL] [LOW/HIGH]

     // This counts a waitall as a single event rather than all the receives
    int counts[2][5][2];
};

#endif // CLUSTEREVENT_H
