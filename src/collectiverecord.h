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
#ifndef COLLECTIVERECORD_H
#define COLLECTIVERECORD_H

#include <QMap>
#include "commbundle.h"

class CollectiveEvent;

// Information we get from OTF about collectives
class CollectiveRecord : public CommBundle
{
public:
    CollectiveRecord(unsigned long long int _matching, unsigned int _root,
                     unsigned int _collective, unsigned int _taskgroup);

    unsigned long long int matchingId;
    unsigned int root;
    unsigned int collective;
    unsigned int taskgroup;
    bool mark;

    // Map from process to enter/leave times
    //QMap<int, std::pair<unsigned long long, unsigned long long> > * times;
    QList<CollectiveEvent *> * events;

    CommEvent * getDesignee();
    void draw(QPainter * painter, CommDrawInterface * vis);

    int set_basic_strides();
};

#endif // COLLECTIVERECORD_H
