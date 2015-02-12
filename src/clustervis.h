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
#ifndef CLUSTERVIS_H
#define CLUSTERVIS_H

#include "timelinevis.h"

class ClusterTreeVis;
class PartitionCluster;

class ClusterVis : public TimelineVis
{
    Q_OBJECT
public:
    ClusterVis(ClusterTreeVis * ctv, QWidget* parent = 0,
               VisOptions *_options = new VisOptions());
    ~ClusterVis() {}
    void setTrace(Trace * t);

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);

signals:
    void focusGnome(); // used by clustertreevis
    void clusterChange(); // for cluster selection
    void neighborChange(int); // change neighborhood radius for top processes

public slots:
    void setSteps(float start, float stop, bool jump = false);
    void clusterChanged(); // for cluster selection
    void changeNeighborRadius(int neighbors); // neighborhood radius of top processes
    void selectEvent(Event * event, bool aggregate, bool overdraw);

protected:
    void qtPaint(QPainter *painter);
    void drawNativeGL();
    void paintEvents(QPainter *painter);
    void prepaint();
    void mouseDoubleClickEvent(QMouseEvent * event);

    QMap<Gnome *, QRect> drawnGnomes;
    PartitionCluster * selected;
    ClusterTreeVis * treevis;
    Gnome * hover_gnome;

};

#endif // CLUSTERVIS_H
