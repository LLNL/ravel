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
    if (changeSource) {
        changeSource = false;
        return;
    }
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

    int timespan = maxTime - minTime;
    int width = size().width() - 2 * border;

    startTime = (startCursor / 1.0 / width) * timespan + minTime;
    stopTime = (stopCursor / 1.0 / width) * timespan + minTime;
    int startStep = -1;
    int stopStep = -1 ;
    std::cout << startCursor << ", " << stopCursor << std::endl;

    // We know first is always greater than second
    // Each stepPosition is the range of cursor positions for that step
    // For a range, we want the startStep to be the first one where first < cursor and second > cursor or
    // both are above cursor
    // and stopStep to be the last one where second > cursor or first > cursor
    // and we want to ignore invalid values (where second is -1)
    for (int i = 0; i < stepPositions.size(); i++) {
        std::cout << i << " :  " << stepPositions[i].first << ", " << stepPositions[i].second << std::endl;
        std::cout << startStep << ", " << stopStep << std::endl;
        if (stepPositions[i].second != -1) {
            if (stepPositions[i].second >= startCursor && startStep == -1)
                startStep = i;
            if (stepPositions[i].second >= stopCursor && stopStep == -1)
                stopStep = i;
        }
    }
    changeSource = true;
    std::cout << startStep << ", " << stopStep << ", " << maxStep << std::endl;
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
    stepPositions = QVector<std::pair<int, int> >(maxStep+1, std::pair<int, int>(width + 1, -1));
    for (QVector<Event *>::Iterator itr = trace->events->begin(); itr != trace->events->end(); itr++) {
        // Accumulate lateness on the overview. We assume overview is real time
        if ((*itr)->lateness > 0) {
            // start and stop are the cursor positions
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

            // Figure out where the steps fall in width for later
            // stepPositions maps steps to position in the cursor
            if ((*itr)->step >= 0) {
                if (stepPositions[(*itr)->step].second < stop_int)
                    stepPositions[(*itr)->step].second = stop_int;
                if (stepPositions[(*itr)->step].first > start_int)
                    stepPositions[(*itr)->step].first = start_int;
            }
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
