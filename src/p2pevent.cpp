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
#include "p2pevent.h"
#include "commbundle.h"
#include "message.h"
#include "metrics.h"
#include "commdrawinterface.h"
#include <iostream>

P2PEvent::P2PEvent(unsigned long long _enter, unsigned long long _exit,
                   int _function, int _entity, int _pe, int _phase,
                   QVector<Message *> *_messages)
    : CommEvent(_enter, _exit, _function, _entity, _pe, _phase),
      messages(_messages),
      is_recv(false)
{
}

P2PEvent::~P2PEvent()
{
    for (QVector<Message *>::Iterator itr = messages->begin();
         itr != messages->end(); ++itr)
    {
            delete *itr;
            *itr = NULL;
    }
    delete messages;
}

bool P2PEvent::operator<(const P2PEvent &event)
{
    return enter < event.enter;
}

bool P2PEvent::operator>(const P2PEvent &event)
{
    return enter > event.enter;
}

bool P2PEvent::operator<=(const P2PEvent &event)
{
    return enter <= event.enter;
}

bool P2PEvent::operator>=(const P2PEvent &event)
{
    return enter >= event.enter;
}

bool P2PEvent::operator==(const P2PEvent &event)
{
    return enter == event.enter
            && is_recv == event.is_recv;
}


bool P2PEvent::isReceive() const
{
    return is_recv;
}

void P2PEvent::addComms(QSet<CommBundle *> * bundleset)
{
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
        bundleset->insert(*msg);
}
