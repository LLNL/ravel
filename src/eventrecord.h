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
#ifndef EVENTRECORD_H
#define EVENTRECORD_H

#include <QList>
#include <QString>
#include <QMap>

class Event;

// Holder for OTF Event info
class EventRecord
{
public:
    EventRecord(unsigned long _entity, unsigned long long int _t, unsigned int _v, bool _e = true);
    ~EventRecord();

    unsigned long entity;
    unsigned long long int time;
    unsigned int value;
    bool enter;
    QList<Event *> children;
    QMap<QString, unsigned long long> * metrics;
    QMap<QString, int> * ravel_info;

    // Based on time
    bool operator<(const EventRecord &);
    bool operator>(const EventRecord &);
    bool operator<=(const EventRecord &);
    bool operator>=(const EventRecord &);
    bool operator==(const EventRecord &);
};

#endif // EVENTRECORD_H
