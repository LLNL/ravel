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
#include <QSet>
#include <QString>
#include <otf2/otf2.h>

class Function;
class QPainter;
class CommDrawInterface;
class Metrics;
class CommBundle;

class Event
{
public:
    Event(unsigned long long _enter, unsigned long long _exit, int _function,
          unsigned long _entity, unsigned long _pe);
    ~Event();

    // Based on enter time
    bool operator<(const Event &);
    bool operator>(const Event &);
    bool operator<=(const Event &);
    bool operator>=(const Event &);
    bool operator==(const Event &);
    static bool eventEntityLessThan(const Event * evt1, const Event * evt2)
    {
        return evt1->entity < evt2->entity;
    }

    Event * findChild(unsigned long long time);
    unsigned long long getVisibleEnd(unsigned long long start);
    virtual bool isCommEvent() { return false; }
    virtual bool isReceive() const { return false; }
    virtual bool isCollective() { return false; }
    virtual void addComms(QSet<CommBundle *> * bundleset) { Q_UNUSED(bundleset); }

    bool hasMetric(QString name);
    double getMetric(QString name);
    void addMetric(QString name, double event_value);

    // Call tree info
    Event * caller;
    QVector<Event *> * callees;

    unsigned long long enter;
    unsigned long long exit;
    int function;
    unsigned long entity;
    unsigned long pe;
    int depth;

    Metrics * metrics; // Lateness or Counters etc
};

#endif // EVENT_H
