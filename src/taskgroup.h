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
#ifndef TASKGROUP_H
#define TASKGROUP_H

#include <QString>
#include <QList>
#include <QMap>

class TaskGroup
{
public:
    TaskGroup(int _id, QString _name);
    ~TaskGroup() { delete tasks; }

    int id;
    QString name;
    // May need to keep additional names if we have several that are the same

    // This order is important for communicator rank ID
    QList<unsigned int> * tasks;
    QMap<unsigned int, int> * taskorder;

    // In the future - some sort of hierarchy links between communicators?


};

#endif // TASKGROUP_H
