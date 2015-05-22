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
#ifndef EVENT_H
#define EVENT_H

#include <QVector>
#include <QMap>
#include <QString>
#include <otf2/otf2.h>


class Partition;
class Function;

class Event
{
public:
    Event(unsigned long long _enter, unsigned long long _exit, int _function,
          int _task);
    ~Event();

    // Based on enter time
    bool operator<(const Event &);
    bool operator>(const Event &);
    bool operator<=(const Event &);
    bool operator>=(const Event &);
    bool operator==(const Event &);

    Event * findChild(unsigned long long time);
    unsigned long long getVisibleEnd(unsigned long long start);
    Event * least_common_caller(Event * other);
    bool same_subtree(Event * other);
    Event * least_multiple_caller(QMap<Event *, int> * memo = NULL);
    Event * least_multiple_function_caller(QMap<int, Function *> * functions);
    virtual int comm_count(QMap<Event *, int> * memo = NULL);
    virtual bool isCommEvent() { return false; }
    virtual bool isReceive() { return false; }
    virtual bool isCollective() { return false; }
    virtual void writeToOTF2(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap);
    virtual void writeOTF2Leave(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap);
    virtual void writeOTF2Enter(OTF2_EvtWriter * writer);

    // Call tree info
    Event * caller;
    QVector<Event *> * callees;

    unsigned long long enter;
    unsigned long long exit;
    int function;
    int task;
    int depth;
};

static bool eventTaskLessThan(const Event * evt1, const Event * evt2)
{
    return evt1->task < evt2->task;
}

#endif // EVENT_H
