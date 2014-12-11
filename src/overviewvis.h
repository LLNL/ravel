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
#ifndef OVERVIEWVIS_H
#define OVERVIEWVIS_H

#include "viswidget.h"
#include <QVector>

class VisOptions;
class QResizeEvent;
class QMouseEvent;
class QPainter;
class Trace;

// Full timeline shown by metric
class OverviewVis : public VisWidget
{
    Q_OBJECT
public:
    OverviewVis(QWidget *parent = 0, VisOptions *_options = new VisOptions());
    void setTrace(Trace * t);
    void processVis();
    void resizeEvent(QResizeEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

public slots:
    void setSteps(float start, float stop, bool jump = false);

protected:
    void qtPaint(QPainter *painter);

private:
    int roundeven(float step);

    QString cacheMetric; // So we can tell if metric changes

    // We can do this by time or steps, currently steps
    unsigned long long minTime;
    unsigned long long maxTime;
    int maxStep;
    int startCursor;
    int stopCursor;
    unsigned long long startTime;
    unsigned long long stopTime;
    int startStep;
    int stopStep;
    float stepWidth;
    int height;
    bool mousePressed;
    QVector<float> heights; // bar heights

    // Map between cursor position and steps
    QVector<std::pair<int, int> > stepPositions;
};

#endif // OVERVIEWVIS_H
