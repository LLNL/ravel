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
#include "trace.h"

#include <iostream>
#include <fstream>
#include <QElapsedTimer>
#include <QTime>
#include <cmath>
#include <climits>
#include <cfloat>

#include "entity.h"
#include "event.h"
#include "commevent.h"
#include "p2pevent.h"
#include "collectiveevent.h"
#include "function.h"
#include "entitygroup.h"
#include "otfcollective.h"
#include "ravelutils.h"
#include "primaryentitygroup.h"
#include "metrics.h"

Trace::Trace(int nt, int np)
    : name(""),
      fullpath(""),
      num_entities(nt),
      num_pes(np),
      units(-9),
      totalTime(0), // for paper timing
      metrics(new QList<QString>()),
      metric_units(new QMap<QString, QString>()),
      functionGroups(new QMap<int, QString>()),
      functions(new QMap<int, Function *>()),
      primaries(NULL),
      processingElements(NULL),
      entitygroups(NULL),
      collective_definitions(NULL),
      collectives(NULL),
      collectiveMap(NULL),
      events(new QVector<QVector<Event *> *>(std::max(nt, np))),
      roots(new QVector<QVector<Event *> *>(std::max(nt, np))),
      mpi_group(-1),
      max_time(0),
      min_time(ULLONG_MAX),
      isProcessed(false),
      totalTimer(QElapsedTimer())
{
    for (int i = 0; i < std::max(nt, np); i++) {
        (*events)[i] = new QVector<Event *>();
    }

    for (int i = 0; i < std::max(nt, np); i++)
    {
        (*roots)[i] = new QVector<Event *>();
    }
}

Trace::~Trace()
{
    delete metrics;
    delete metric_units;
    delete functionGroups;

    for (QMap<int, Function *>::Iterator itr = functions->begin();
         itr != functions->end(); ++itr)
    {
        delete (itr.value());
        itr.value() = NULL;
    }
    delete functions;

    for (QVector<QVector<Event *> *>::Iterator eitr = events->begin();
         eitr != events->end(); ++eitr)
    {
        for (QVector<Event *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete events;

    // Don't need to delete Events, they were deleted above
    for (QVector<QVector<Event *> *>::Iterator eitr = roots->begin();
         eitr != roots->end(); ++eitr)
    {
        delete *eitr;
        *eitr = NULL;
    }
    delete roots;

    for (QMap<int, EntityGroup *>::Iterator comm = entitygroups->begin();
         comm != entitygroups->end(); ++comm)
    {
        delete *comm;
        *comm = NULL;
    }
    delete entitygroups;

    for (QMap<int, OTFCollective *>::Iterator cdef = collective_definitions->begin();
         cdef != collective_definitions->end(); ++cdef)
    {
        delete *cdef;
        *cdef = NULL;
    }
    delete collective_definitions;

    for (QMap<unsigned long long, CollectiveRecord *>::Iterator coll = collectives->begin();
         coll != collectives->end(); ++coll)
    {
        delete *coll;
        *coll = NULL;
    }
    delete collectives;

    delete collectiveMap;

    for (QMap<int, PrimaryEntityGroup *>::Iterator primary = primaries->begin();
         primary != primaries->end(); ++primary)
    {
        for (QList<Entity *>::Iterator entity = primary.value()->entities->begin();
             entity != primary.value()->entities->end(); ++entity)
        {
            delete *entity;
            *entity = NULL;
        }
        delete primary.value();
    }
    delete primaries;
}

void Trace::preprocess()
{
    isProcessed = true;
}

// Find the smallest event in a timeline that contains the given time
Event * Trace::findEvent(int entity, unsigned long long time)
{
    Event * found = NULL;
    for (QVector<Event *>::Iterator root = roots->at(entity)->begin();
         root != roots->at(entity)->end(); ++root)
    {
        found = (*root)->findChild(time);
        if (found)
            return found;
    }

    return found;
}
