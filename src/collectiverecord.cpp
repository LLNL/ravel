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
#include "collectiverecord.h"
#include "commevent.h"
#include "collectiveevent.h"
#include "viswidget.h"

CollectiveRecord::CollectiveRecord(unsigned long long _matching,
                                   unsigned int _root,
                                   unsigned int _collective,
                                   unsigned int _taskgroup)
    : CommBundle(),
      matchingId(_matching),
      root(_root),
      collective(_collective),
      taskgroup(_taskgroup),
      mark(false),
      events(new QList<CollectiveEvent *>())/*,
      times(new QMap<int,
            std::pair<unsigned long long int, unsigned long long int> >())*/

{
}


CommEvent * CollectiveRecord::getDesignee()
{
    return events->first();
}


void CollectiveRecord::draw(QPainter * painter, CommDrawInterface *vis)
{
    vis->drawCollective(painter, this);
}

// Return stride set to this collective. Will return zero if cannot set.
int CollectiveRecord::set_basic_strides()
{
    int max_stride;
    for (QList<CollectiveEvent *>::Iterator evt = events->begin();
         evt != events->end(); ++evt)
    {
        for (QSet<CommEvent *>::Iterator parent = (*evt)->stride_parents->begin();
             parent != (*evt)->stride_parents->end(); ++parent)
        {
            if (!((*parent)->stride)) // Equals zero meaning its unset
                return 0;
            else if ((*parent)->stride > max_stride)
                max_stride = (*parent)->stride;
        }
    }

    max_stride++;
    for (QList<CollectiveEvent *>::Iterator evt = events->begin();
         evt != events->end(); ++evt)
    {
        (*evt)->stride = max_stride;
    }
    return max_stride;
}
