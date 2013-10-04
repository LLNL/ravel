#include "overviewvis.h"
#include <QPaintEvent>
#include <cfloat>
#include <iostream>

OverviewVis::OverviewVis(QWidget *parent)
{
    VisWidget(parent = parent);

    // GLWidget options
    setMinimumSize(200, height);
    setMaximumHeight(height);
    setAutoFillBackground(true);

    // Set painting variables
    backgroundColor = QBrush(QColor(204, 229, 255));
    visProcessed = false;
    border = 20;
    height = 70;
    trace = NULL;
}

// We use the set steps to find out where the cursor goes in the overview.
void OverviewVis::setSteps(int start, int stop)
{
    startCursor = stepPositions[start].first;
    stopCursor = stepPositions[stop].second;
    repaint();
}

void OverviewVis::resizeEvent(QResizeEvent * event) {
    std::cout << "Resize!" << std::endl;
    visProcessed = false;
    VisWidget::resizeEvent(event);
    processVis();
    repaint();
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
    visProcessed = true;
}

void OverviewVis::paint(QPainter *painter, QPaintEvent *event, int elapsed)
{
    painter->fillRect(event->rect(), backgroundColor);

    if(!visProcessed)
        return;

    QRectF plotBBox = QRectF(border, 0,
                      event->rect().width()-2*border,
                      event->rect().height()-border);

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

    // Draw latenes
    painter->setPen(QPen(Qt::red, 1, Qt::SolidLine));
    for(int i = 0; i < heights.size(); i++)
    {
        p1 = QPointF(o.x() + i, o.y());
        p2 = QPointF(o.x() + i, o.y() - heights[i]);
        painter->drawLine(p1, p2);
    }
}
