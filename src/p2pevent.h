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
#ifndef P2PEVENT_H
#define P2PEVENT_H

#include "commevent.h"

class QPainter;
class CommDrawInterface;

class P2PEvent : public CommEvent
{
public:
    P2PEvent(unsigned long long _enter, unsigned long long _exit,
             int _function, int _entity, int _pe, int _phase,
             QVector<Message *> * _messages = NULL);
    ~P2PEvent();

    // Based on enter time, add_order & receive-ness
    bool operator<(const P2PEvent &);
    bool operator>(const P2PEvent &);
    bool operator<=(const P2PEvent &);
    bool operator>=(const P2PEvent &);
    bool operator==(const P2PEvent &);


    int comm_count(QMap<Event *, int> *memo = NULL) { Q_UNUSED(memo); return 1; }
    bool isP2P() { return true; }
    bool isReceive() const;

    CommEvent * compare_to_sender(CommEvent * prev);

    void addComms(QSet<CommBundle *> * bundleset);
    QVector<Message *> * getMessages() { return messages; }


    // Messages involved wiht this event
    QVector<Message *> * messages;

    bool is_recv;
};

#endif // P2PEVENT_H
