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
#ifndef EXCHANGEGNOME_H
#define EXCHANGEGNOME_H

#include "gnome.h"
#include <QMap>
#include <QSet>
#include <QRect>

class QPainter;
class PartitionCluster;

// Gnome with detector & special drawing functions for exchange patterns
class ExchangeGnome : public Gnome
{
public:
    ExchangeGnome();

    bool detectGnome(Partition * part);
    Gnome * create();
    void preprocess();

protected:
    void drawGnomeQtClusterEnd(QPainter * painter, QRect clusterRect,
                               PartitionCluster * pc,
                               int barwidth, int barheight, int blockwidth,
                               int blockheight, int startStep);
    void generateTopTasks();

private:
    // send-receive, sends-receives, sends-waitall
    enum ExchangeType { EXCH_SRSR, EXCH_SSRR, EXCH_SSWA, EXCH_UNKNOWN };
    ExchangeType type;
    ExchangeType findType();

    // Trying to grow SRSR patterns to capture large number of them
    // can be used for automatically determining neighborhood size
    QMap<int, int> SRSRmap;
    QSet<int> SRSRpatterns;

    int maxWAsize; // for drawing Waitall pies

    void drawGnomeQtClusterSRSR(QPainter * painter, QRect startxy,
                                PartitionCluster * pc, int barwidth,
                                int barheight, int blockwidth, int blockheight,
                                int startStep);
    void drawGnomeQtClusterSSWA(QPainter * painter, QRect startxy,
                                PartitionCluster * pc, int barwidth,
                                int barheight, int blockwidth, int blockheight,
                                int startStep);


};

#endif // EXCHANGEGNOME_H
