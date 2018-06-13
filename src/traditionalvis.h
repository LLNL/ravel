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
#ifndef TRADITIONALVIS_H
#define TRADITIONALVIS_H

#include "timelinevis.h"
#include <QVector>

class CommEvent;
class CommBundle;

// Physical timeline
class TraditionalVis : public TimelineVis
{
    Q_OBJECT
public:
    TraditionalVis(QWidget * parent = 0,
                   VisOptions *_options = new VisOptions());
    ~TraditionalVis();
    void setTrace(Trace * t);

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void rightDrag(QMouseEvent * event);
    void rightClickEvent(QMouseEvent * event);

    void drawMessage(QPainter * painter, Message * message);
    void drawCollective(QPainter * painter, CollectiveRecord * cr);

signals:
    void timeScaleString(QString);
    void taskPropertyDisplay(Event *);

public slots:
    void setTime(float start, float stop, bool jump = false);

protected:
    void qtPaint(QPainter *painter);

    // Paint MPI events we handle directly and thus can color.
    void paintEvents(QPainter *painter);

    void prepaint();
    void drawNativeGL();

    // Paint all other events available
    void paintNotStepEvents(QPainter *painter, Event * evt, float position,
                            int entity_spacing, float barheight,
                            float blockheight, QRect * extents, QSet<CommBundle *> *drawComms, QSet<CommBundle *> *selectedComms);
    void paintNotStepEventsGL(Event * evt,
                              float position, float barheight,
                              QVector<GLfloat> * bars,
                              QVector<GLfloat> * colors);

private:
    // For keeping track of map betewen real time and step time
    class TimePair {
    public:
        TimePair(unsigned long long _s1, unsigned long long _s2)
            : start(_s1), stop(_s2) {}

        unsigned long long start;
        unsigned long long stop;
    };

    double minTime;
    double maxTime;
    double startTime;
    double timeSpan;
    QRect lassoRect;
    float blockheight;

    int getX(CommEvent * evt);
    int getY(CommEvent * evt);
    int getW(CommEvent * evt);
};

#endif // TRADITIONALVIS_H
