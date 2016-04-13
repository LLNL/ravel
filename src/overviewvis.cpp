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
void OverviewVis::setSteps(float start, float stop, bool jump)
{
    Q_UNUSED(jump);

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

    int width = size().width() - 2*border;
    startCursor = floor(width * start / 1.0 / maxStep);
    stopCursor = ceil(width * stop / 1.0 / maxStep);
    /* Pre-VIS when this was done with time rather than steps
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
    */

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

    /* PreVis
    startTime = (startCursor / 1.0 / width) * timespan + minTime;
    stopTime = (stopCursor / 1.0 / width) * timespan + minTime;

    int startStep = -1;
    int stopStep = -1 ;
    */
    startStep = floor(maxStep * startCursor / 1.0 / width);
    stopStep = ceil(maxStep * stopCursor / 1.0 / width);

    // We know first is always greater than second
    // Each stepPosition is the range of cursor positions for that step
    // For a range, we want the startStep to be the first one where
    // first < cursor and second > cursor or both are above cursor
    // and stopStep to be the last one where second > cursor or first > cursor
    // and we want to ignore invalid values (where second is -1)
    /* PreVis for (int i = 0; i < stepPositions.size(); i++) {
        if (stepPositions[i].second != -1) {
            if (stepPositions[i].second >= startCursor && startStep == -1)
                startStep = i;
            if (stepPositions[i].second >= stopCursor && stopStep == -1)
                stopStep = i;
        }
    } */
    if (stopStep < 0)
        stopStep = maxStep;
    //std::cout << startStep << ", " << stopStep << " : " << startCursor << ", " << stopCursor << std::endl;
    changeSource = true;
    emit stepsChanged(startStep, stopStep, true);
}


// Upon setting the trace, we determine the min and max that don't change
// We also set the initial cursorStep
void OverviewVis::setTrace(Trace * t)
{
    VisWidget::setTrace(t);
    cacheMetric = options->metric;
    maxStep = trace->global_max_step;
    minTime = ULLONG_MAX;
    maxTime = 0;
    //unsigned long long init_time = ULLONG_MAX;
    //unsigned long long finalize_time = 0;
    // Maybe we should have this be by step instead? We'll see. Right now it
    // uses the full events list which seems unnecessary

    // PreVis commented to change
    /*for (QList<Partition *>::Iterator part = trace->partitions->begin(); part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *>*>::Iterator elist = (*part)->events->begin(); elist != (*part)->events->end(); ++elist)
        {
            for (QList<Event *>::Iterator evt = (elist.value())->begin(); evt != (elist.value())->end(); ++evt)
            {
                if ((*evt)->enter < minTime)
                    minTime = (*evt)->enter;
                if ((*evt)->exit > maxTime)
                    maxTime = (*evt)->exit;
            }

        }

    }
    */

    /*for (QVector<QVector<Event *> *>::Iterator eitr = trace->events->begin(); eitr != trace->events->end(); ++eitr)
    {
        for (QVector<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr)
        {
            if ((*itr)->enter > maxTime)
                maxTime = (*itr)->enter;
            if ((*itr)->exit < minTime)
                minTime = (*itr)->exit;
            if ((*itr)->enter > finalize_time && ((*(trace->functions))[(*itr)->function])->name == "MPI_Finalize")
                finalize_time = (*itr)->enter; // Last MPI_Finalize enter
            if ((*itr)->exit < init_time && ((*(trace->functions))[(*itr)->function])->name == "MPI_Init")
                init_time = (*itr)->exit; // Earliest MPI_Init exit
        }
    }*/

    /*if (finalize_time > 0)
        maxTime = std::min(maxTime, finalize_time);
    if (init_time < ULLONG_MAX)
        minTime = std::max(minTime, init_time);
        */

    startTime = minTime;
    stopTime = minTime;

    // VIS
    startStep = 0;
    stopStep = initStepSpan;
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
    int stepspan = maxStep + 1;
    stepWidth = width / 1.0 / stepspan;
    int start_int, stop_int;
    QString metric = options->metric;
    //stepPositions = QVector<std::pair<int, int> >(maxStep+1, std::pair<int, int>(width + 1, -1));
    for (QList<Partition *>::Iterator part = trace->partitions->begin();
         part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list
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
                float start = (width - 1) * (((*evt)->step) / 1.0 / stepspan);
                float stop = start + stepWidth;
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

                // again for the aggregate
                if ((*evt)->step == 0)
                    continue;
                start = (width - 1) * (((*evt)->step - 1) / 1.0 / stepspan);
                stop = start + stepWidth;
                start_int = static_cast<int>(start);
                stop_int = static_cast<int>(stop); // start_int + i_step_width;

                if ((*evt)->hasMetric(metric)
                        && (*evt)->getMetric(metric, true)> 0)
                {
                    heights[start_int] += (*evt)->getMetric(metric, true)
                                          * (start - start_int);
                    if (stop_int != start_int)
                    {
                        heights[stop_int] += (*evt)->getMetric(metric, true)
                                             * (stop - stop_int);
                    }
                    for (int i = start_int + 1; i < stop_int; i++)
                    {
                        heights[i] += (*evt)->getMetric(metric, true);
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

    startCursor = (startStep) / 1.0 / (maxStep) * width;
    stopCursor = (stopStep) / 1.0 / (maxStep) * width;

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
