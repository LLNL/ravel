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
#ifndef COMMEVENT_H
#define COMMEVENT_H

#include "event.h"
#include <QString>
#include <QList>
#include <QSet>
#include <QMap>

class CommBundle;
class Message;
class CollectiveRecord;
class Metrics;

class CommEvent : public Event
{
public:
    CommEvent(unsigned long long _enter, unsigned long long _exit,
              int _function, int _entity, int _pe, int _phase);
    ~CommEvent();

    bool operator<(const CommEvent &);
    bool operator>(const CommEvent &);
    bool operator<=(const CommEvent &);
    bool operator>=(const CommEvent &);
    bool operator==(const CommEvent &);

    virtual int comm_count(QMap<Event *, int> *memo = NULL)=0;
    bool isCommEvent() { return true; }
    virtual bool isP2P() { return false; }
    virtual bool isReceive() const { return false; }
    virtual bool isCollective() { return false; }

    virtual QVector<Message *> * getMessages() { return NULL; }
    virtual CollectiveRecord * getCollective() { return NULL; }

    CommEvent * comm_next;
    CommEvent * comm_prev;
    CommEvent * true_next;
    CommEvent * true_prev;
    CommEvent * pe_next;
    CommEvent * pe_prev;

    int phase;

    // For graph drawing
    QString gvid;

};
#endif // COMMEVENT_H
