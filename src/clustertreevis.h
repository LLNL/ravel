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
#ifndef CLUSTERTREEVIS_H
#define CLUSTERTREEVIS_H

#include "viswidget.h"

class ClusterTreeVis : public VisWidget
{
    Q_OBJECT
public:
    explicit ClusterTreeVis(QWidget *parent = 0,
                            VisOptions * _options = new VisOptions());
    Gnome * getGnome() { return gnome; }

signals:
    void clusterChange();
    
public slots:
    void setGnome(Gnome * _gnome);
    void clusterChanged();

protected:
    void qtPaint(QPainter *painter);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
    
private:
    Gnome * gnome;
};

#endif // CLUSTERTREEVIS_H
