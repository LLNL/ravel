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
#include "rawtrace.h"

#include "primaryentitygroup.h"
#include "entity.h"
#include "eventrecord.h"
#include "commrecord.h"
#include "entitygroup.h"
#include "otfcollective.h"
#include "collectiverecord.h"
#include "function.h"
#include "counter.h"
#include "counterrecord.h"
#include <stdint.h>


RawTrace::RawTrace(int nt, int np)
    : primaries(NULL),
      processingElements(NULL),
      functionGroups(NULL),
      functions(NULL),
      events(NULL),
      messages(NULL),
      messages_r(NULL),
      entitygroups(NULL),
      collective_definitions(NULL),
      counters(NULL),
      counter_records(NULL),
      collectives(NULL),
      collectiveMap(NULL),
      collectiveBits(NULL),
      num_entities(nt),
      num_pes(np),
      second_magnitude(1),
      metric_names(NULL),
      metric_units(NULL)
{

}

// Note we do not delete the function/functionGroup map because
// we know that will get passed to the processed trace
RawTrace::~RawTrace()
{
    for (QVector<QVector<EventRecord *> *>::Iterator eitr = events->begin();
         eitr != events->end(); ++eitr)
    {
        for (QVector<EventRecord *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete events;

    for (QVector<QVector<CommRecord *> *>::Iterator eitr = messages->begin();
         eitr != messages->end(); ++eitr)
    {
        for (QVector<CommRecord *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete messages;
    delete messages_r;

    for (QVector<QVector<CollectiveBit *> *>::Iterator eitr = collectiveBits->begin();
         eitr != collectiveBits->end(); ++eitr)
    {
        for (QVector<CollectiveBit *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete collectiveBits;

    if (metric_names)
        delete metric_names;
    if (metric_units)
        delete metric_units;
}
