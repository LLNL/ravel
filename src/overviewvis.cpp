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
#include "overviewvis.h"
#include <QPaintEvent>
#include <cfloat>
#include <iostream>
#include <cmath>
#include "trace.h"
#include "rpartition.h"
#include "event.h"
#include "commevent.h"

OverviewVis::OverviewVis(QWidget *parent, VisOptions * _options)
    : VisWidget(parent = parent, _options = _options)
{
    height = 70;

    // GLWidget options
    setMinimumSize(200, height);
    setMaximumHeight(height);

    // Set painting variables
    //backgroundColor = QColor(204, 229, 255);
    mousePressed = false;
}

// When you click that maps to a global step, but we
// find things every other step due to aggregated events,
// so we have to get the event step
int OverviewVis::roundeven(float time)
{
    int rounded = floor(step);
    if (rounded % 2 == 1)
        rounded += 1;
    while (rounded > maxTime)
        rounded -= 2;
    return rounded;
}

// We use the set steps to find out where the cursor goes in the overview.
void OverviewVis::setTime(float start, float stop, bool jump)
{
    Q_UNUSED(jump);

    if (!visProcessed)
        return;

    if (changeSource) {
        changeSource = false;
        return;
    }
    startTime = roundeven(start);
    stopTime = roundeven(stop);
    if (startTime < minTime)
        startTime = minTime;
    if (stopTime > maxTime)
        stopTime = maxTime;

    int width = size().width() - 2*border;
    startCursor = floor(width * start / 1.0 / maxTime);
    stopCursor = ceil(width * stop / 1.0 / maxTime);

    if (!closed)
        repaint();
}

void OverviewVis::resizeEvent(QResizeEvent * event) {
    visProcessed = false;
    VisWidget::resizeEvent(event);
    processVis();
    repaint();
}

// Functions for brushing
void OverviewVis::mousePressEvent(QMouseEvent * event) {
    startCursor = event->x() - border;
    stopCursor = startCursor;
    mousePressed = true;
    repaint();
}

void OverviewVis::mouseMoveEvent(QMouseEvent * event) {
    if (mousePressed) {
        stopCursor = event->x() - border;
        repaint();
    }
}

void OverviewVis::mouseReleaseEvent(QMouseEvent * event) {
    mousePressed = false;
    stopCursor = event->x() - border;
    if (startCursor > stopCursor) {
        int tmp = startCursor;
        startCursor = stopCursor;
        stopCursor = tmp;
    }
    repaint();

    // PreVis int timespan = maxTime - minTime;
    int width = size().width() - 2 * border;

    startTime = (startCursor / 1.0 / width) * timespan + minTime;
    stopTime = (stopCursor / 1.0 / width) * timespan + minTime;

    changeSource = true;
    emit timeChanged(startTime, stopTime, true);
}


// Upon setting the trace, we determine the min and max that don't change
// We also set the initial cursorStep
void OverviewVis::setTrace(Trace * t)
{
    VisWidget::setTrace(t);
    cacheMetric = options->metric;
    maxTime = trace->max_time;
    minTime = trace->min_time;


    startTime = minTime;
    stopTime = initTimeSpan + minTime;
}


// Calculate what the heights should be for the metrics
// TODO: Save this stuff per step somewhere and have that be accessed as needed
// rather than iterating through everything
void OverviewVis::processVis()
{
    // Don't do anything if there's no trace available
    if (trace == NULL)
        return;
    int width = size().width() - 2 * border;
    heights = QVector<float>(width, 0);
    int timeSpan = maxTime + 1;
    timeWidth = width / 1.0 / timeSpan;
    int start_int, stop_int;
    QString metric = options->metric;
    //stepPositions = QVector<std::pair<int, int> >(maxStep+1, std::pair<int, int>(width + 1, -1));
    for (QList<Partition *>::Iterator part = trace->partitions->begin();
         part != trace->partitions->end(); ++part)
    {
        for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list
             = (*part)->events->begin();
             event_list != (*part)->events->end(); ++event_list)
        {
            // For each event, we figure out which steps it spans and then we
            // accumulate height over those steps based on the event's metric
            // value
            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                // start and stop are the cursor positions
                float start = (width - 1) * (((*evt)->enter) / 1.0 / timeSpan);
                float stop = start + timeWidth;
                start_int = static_cast<int>(start);
                stop_int = static_cast<int>(stop);

                if ((*evt)->hasMetric(metric) && (*evt)->getMetric(metric)> 0)
                {
                    heights[start_int] += (*evt)->getMetric(metric)
                                          * (start - start_int);
                    if (stop_int != start_int) {
                        heights[stop_int] += (*evt)->getMetric(metric)
                                             * (stop - stop_int);
                    }
                    for (int i = start_int + 1; i < stop_int; i++)
                    {
                        heights[i] += (*evt)->getMetric(metric);
                    }

                }      
            }
        }
    }
    float minMetric = FLT_MAX;
    float maxMetric = 0;
    for (int i = 0; i < width; i++) {
        if (heights[i] > maxMetric)
            maxMetric = heights[i];
        if (heights[i] < minMetric)
            minMetric = heights[i];
    }


    int maxHeight = height - border;
    for (int i = 0; i < width; i++) {
        heights[i] = maxHeight * (heights[i] - minMetric) / maxMetric;
    }

    startCursor = (startTime) / 1.0 / (maxTime) * width;
    stopCursor = (stopTime) / 1.0 / (maxTime) * width;

    visProcessed = true;
}

void OverviewVis::qtPaint(QPainter *painter)
{
    painter->fillRect(rect(), QWidget::palette().color(QWidget::backgroundRole()));

    if(!visProcessed)
        return;

    if (options->metric != cacheMetric)
    {
        cacheMetric = options->metric;
        processVis();
    }

    QRectF plotBBox(border, 5,
                      rect().width()-2*border,
                      rect().height() - 10);

    // Draw axes
    //drawTimescale(painter, minTime, maxTime - minTime, border);
    painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
    painter->drawLine(border, rect().height() - 4,
                      rect().width() - border, rect().height() - 4);

    QPointF o = plotBBox.bottomLeft();
    QPointF p1, p2;

    // Draw lateness
    painter->setPen(QPen(QColor(127, 0, 0), 1, Qt::SolidLine));
    for(int i = 0; i < heights.size(); i++)
    {
        p1 = QPointF(o.x() + i, o.y());
        p2 = QPointF(o.x() + i, o.y() - heights[i]);
        painter->drawLine(p1, p2);
    }

    // Draw selection
    int startSelect = startCursor;
    int stopSelect = stopCursor;
    if (startSelect > stopSelect) {
        int tmp = startSelect;
        startSelect = stopSelect;
        stopSelect = tmp;
    }
    painter->setPen(QPen(QColor(255, 255, 144, 150)));
    painter->setBrush(QBrush(QColor(255, 255, 144, 100)));

    // Starts at top left for drawing
    QRectF selection(plotBBox.bottomLeft().x() + startSelect,
                     plotBBox.bottomLeft().y() - height + border,
                     stopSelect - startSelect,
                     height - border);
    painter->fillRect(selection, QBrush(QColor(255, 255, 144, 100)));
    painter->drawRect(selection);
}
