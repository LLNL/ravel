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
#ifndef TIMELINEVIS_H
#define TIMELINEVIS_H

#include "viswidget.h"

// Parent class for those who pan and zoom like a timeline view
class TimelineVis : public VisWidget
{
    Q_OBJECT
public:
    TimelineVis(QWidget* parent = 0, VisOptions * _options = new VisOptions());
    ~TimelineVis();
    void processVis();

    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseDoubleClickEvent(QMouseEvent * event);
    void leaveEvent(QEvent * event);
    virtual void rightDrag(QMouseEvent * event) { Q_UNUSED(event); }

public slots:
    virtual void selectEvent(Event * event, bool aggregate, bool overdraw);
    void selectTasks(QList<int> tasks, Gnome *gnome);

protected:
    void drawHover(QPainter *painter);
    void drawTaskLabels(QPainter * painter, int effectiveHeight,
                           float barHeight);

    bool jumped;
    bool mousePressed;
    bool rightPressed;
    int mousex;
    int mousey;
    int pressx;
    int pressy;
    float stepwidth;
    float taskheight;
    int labelWidth;
    int labelHeight;
    int labelDescent;
    int cursorWidth;

    int maxStep;
    int startPartition;
    float startStep;
    float startTask; // refers to order rather than process really
    float stepSpan;
    float taskSpan;
    float lastStartStep;
    QMap<int, int> proc_to_order;
    QMap<int, int> order_to_proc;

    static const int spacingMinimum = 12;

};
#endif // TIMELINEVIS_H
