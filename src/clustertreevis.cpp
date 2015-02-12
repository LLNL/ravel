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
#include "clustertreevis.h"
#include "gnome.h"

ClusterTreeVis::ClusterTreeVis(QWidget *parent, VisOptions * _options)
    : VisWidget(parent, _options),
      gnome(NULL)
{
    backgroundColor = palette().color(QPalette::Background);
    setAutoFillBackground(true);
}

// We do everything based on the active gnome which is set here.
// This vis does not keep track of anything else like the others do
void ClusterTreeVis::setGnome(Gnome * _gnome)
{
    gnome = _gnome;
    //repaint();
}

// We'll let the active gnome handle this
void ClusterTreeVis::qtPaint(QPainter *painter)
{
    if (!visProcessed)
        return;

    if (gnome)
    {
        gnome->drawTopLabels(painter, rect());
        gnome->drawQtTree(painter, rect());
    }

}

// Pass to active gnome
void ClusterTreeVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (gnome)
    {
        gnome->handleTreeDoubleClick(event);
        changeSource = true;
        emit(clusterChange());
        repaint();
    }
}

// Sometimes this is the same on systems, sometimes not
void ClusterTreeVis::mousePressEvent(QMouseEvent * event)
{
    mouseDoubleClickEvent(event);
}

// Active cluster/gnome to paint
void ClusterTreeVis::clusterChanged()
{
    if (changeSource)
    {
        changeSource = false;
        return;
    }

    repaint();
}
