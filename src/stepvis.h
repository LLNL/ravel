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
#ifndef STEPVIS_H
#define STEPVIS_H

#include "timelinevis.h"

class MetricRangeDialog;
class CommEvent;

// Logical timeline vis
class StepVis : public TimelineVis
{
    Q_OBJECT
public:
    StepVis(QWidget* parent = 0, VisOptions *_options = new VisOptions());
    ~StepVis();
    void setTrace(Trace * t);

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void rightDrag(QMouseEvent * event);

    // Saves colorbar range information
    MetricRangeDialog * metricdialog;

    int getHeight() { return rect().height() - colorBarHeight; }
    void drawMessage(QPainter * painter, Message * message);
    void drawCollective(QPainter * painter, CollectiveRecord * cr);

public slots:
    void setSteps(float start, float stop, bool jump = false);
    void setMaxMetric(long long int new_max);

protected:
    void qtPaint(QPainter *painter);
    void drawNativeGL();
    void paintEvents(QPainter *painter);
    void prepaint();
    void overdrawSelected(QPainter *painter, QList<int> tasks);
    void drawColorBarGL();
    void drawColorBarText(QPainter * painter);
    void drawCollective(QPainter * painter, CollectiveRecord * cr,
                        int ellipse_width, int ellipse_height,
                        int effectiveHeight);
    void drawLine(QPainter * painter, QPointF * p1, QPointF * p2);
    void drawArc(QPainter * painter, QPointF * p1, QPointF * p2,
                 int width, bool forward = true);
    void setupMetric();
    void drawColorValue(QPainter * painter);
    int getX(CommEvent * evt);
    int getY(CommEvent * evt);

private:
    double maxMetric;
    QString cacheMetric;
    QString maxMetricText;
    QString hoverText;
    int maxMetricTextWidth;
    float colorbar_offset;
    QRect lassoRect;
    float blockwidth;
    float blockheight;
    int ellipse_width;
    int ellipse_height;
    QMap<int, int> * overdrawYMap;

    static const int colorBarHeight = 24;
};

#endif // STEPVIS_H
