#include "overviewvis.h"
#include <QPaintEvent>
#include <cfloat>
#include <iostream>
#include <cmath>

OverviewVis::OverviewVis(QWidget *parent) : VisWidget(parent = parent)
{
    height = 70;

    // GLWidget options
    setMinimumSize(200, height);
    setMaximumHeight(height);

    // Set painting variables
    backgroundColor = QColor(204, 229, 255);
    trace = NULL;
    mousePressed = false;
}

int OverviewVis::roundeven(float step)
{
    int rounded = floor(step);
    if (rounded % 2 == 1)
        rounded += 1;
    while (rounded > maxStep)
        rounded -= 2;
    return rounded;
}

// We use the set steps to find out where the cursor goes in the overview.
void OverviewVis::setSteps(float start, float stop)
{
    if (!visProcessed)
        return;

    if (changeSource) {
        changeSource = false;
        return;
    }
    int startStep = roundeven(start);
    int stopStep = roundeven(stop);
    if (startStep < 0)
        startStep = 0;
    if (stopStep > maxStep)
        stopStep = maxStep;
    startCursor = stepPositions[startStep].first;
    while (startCursor >= rect().width()) // For no-data steps
    {
        startStep -= 2;
        startCursor = stepPositions[startStep].second;
    }
    stopCursor = stepPositions[stopStep].second;
    while (stopCursor < 0) // For no-data steps
    {
        stopStep += 2;
        stopCursor = stepPositions[stopStep].second;
    }
    startTime = (startCursor / 1.0  / (size().width() - 2 * border)) * (maxTime - minTime) + minTime;
    stopTime = (stopCursor / 1.0  / (size().width() - 2 * border)) * (maxTime - minTime) + minTime;
    //std::cout << startCursor << ", " << stopCursor << std::endl;
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

    // We know first is always greater than second
    // Each stepPosition is the range of cursor positions for that step
    // For a range, we want the startStep to be the first one where first < cursor and second > cursor or
    // both are above cursor
    // and stopStep to be the last one where second > cursor or first > cursor
    // and we want to ignore invalid values (where second is -1)
    for (int i = 0; i < stepPositions.size(); i++) {
        if (stepPositions[i].second != -1) {
            if (stepPositions[i].second >= startCursor && startStep == -1)
                startStep = i;
            if (stepPositions[i].second >= stopCursor && stopStep == -1)
                stopStep = i;
        }
    }
    if (stopStep < 0)
        stopStep = maxStep;
    //std::cout << startStep << ", " << stopStep << " : " << startCursor << ", " << stopCursor << std::endl;
    changeSource = true;
    emit stepsChanged(startStep, stopStep);
}


// Upon setting the trace, we determine the min and max that don't change
// We also set the initial cursorStep
void OverviewVis::setTrace(Trace * t)
{
    VisWidget::setTrace(t);
    maxStep = trace->global_max_step;
    minTime = ULLONG_MAX;
    maxTime = 0;
    // Maybe we should have this be by step instead? We'll see. Right now it uses the full events list which seems
    // unnecessary
    for (QVector<QVector<Event *> *>::Iterator eitr = trace->events->begin(); eitr != trace->events->end(); ++eitr) {
        for (QVector<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            if ((*itr)->enter > maxTime && ((*(trace->functions))[(*itr)->function])->name == "MPI_Finalize")
                maxTime = (*itr)->enter; // Last MPI_Finalize enter
            if ((*itr)->exit < minTime && ((*(trace->functions))[(*itr)->function])->name == "MPI_Init")
                minTime = (*itr)->exit; // Earliest MPI_Init exit
        }
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
    unsigned long long int timespan = maxTime - minTime;
    int start_int, stop_int;
    QString lateness("Lateness");
    stepPositions = QVector<std::pair<int, int> >(maxStep+1, std::pair<int, int>(width + 1, -1));
    for (QList<Partition *>::Iterator part = trace->partitions->begin(); part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list) {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt) {
                // Accumulate lateness on the overview. We assume overview is real time
                // Note we're not counting aggregate lateness here. That would require that our vector is
                // in order and keeping track of the last for each process. We don't want to do that right now.

                // start and stop are the cursor positions
                float start = (width - 1) * (((*evt)->enter - minTime) / 1.0 / timespan);
                float stop = (width - 1) * (((*evt)->exit - minTime) / 1.0 / timespan);
                start_int = static_cast<int>(start);
                stop_int = static_cast<int>(stop);
                Q_ASSERT((*evt)->metrics->contains(lateness));
                if ((*(*evt)->metrics)[lateness]->event > 0) {
                    heights[start_int] += (*(*evt)->metrics)[lateness]->event * (start - start_int);
                    if (stop_int != start_int) {
                        heights[stop_int] += (*(*evt)->metrics)[lateness]->event * (stop - stop_int);
                    }
                    for (int i = start_int + 1; i < stop_int; i++) {
                        heights[i] += (*(*evt)->metrics)[lateness]->event;
                    }

                }

                // Figure out where the steps fall in width for later
                // stepPositions maps steps to position in the cursor
                if ((*evt)->step >= 0) {
                    if (stepPositions[(*evt)->step].second < stop_int)
                        stepPositions[(*evt)->step].second = stop_int;
                    if (stepPositions[(*evt)->step].first > start_int)
                        stepPositions[(*evt)->step].first = start_int;
                }
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

void OverviewVis::qtPaint(QPainter *painter)
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
