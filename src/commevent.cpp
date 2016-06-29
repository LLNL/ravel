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
#include "commevent.h"
#include <otf2/OTF2_AttributeList.h>
#include <otf2/OTF2_GeneralDefinitions.h>
#include <iostream>
#include "metrics.h"

CommEvent::CommEvent(unsigned long long _enter, unsigned long long _exit,
                     int _function, int _entity, int _pe, int _phase)
    : Event(_enter, _exit, _function, _entity, _pe),
      comm_next(NULL),
      comm_prev(NULL),
      true_next(NULL),
      true_prev(NULL),
      pe_next(NULL),
      pe_prev(NULL),
      phase(_phase),
      gvid("")
{
}

CommEvent::~CommEvent()
{
}


bool CommEvent::operator<(const CommEvent &event)
{
    return enter < event.enter;
}

bool CommEvent::operator>(const CommEvent &event)
{
    return enter > event.enter;
}

bool CommEvent::operator<=(const CommEvent &event)
{
    return enter <= event.enter;
}

bool CommEvent::operator>=(const CommEvent &event)
{
    return enter >= event.enter;
}

bool CommEvent::operator==(const CommEvent &event)
{
    return enter == event.enter
            && isReceive() == event.isReceive();
}

bool CommEvent::hasMetric(QString name)
{
    if (metrics->hasMetric(name))
        return true;
    else if (caller && caller->metrics->hasMetric(name))
        return true;
    else
        return false;
}

double CommEvent::getMetric(QString name)
{
    if (metrics->hasMetric(name))
        return metrics->getMetric(name);

    if (caller && caller->metrics->hasMetric(name))
        return caller->metrics->getMetric(name);

    return 0;
}

