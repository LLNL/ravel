#include "overviewvis.h"
#include <QPaintEvent>
#include <cfloat>
#include <iostream>
#include <cmath>

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
    /* PreVis
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
    // For a range, we want the startStep to be the first one where first < cursor and second > cursor or
    // both are above cursor
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
    maxStep = trace->global_max_step;
    minTime = ULLONG_MAX;
    maxTime = 0;
    //unsigned long long init_time = ULLONG_MAX;
    //unsigned long long finalize_time = 0;
    // Maybe we should have this be by step instead? We'll see. Right now it uses the full events list which seems
    // unnecessary

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

void OverviewVis::processVis()
{
    // Don't do anything if there's no trace available
    if (trace == NULL)
        return;
    int width = size().width() - 2 * border;
    heights = QVector<float>(width, 0);
    int stepspan = maxStep;
    stepWidth = width / 1.0 / stepspan;
    int i_step_width = int(stepWidth);
    int start_int, stop_int;
    QString metric = options->metric;
    //stepPositions = QVector<std::pair<int, int> >(maxStep+1, std::pair<int, int>(width + 1, -1));
    for (QList<Partition *>::Iterator part = trace->partitions->begin(); part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list) {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt) {
                // Accumulate lateness on the overview. We assume overview is real time
                // Note we're not counting aggregate lateness here. That would require that our vector is
                // in order and keeping track of the last for each process. We don't want to do that right now.

                // start and stop are the cursor positions
                float start = (width - 1) * (((*evt)->step) / 1.0 / stepspan);
                float stop = start + stepWidth;
                start_int = static_cast<int>(start);
                stop_int = static_cast<int>(stop); // start_int + i_step_width;
                //std::cout << start_int << " from " << start << " from enter " << (*evt)->enter;
                //std::cout << " and mintime " << minTime <<  " over " << width << " and timespan " << timespan << std::endl;
                //Q_ASSERT((*evt)->metrics->contains(lateness));
                if ((*evt)->hasMetric(metric) && (*evt)->getMetric(metric)> 0) {
                    heights[start_int] += (*evt)->getMetric(metric) * (start - start_int);
                    if (stop_int != start_int) {
                        heights[stop_int] += (*evt)->getMetric(metric) * (stop - stop_int);
                    }
                    for (int i = start_int + 1; i < stop_int; i++) {
                        heights[i] += (*evt)->getMetric(metric);
                    }

                }

                // Figure out where the steps fall in width for later
                // stepPositions maps steps to position in the cursor
                /*if ((*evt)->step >= 0) {
                    if (stepPositions[(*evt)->step].second < stop_int)
                        stepPositions[(*evt)->step].second = stop_int;
                    if (stepPositions[(*evt)->step].first > start_int)
                        stepPositions[(*evt)->step].first = start_int;
                }*/
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

    startCursor = (startStep) / 1.0 / (maxStep) * width;
    stopCursor = (stopStep) / 1.0 / (maxStep) * width;

    visProcessed = true;

    /* PreVis change to steps
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
                //std::cout << start_int << " from " << start << " from enter " << (*evt)->enter;
                //std::cout << " and mintime " << minTime <<  " over " << width << " and timespan " << timespan << std::endl;
                //Q_ASSERT((*evt)->metrics->contains(lateness));
                if ((*evt)->hasMetric(lateness) && (*evt)->getMetric(lateness)> 0) {
                    heights[start_int] += (*evt)->getMetric(lateness) * (start - start_int);
                    if (stop_int != start_int) {
                        heights[stop_int] += (*evt)->getMetric(lateness) * (stop - stop_int);
                    }
                    for (int i = start_int + 1; i < stop_int; i++) {
                        heights[i] += (*evt)->getMetric(lateness);
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
    */

    visProcessed = true;
}

void OverviewVis::qtPaint(QPainter *painter)
{
    painter->fillRect(rect(), QWidget::palette().color(QWidget::backgroundRole()));

    if(!visProcessed)
        return;

    QRectF plotBBox(border, 5,
                      rect().width()-2*border,
                      rect().height() - 10);
                      //rect().height()-timescaleHeight);


    // Draw axes
    //drawTimescale(painter, minTime, maxTime - minTime, border);
    painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
    painter->drawLine(border, rect().height() - 4, rect().width() - border, rect().height() - 4);

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
    QRectF selection(plotBBox.bottomLeft().x() + startSelect,
                     plotBBox.bottomLeft().y() - height + border, // starting top left
                     stopSelect - startSelect,
                     height - border);
    painter->fillRect(selection, QBrush(QColor(255, 255, 144, 100)));
    painter->drawRect(selection);
}
