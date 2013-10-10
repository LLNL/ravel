#include "overviewvis.h"
#include <QPaintEvent>
#include <cfloat>
#include <iostream>
#include <cmath>

OverviewVis::OverviewVis(QWidget *parent)
{
    VisWidget(parent = parent);
    height = 70;

    // GLWidget options
    setMinimumSize(200, height);
    setMaximumHeight(height);
    setAutoFillBackground(false);

    // Set painting variables
    backgroundColor = QBrush(QColor(204, 229, 255));
    visProcessed = false;
    border = 20;
    trace = NULL;
    mousePressed = false;
}

// We use the set steps to find out where the cursor goes in the overview.
void OverviewVis::setSteps(float start, float stop)
{
    startCursor = stepPositions[static_cast<int>(round(start))].first;
    stopCursor = stepPositions[static_cast<int>(round(stop))].second;
    startTime = (startCursor / 1.0  / (size().width() - 2 * border)) * (maxTime - minTime) + minTime;
    stopTime = (stopCursor / 1.0  / (size().width() - 2 * border)) * (maxTime - minTime) + minTime;
    repaint();
}

void OverviewVis::resizeEvent(QResizeEvent * event) {
    visProcessed = false;
    VisWidget::resizeEvent(event);
    processVis();
    repaint();
}

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
    startTime = (startCursor / 1.0  / (size().width() - 2 * border)) * (maxTime - minTime) + minTime;
    stopTime = (stopCursor / 1.0  / (size().width() - 2 * border)) * (maxTime - minTime) + minTime;
    int start = ((startCursor - 0.5) / (size().width() - 2 * border)) * (maxTime - minTime) + minTime;
    int stop = ((stopCursor + 0.5) / (size().width() - 2 * border)) * (maxTime - minTime) + minTime;
    int currentstart = maxTime;
    int currentstop = minTime;
    int startStep = -1;
    int stopStep = -1;
    for (int i = 0; i < stepPositions.size(); i++) {
        if (stepPositions[i].first >= start && stepPositions[i].first < currentstart) {
            currentstart = stepPositions[i].first;
            startStep = i;
        }
        if (stepPositions[i].second <= stop && stepPositions[i].second < currentstop) {
            currentstop = stepPositions[i].second;
            stopStep = i;
        }
    }
    emit stepsChanged(startStep, stopStep);
}


// Upon setting the trace, we determine the min and max that don't change
// We also set the initial cursorStep
void OverviewVis::setTrace(Trace * t)
{
    VisWidget::setTrace(t);
    maxStep = 0;
    minTime = ULLONG_MAX;
    maxTime = 0;
    for (QVector<Event *>::Iterator itr = trace->events->begin(); itr != trace->events->end(); itr++) {
        if ((*itr)->step > maxStep)
            maxStep = (*itr)->step;
        if ((*itr)->exit > maxTime)
            maxTime = (*itr)->exit;
        if ((*itr)->enter < minTime)
            minTime = (*itr)->enter;
    }
    stepPositions = QVector<std::pair<int, int> >(maxStep+1, std::pair<int, int>(-1, -1));
    startTime = minTime;
    stopTime = minTime;
}

void OverviewVis::processVis()
{
    // Don't do anything if there's no trace available
    if (trace == NULL)
        return;

    int width = size().width() - 2 * border;
    heights = QVector<float>(width, 0);
    int timespan = maxTime - minTime;
    for (QVector<Event *>::Iterator itr = trace->events->begin(); itr != trace->events->end(); itr++) {
        // Accumulate lateness on the overview. We assume overview is real time
        if ((*itr)->lateness > 0) {
            float start = (width - 1) * (((*itr)->enter - minTime) / 1.0 / timespan);
            float stop = (width - 1) * (((*itr)->exit - minTime) / 1.0 / timespan);
            int start_int = static_cast<int>(start);
            int stop_int = static_cast<int>(stop);
            heights[start_int] += (*itr)->lateness * (start - start_int);
            if (stop_int != start_int) {
                heights[stop_int] += (*itr)->lateness * (stop - stop_int);
            }
            for (int i = start_int + 1; i < stop_int; i++) {
                heights[i] += (*itr)->lateness;
            }
        }
        // Figure out where the steps fall in width
        if ((*itr)->step >= 0) {
            int stepMax = width * ((*itr)->step / 1.0 / maxStep);
            if (stepPositions[(*itr)->step].second < stepMax)
                stepPositions[(*itr)->step].second = stepMax;
            if (stepPositions[(*itr)->step].first > stepMax)
                stepPositions[(*itr)->step].first = stepMax;
        }
    }
    float minLateness = FLT_MAX;
    float maxLateness = 0;
    for (int i = 0; i < width; i++) {
        if (heights[i] > maxLateness)
            maxLateness = heights[i];
        if (heights[i] < minLateness)
            minLateness = heights[i];
    }

    int maxHeight = height - border;
    for (int i = 0; i < width; i++) {
        heights[i] = maxHeight * (heights[i] - minLateness) / maxLateness;
    }

    startCursor = (startTime - minTime) / 1.0 / (maxTime - minTime) * width;
    stopCursor = (stopTime - minTime) / 1.0 / (maxTime - minTime) * width;

    visProcessed = true;
}

void OverviewVis::paint(QPainter *painter, QPaintEvent *event, int elapsed)
{
    painter->fillRect(rect(), backgroundColor);

    if(!visProcessed)
        return;

    QRectF plotBBox(border, 0,
                      rect().width()-2*border,
                      rect().height()-border);

    // Draw axes
    painter->setBrush(QBrush(QColor(0,0,0)));
    painter->drawLine(plotBBox.bottomLeft(),plotBBox.bottomRight());

    // Draw axis labels
    /*painter->drawText(plotBBox.bottomLeft()+QPointF(0,10),data->meta[xdim]);
    painter->save();
    painter->translate(plotBBox.bottomLeft()-QPointF(5,0));
    painter->rotate(270);
    painter->drawText(QPointF(0,0),data->meta[ydim]);
    painter->restore();
    */

    QPointF o = plotBBox.bottomLeft();
    QPointF p1, p2;

    // Draw lateness
    painter->setPen(QPen(Qt::red, 1, Qt::SolidLine));
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
    QRectF selection(plotBBox.bottomLeft().x() + startSelect,
                     plotBBox.bottomLeft().y() - height + border, // starting top left
                     stopSelect - startSelect,
                     height - border);
    painter->fillRect(selection, QBrush(QColor(255, 255, 144, 100)));
    painter->drawRect(selection);
}
