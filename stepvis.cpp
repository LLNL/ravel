#include "stepvis.h"
#include <iostream>

StepVis::StepVis(QWidget* parent, VisOptions * _options)
    : TimelineVis(parent = parent, _options),
      metricdialog(NULL),
    maxMetric(0),
    cacheMetric(""),
    maxMetricText(""),
    hoverText(""),
    maxMetricTextWidth(0),
    colorbar_offset(0),
    lassoRect(QRect()),
    blockwidth(0),
    blockheight(0),
    ellipse_width(0),
    ellipse_height(0)
{

}

StepVis::~StepVis()
{

}

void StepVis::setTrace(Trace * t)
{
    VisWidget::setTrace(t);
    // Initial conditions
    if (options->showAggregateSteps)
        startStep = -1;
    else
        startStep = 0;
    stepSpan = initStepSpan;
    startProcess = 0;
    processSpan = trace->num_processes;
    startPartition = 0;

    maxStep = trace->global_max_step;
    setupMetric();
}

void StepVis::setupMetric()
{
    // Find the maximum of a metric -- TODO: Move this into trace as a lookup
    // some day before we do tiling
    maxMetric = 0;
    QString metric(options->metric);
    for (QList<Partition *>::Iterator part = trace->partitions->begin();
         part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list
             = (*part)->events->begin();
             event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                if ((*evt)->hasMetric(metric))
                {
                    if ((*evt)->getMetric(metric) > maxMetric)
                        maxMetric = (*evt)->getMetric(metric);
                    if ((*evt)->getMetric(metric, true) > maxMetric)
                        maxMetric = (*evt)->getMetric(metric, true);
                }
            }
        }
    }
    options->setRange(0, maxMetric);
    cacheMetric = options->metric;

    // For colorbar
    QLocale systemlocale = QLocale::system();
    if (options->colormap->isCategorical())
        maxMetricText = "";
    else
        maxMetricText = systemlocale.toString(maxMetric)
                        + trace->metric_units->value(options->metric);

    // For colorbar range
    delete metricdialog;
    metricdialog = new MetricRangeDialog(this, maxMetric, maxMetric);
    connect(metricdialog, SIGNAL(valueChanged(long long int)), this,
            SLOT(setMaxMetric(long long int)));
    metricdialog->hide();
}


void StepVis::setMaxMetric(long long int new_max)
{
    options->setRange(0, new_max);
    QLocale systemlocale = QLocale::system();
    if (options->colormap->isCategorical())
        maxMetricText = "";
    else
        maxMetricText = systemlocale.toString(new_max)
                        + trace->metric_units->value(options->metric);
    repaint();
}

// SLOT for coordinated step changing
void StepVis::setSteps(float start, float stop, bool jump)
{
    if (!visProcessed)
        return;

    if (changeSource) {
        changeSource = false;
        return;
    }
    lastStartStep = startStep;
    startStep = start;
    stepSpan = stop - start;
    jumped = jump;

    if (!closed)
        repaint();
}

// Rectangle selection graphic
void StepVis::rightDrag(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    if (pressx < event->x())
    {
        startStep = startStep + stepSpan * pressx / rect().width();
        stepSpan = stepSpan * (event->x() - pressx) / rect().width();
    }
    else
    {
        startStep = startStep + stepSpan * event->x() / rect().width();
        stepSpan = stepSpan * (pressx - event->x()) / rect().width();
    }

    if (pressy < event->y())
    {
        startProcess = startProcess + processSpan * pressy
                       / (rect().height() - colorBarHeight);
        processSpan = processSpan * (event->y() - pressy)
                      / (rect().height() - colorBarHeight);
    }
    else
    {
        startProcess = startProcess + processSpan * event->y()
                       / (rect().height() - colorBarHeight);
        processSpan = processSpan * (pressy - event->y())
                      / (rect().height() - colorBarHeight);
    }
    repaint();
    changeSource = true;
    emit stepsChanged(startStep, startStep + stepSpan, false);
}

void StepVis::mouseMoveEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    lassoRect = QRect();
    if (mousePressed && !rightPressed) // For panning
    {
        lastStartStep = startStep;
        int diffx = mousex - event->x();
        int diffy = mousey - event->y();
        startStep += diffx / 1.0 / stepwidth;
        startProcess += diffy / 1.0 / processheight;

        if (options->showAggregateSteps)
        {
            if (startStep < -1)
                startStep = -1;
        }
        else
        {
            if (startStep < 0)
                startStep = 0;
        }
        if (startStep > maxStep)
            startStep = maxStep;

        if (startProcess < 0)
            startProcess = 0;
        if (startProcess + processSpan > trace->num_processes)
            startProcess = trace->num_processes - processSpan;

        mousex = event->x();
        mousey = event->y();

        repaint();
        changeSource = true;
        emit stepsChanged(startStep, startStep + stepSpan, false);
    }
    else if (mousePressed && rightPressed) // For rectangle selection
    {
        lassoRect = QRect(std::min(pressx, event->x()),
                          std::min(pressy, event->y()),
                          abs(pressx - event->x()),
                          abs(pressy - event->y()));
        repaint();
    }
    else // potential hover - check event vs aggregate event vs colorbar area
    {
        mousex = event->x();
        mousey = event->y();
        hoverText = "";
        if (mousey >= rect().height() - colorBarHeight)
        {
            if (event->x() < rect().width() - colorbar_offset
                && event->x() > colorbar_offset)
            {
                long long int colormax = options->colormap->getMax();
                hoverText = QString::number(int(colormax
                                                * (event->x() - colorbar_offset)
                                                / (rect().width()
                                                   - 2 * colorbar_offset)));
                repaint();
            }
        }
        else if (options->showAggregateSteps && hover_event
                 && drawnEvents[hover_event].contains(mousex, mousey))
        {
            if (!hover_aggregate && mousex
                <= drawnEvents[hover_event].x() + stepwidth)
            {
                hover_aggregate = true;
                repaint();
            }
            else if (hover_aggregate && mousex >=  drawnEvents[hover_event].x()
                     + stepwidth)
            {
                hover_aggregate = false;
                repaint();
            }
        }
        else if (hover_event == NULL
                 || !drawnEvents[hover_event].contains(mousex, mousey))
        {
            hover_event = NULL;
            for (QMap<Event *, QRect>::Iterator evt = drawnEvents.begin();
                 evt != drawnEvents.end(); ++evt)
            {
                if (evt.value().contains(mousex, mousey))
                {
                    hover_aggregate = false;
                    if (options->showAggregateSteps && mousex
                        <= evt.value().x() + stepwidth)
                    {
                        hover_aggregate = true;
                    }
                    hover_event = evt.key();
                }
            }

            repaint();
        }
    }

}

// Zoom
void StepVis::wheelEvent(QWheelEvent * event)
{
    if (!visProcessed)
        return;

    float scale = 1;
    int clicks = event->delta() / 8 / 15;
    scale = 1 + clicks * 0.05;
    if (Qt::MetaModifier && event->modifiers()) {
        // Vertical
        float avgProc = startProcess + processSpan / 2.0;
        processSpan *= scale;
        startProcess = avgProc - processSpan / 2.0;
    } else {
        // Horizontal
        lastStartStep = startStep;
        float avgStep = startStep + stepSpan / 2.0;
        stepSpan *= scale;
        startStep = avgStep - stepSpan / 2.0;
    }
    repaint();
    changeSource = true;
    emit stepsChanged(startStep, startStep + stepSpan, false);
}

void StepVis::prepaint()
{
    drawnEvents.clear();
    if (jumped) // We have to redo the active_partitions
    {
        // We know this list is in order, so we only have to go so far
        //int topStep = boundStep(startStep + stepSpan) + 1;
        int bottomStep = floor(startStep) - 1;
        Partition * part = NULL;
        for (int i = 0; i < trace->partitions->length(); ++i)
        {
            part = trace->partitions->at(i);
            if (part->max_global_step >= bottomStep)
            {
                startPartition = i;
                break;
            }
        }
    }
    else // We nudge the active partitions as necessary
    {
        int bottomStep = floor(startStep) - 1;
        Partition * part = NULL;
        if (startStep < lastStartStep) // check earlier partitions
        {
            // Keep setting the one before until its right
            for (int i = startPartition; i >= 0; --i)
            {
                part = trace->partitions->at(i);
                if (part->max_global_step >= bottomStep)
                {
                    startPartition = i;
                }
                else
                {
                    break;
                }
            }
        }
        else if (startStep > lastStartStep) // check current partitions up
        {
            // As soon as we find one, we're good
            for (int i = startPartition; i < trace->partitions->length(); ++i)
            {
                part = trace->partitions->at(i);
                if (part->max_global_step >= bottomStep)
                {
                    startPartition = i;
                    break;
                }
            }
        }
    }

    // If the metric has changed, we have to redo this
    if (trace && options->metric.compare(cacheMetric) != 0)
        setupMetric();
}

void StepVis::qtPaint(QPainter *painter)
{
    if(!visProcessed)
        return;

    // In this case we haven't already drawn stuff with GL, so we paint it here.
    if ((rect().height() - colorBarHeight) / processSpan >= 3
        && rect().width() / stepSpan >= 3)
    {
      paintEvents(painter);
    }

    // Always done by qt
    drawProcessLabels(painter, rect().height() - colorBarHeight,
                      processheight);
    drawColorBarText(painter);

    // Hover is independent of how we drew things
    drawHover(painter);
    drawColorValue(painter);

    // Lasso drawing is on top
    if (!lassoRect.isNull())
    {
        painter->setPen(Qt::yellow);
        painter->drawRect(lassoRect);
        painter->fillRect(lassoRect, QBrush(QColor(255, 255, 144, 150)));
    }
}


void StepVis::drawNativeGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!visProcessed)
        return;

    drawColorBarGL();

    int effectiveHeight = rect().height() - colorBarHeight;
    if (effectiveHeight / processSpan >= 3 && rect().width() / stepSpan >= 3)
        return;

    QString metric(options->metric);

    // Setup viewport
    int width = rect().width() - labelWidth;
    int height = effectiveHeight;
    float effectiveSpan = stepSpan;
    if (!(options->showAggregateSteps))
        effectiveSpan /= 2.0;

    glViewport(labelWidth,
               colorBarHeight,
               width,
               height);
    glLoadIdentity();
    glOrtho(0, effectiveSpan, 0, processSpan, 0, 1);

    float barwidth = 1.0;
    float barheight = 1.0;
    processheight = height/ processSpan;
    stepwidth = width / effectiveSpan;


    double num_events = 0;
    Partition * part = NULL;
    int topStep = boundStep(startStep + stepSpan) + 1;
    int bottomStep = floor(startStep) - 1;

    // Based on the number of events, determine how much overplotting we
    // should do
    for (int i = startPartition; i < trace->partitions->length(); ++i)
    {
        part = trace->partitions->at(i);
        if (part->min_global_step > topStep)
            break;
        else if (part->max_global_step < bottomStep)
            continue;
        // Twice (each event is both it and its aggregate) of the
        // step overlap with the view, divided by how many steps are
        // spanned by the partition
        double overlap = 2 * (part->max_global_step - part->min_global_step + 2
                              - std::max(0, part->max_global_step - topStep)
                              - std::max(0, bottomStep - part->min_global_step))
                              / (part->max_global_step - part->min_global_step + 2);
        num_events += part->num_events() * overlap;
    }

    // 1 if an event at every step, small if less
    double density = num_events / 1.0 / (stepSpan * processSpan);
    float opacity = 1.0;
    float xoffset = 0;
    float yoffset = 0;

    // How we use density is by feel, we should improve this.
    // If density is one, we want low opacity for proper blending.
    // If density is very small, we want high opacity to make things
    // more visible.
    if (density)
    {
        if (processheight < 1.0)
        {
            double processes_per_pixel = 1.0 / processheight;
            yoffset = processes_per_pixel / 2;
            opacity = processheight / density;
        }
        if (stepwidth < 1.0)
        {
            double steps_per_pixel = 1.0 / stepwidth;
            xoffset = steps_per_pixel / 2;
            if (stepwidth / density > opacity)
                opacity = stepwidth / density;
        }
    }

    // Generate buffers to hold each bar. We don't know how many there will
    // be since we draw one per event.
    QVector<GLfloat> bars = QVector<GLfloat>();
    QVector<GLfloat> colors = QVector<GLfloat>();

    // Process events for values
    float x, y; // true position
    float position; // placement of process
    QColor color;
    float maxProcess = processSpan + startProcess;
    float myopacity, opacity_multiplier = 1.0;
    if (selected_gnome && !selected_processes.isEmpty())
        opacity_multiplier = 0.50;
    for (int i = startPartition; i < trace->partitions->length(); ++i)
    {
        part = trace->partitions->at(i);
        if (part->min_global_step > topStep)
            break;
        else if (part->max_global_step < bottomStep)
            continue;
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list = part->events->begin();
             event_list != part->events->end(); ++event_list)
        {
            bool selected = false;
            if (part->gnome == selected_gnome
                && selected_processes.contains(proc_to_order[event_list.key()]))
            {
                selected = true;
            }

            position = proc_to_order[event_list.key()];
            // Out of process span check
            if (position < floor(startProcess)
                || position > ceil(startProcess + processSpan))
            {
                 continue;
            }
            y = (maxProcess - position) * barheight - 1;

            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                // Out of step span test
                if ((*evt)->step < bottomStep || (*evt)->step > topStep)
                    continue;

                // Calculate position of this bar in float space
                if (options->showAggregateSteps)
                    x = ((*evt)->step - startStep) * barwidth;
                else
                    x = ((*evt)->step - startStep) / 2 * barwidth;

                color = options->colormap->color((*(*evt)->metrics)[metric]->event);
                if (selected)
                    myopacity = opacity;
                else
                    myopacity = opacity * opacity_multiplier;

                bars.append(x - xoffset);
                bars.append(y - yoffset);
                bars.append(x - xoffset);
                bars.append(y + barheight + yoffset);
                bars.append(x + barwidth + xoffset);
                bars.append(y + barheight + yoffset);
                bars.append(x + barwidth + xoffset);
                bars.append(y - yoffset);
                for (int j = 0; j < 4; ++j)
                {
                    colors.append(color.red() / 255.0);
                    colors.append(color.green() / 255.0);
                    colors.append(color.blue() / 255.0);
                    colors.append(myopacity);
                }


                if (options->showAggregateSteps) // repeat!
                {
                    x = ((*evt)->step - startStep - 1) * barwidth;
                    if (x + barwidth <= 0)
                        continue;

                    color = options->colormap->color((*(*evt)->metrics)[metric]->aggregate);

                    bars.append(x - xoffset);
                    bars.append(y - yoffset);
                    bars.append(x - xoffset);
                    bars.append(y + barheight + yoffset);
                    bars.append(x + barwidth + xoffset);
                    bars.append(y + barheight + yoffset);
                    bars.append(x + barwidth + xoffset);
                    bars.append(y - yoffset);
                    for (int j = 0; j < 4; ++j)
                    {
                        colors.append(color.red() / 255.0);
                        colors.append(color.green() / 255.0);
                        colors.append(color.blue() / 255.0);
                        colors.append(myopacity);
                    }
                }

            }
        }
    }

    // Draw
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColorPointer(4,GL_FLOAT,0,colors.constData());
    glVertexPointer(2,GL_FLOAT,0,bars.constData());
    glDrawArrays(GL_QUADS,0,bars.size()/2);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}


// Qt Event painting
void StepVis::paintEvents(QPainter * painter)
{
    // Figure out block sizes. Need integers for qtpaint to align right
    int effectiveHeight = rect().height() - colorBarHeight;
    int effectiveWidth = rect().width();

    int process_spacing = 0;
    if (effectiveHeight / processSpan > spacingMinimum)
        process_spacing = 3;

    int step_spacing = 0;
    if (effectiveWidth / stepSpan > spacingMinimum)
        step_spacing = 3;

    float x, y, w, h, xa, wa;
    blockheight = floor(effectiveHeight / processSpan);
    if (options->showAggregateSteps)
    {
        blockwidth = floor(effectiveWidth / stepSpan);
    }
    else
    {
        blockwidth = floor(effectiveWidth / (ceil(stepSpan / 2.0)));
    }
    float barheight = blockheight - process_spacing;
    float barwidth = blockwidth - step_spacing;
    processheight = blockheight;
    stepwidth = blockwidth;
    QRect extents = QRect(0, 0, rect().width(), effectiveHeight);

    QString metric(options->metric);
    int position;
    bool complete, aggcomplete;
    QSet<CommBundle *> drawComms = QSet<CommBundle *>();
    painter->setPen(QPen(QColor(0, 0, 0)));
    Partition * part = NULL;
    int topStep = boundStep(startStep + stepSpan) + 1;
    int bottomStep = floor(startStep) - 1;
    float myopacity, opacity = 1.0;
    if (selected_gnome && !selected_processes.isEmpty())
        opacity = 0.50;

    // Only do partitions in our range
    for (int i = startPartition; i < trace->partitions->length(); ++i)
    {
        part = trace->partitions->at(i);
        if (part->min_global_step > topStep)
            break;
        else if (part->max_global_step < bottomStep)
            continue;

        // Go through events in partition
        for (QMap<int, QList<CommEvent *> *>::Iterator event_list
             = part->events->begin();
             event_list != part->events->end(); ++event_list)
        {
            bool selected = false;
            if (part->gnome == selected_gnome
                && selected_processes.contains(proc_to_order[event_list.key()]))
            {
                selected = true;
            }

            // Out of span test
            position = proc_to_order[event_list.key()];
            if (position < floor(startProcess)
                || position > ceil(startProcess + processSpan))
            {
                continue;
            }
            y = floor((position - startProcess) * blockheight) + 1;

            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                 // Out of step span test
                if ((*evt)->step < bottomStep || (*evt)->step > topStep)
                {
                    continue;
                }
                // 0 = startProcess, effectiveHeight = stopProcess (startProcess + processSpan)
                // 0 = startStep, rect().width() = stopStep (startStep + stepSpan)

                x = getX(*evt);
                w = barwidth;
                h = barheight;

                // Corrections for partially drawn
                complete = true;
                if (y < 0) {
                    h = barheight - fabs(y);
                    y = 0;
                    complete = false;
                } else if (y >= effectiveHeight) {
                    continue;
                } else if (y + barheight > effectiveHeight) {
                    h = effectiveHeight - y;
                    complete = false;
                }
                if (x < 0) {
                    w = barwidth + x;
                    x = 0;
                    complete = false;
                } else if (x + barwidth > rect().width()) {
                    w = rect().width() - x;
                    complete = false;
                }

                myopacity = opacity;
                if (selected)
                    myopacity = 1.0;
                painter->setPen(QPen(QColor(0, 0, 0, myopacity*255)));
                // Draw the event
                if ((*evt)->hasMetric(metric))
                    painter->fillRect(QRectF(x, y, w, h),
                                      QBrush(options->colormap->color((*evt)->getMetric(metric),
                                                                      myopacity)));
                else
                    painter->fillRect(QRectF(x, y, w, h),
                                      QBrush(QColor(180, 180, 180)));
                // Change pen color if selected
                if (*evt == selected_event && !selected_aggregate)
                    painter->setPen(QPen(Qt::yellow));
                // Draw border only if we're doing spacing, otherwise too messy
                if (step_spacing > 0 && process_spacing > 0)
                {
                    if (complete)
                        painter->drawRect(QRectF(x,y,w,h));
                    else
                        incompleteBox(painter, x, y, w, h, &extents);
                }
                // Revert pen color
                if (*evt == selected_event && !selected_aggregate)
                    painter->setPen(QPen(QColor(0, 0, 0)));


                // Save messages for the end since they draw on top
                (*evt)->addComms(&drawComms);

                // Draw aggregate events if necessary
                if (options->showAggregateSteps) {
                    xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1
                         + labelWidth;
                    wa = barwidth;
                    if (xa + wa <= 0)
                        continue;

                    aggcomplete = true;
                    if (xa < 0) {
                        wa = barwidth + xa;
                        xa = 0;
                        aggcomplete = false;
                    } else if (xa + barwidth > rect().width()) {
                        wa = rect().width() - xa;
                        aggcomplete = false;
                    }

                    aggcomplete = aggcomplete && complete;
                    if ((*evt)->hasMetric(metric))
                        painter->fillRect(QRectF(xa, y, wa, h),
                                          QBrush(options->colormap->color((*evt)->getMetric(metric,
                                                                                            true),
                                                                          myopacity)));
                    else
                        painter->fillRect(QRectF(xa, y, wa, h),
                                          QBrush(QColor(180, 180, 180)));

                    if (*evt == selected_event && selected_aggregate)
                        painter->setPen(QPen(Qt::yellow));
                    if (step_spacing > 0 && process_spacing > 0)
                        if (aggcomplete)
                            painter->drawRect(QRectF(xa, y, wa, h));
                        else
                            incompleteBox(painter, xa, y, wa, h, &extents);
                    if (*evt == selected_event && selected_aggregate)
                        painter->setPen(QPen(QColor(0, 0, 0)));

                    // For selection
                    drawnEvents[*evt] = QRect(xa, y, (x - xa) + w, h);
                } else {
                    // For selection
                    drawnEvents[*evt] = QRect(x, y, w, h);
                }

            }
        }
    }

    // Messages
    // We need to do all of the message drawing after the event drawing
    // for overlap purposes
    if (options->showMessages != VisOptions::NONE)
    {
        if (blockwidth / 5 > 0)
            ellipse_width = blockwidth / 5;
        else
            ellipse_width = 3;
        if (blockheight / 5 > 0)
            ellipse_height = blockheight / 5;
        else
            ellipse_height = 3;

        for (QSet<CommBundle *>::Iterator comm = drawComms.begin();
             comm != drawComms.end(); ++comm)
        {
            (*comm)->draw(painter, this);
        }
    }
}

int StepVis::getY(CommEvent * evt)
{
    int y = 0;
    int position = proc_to_order[evt->process];
    y = floor((position - startProcess) * blockheight) + 1;
    return y;
}

int StepVis::getX(CommEvent *evt)
{
    int x = 0;
    if (options->showAggregateSteps) // Factor these calcs into own fxn!
        x = floor((evt->step - startStep) * blockwidth) + 1
            + labelWidth;
    else
        x = floor((evt->step - startStep) / 2 * blockwidth) + 1
            + labelWidth;

    return x;
}

void StepVis::drawMessage(QPainter * painter, Message * msg)
{
    if (processSpan <= 32)
        painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
    else
        painter->setPen(QPen(Qt::black, 1, Qt::SolidLine));

    QPointF p1, p2;
    int y = getY(msg->sender);
    int x = getX(msg->sender);
    int w = blockwidth;
    int h = blockheight;

    if (options->showMessages == VisOptions::TRUE)
    {
        p1 = QPointF(x + w/2.0, y + h/2.0);
        y = getY(msg->receiver);
        x = getX(msg->receiver);
        p2 = QPointF(x + w/2.0, y + h/2.0);
    }
    else
    {
        p1 = QPointF(x, y + h/2.0);
        y = getY(msg->receiver);
        p2 = QPointF(x + w, y + h/2.0);
    }
    drawLine(painter, &p1, &p2);
}


void StepVis::drawCollective(QPainter * painter, CollectiveRecord * cr)
{
    int root, x, y, prev_x, prev_y, root_x, root_y;
    CollectiveEvent * coll_event;
    QPointF p1, p2;

    int ell_w = ellipse_width;
    int ell_h = ellipse_height;
    int coll_type = (*(trace->collective_definitions))[cr->collective]->type;
    int root_offset = 0;
    bool rooted = false;
    int w = blockwidth;
    int h = blockheight;

    // Rooted
    if (coll_type == 2 || coll_type == 3)
    {
        //root = (*(trace->communicators))[(*cr)->communicator]->processes->at((*cr)->root);
        rooted = true;
        root = cr->root;
        root_offset = ell_w;
        if (coll_type == 2)
        {
            root_offset *= -1;
        }
    }

    painter->setPen(QPen(Qt::darkGray, 1, Qt::SolidLine));
    painter->setBrush(QBrush(Qt::darkGray));
    coll_event = cr->events->at(0);
    prev_y = getY(coll_event);
    prev_x = getX(coll_event);

    if (rooted)
    {
        if (coll_event->process == root)
        {
            painter->setBrush(QBrush());
            prev_x += root_offset;
            root_x = prev_x;
            root_y = prev_y;
        }
        else
            prev_x -= root_offset;
    }
    painter->drawEllipse(prev_x + w/2 - ell_w/2,
                         prev_y + h/2 - ell_h/2,
                         ell_w, ell_h);

    for (int i = 1; i < cr->events->size(); i++)
    {
        painter->setPen(QPen(Qt::darkGray, 1, Qt::SolidLine));
        painter->setBrush(QBrush(Qt::darkGray));
        coll_event = cr->events->at(i);

        if (rooted && coll_event->process == root)
        {
            painter->setBrush(QBrush());
        }

        y = getY(coll_event);
        x = getX(coll_event);
        if (rooted)
        {
            if (coll_event->process == root)
                x += root_offset;
            else
                x -= root_offset;
        }
        painter->drawEllipse(x + w/2 - ell_w/2, y + h/2 - ell_h/2,
                             ell_w, ell_h);

        // Arc style
        painter->setPen(QPen(Qt::black, 1, Qt::DashLine));
        painter->setBrush(QBrush());
        if (coll_type == 1) // BARRIER
        {
            p1 = QPointF(prev_x + w/2.0, prev_y + h/2.0);
            p2 = QPointF(x + w/2.0, y + h/2.0);
            drawArc(painter, &p1, &p2, ell_w * 1.5);
        }
        else if (rooted && coll_event->process == root)
        {
            // ONE2ALL / ALL2ONE drawing handled after the loop
            root_x = x;
            root_y = y;
        }
        else if (coll_type == 4) // ALL2ALL
        {
            // First Arc
            p1 = QPointF(prev_x + w/2.0, prev_y + h/2.0);
            p2 = QPointF(x + w/2.0, y + h/2.0);
            drawArc(painter, &p1, &p2, ell_w * 1.5);
            drawArc(painter, &p1, &p2, ell_w * -1.5, false);
        }

        prev_x = x;
        prev_y = y;
    }

    if (rooted)
    {
        painter->setPen(QPen(Qt::black, 1, Qt::DashLine));
        painter->setBrush(QBrush());
        CollectiveEvent * other;
        int ox, oy;
        p1 = QPointF(root_x + w/2, root_y + h/2);
        for (int j = 0; j < cr->events->size(); j++)
        {
            other = cr->events->at(j);
            if (other->process == root)
                continue;

            oy = getY(other);
            ox = getX(other) - root_offset;

            p2 = QPointF(ox + w/2, oy + h/2);
            drawLine(painter, &p1, &p2);
        }
    }
}

// Figure out the angles necessary to draw an arc from point p1 above
// point p2 that is not wider than radius and stops at the
// effectiveHeight.
void StepVis::drawArc(QPainter * painter, QPointF * p1, QPointF * p2,
                      int width, bool forward)
{
    QPainterPath path;
    int effectiveHeight = rect().height() - colorBarHeight;
    // handle effectiveheight -- find correct span angle & start so
    // the arc doesn't draw past effective height
    if (p2->y() > effectiveHeight)
    {
        //
    }

    QRectF bounding(p1->x() - width, p1->y(), width*2, p2->y() - p1->y());
    path.moveTo(p1->x(), p1->y());
    if (forward)
        path.arcTo(bounding, 90, -180);
    else
        path.arcTo(bounding, 90, 180);
    painter->drawPath(path);
}

// This is for drawing the message lines. It determines whether the line will
// exceed the drawing area on the bottom and thus potentially overwrite the
// color bar. If so, it shortens the line while maintaining its slope.
void StepVis::drawLine(QPainter * painter, QPointF * p1, QPointF * p2)
{
    int effectiveHeight = rect().height() - colorBarHeight;
    if (p1->y() > effectiveHeight && p2->y() > effectiveHeight)
    {
        return;
    }
    else if (p1->y() > effectiveHeight)
    {
        float slope = float(p1->y() - p2->y()) / (p1->x() - p2->x());
        float intercept = p1->y() - slope * p1->x();
        painter->drawLine(QPointF((effectiveHeight - intercept) / slope,
                                  effectiveHeight), *p2);
    }
    else if (p2->y() > effectiveHeight)
    {
        float slope = float(p1->y() - p2->y()) / (p1->x() - p2->x());
        float intercept = p1->y() - slope * p1->x();
        painter->drawLine(*p1, QPointF((effectiveHeight - intercept) / slope,
                                       effectiveHeight));
    }
    else
    {
        painter->drawLine(*p1, *p2);
    }
}

void StepVis::drawColorBarGL()
{
    // Setup stuff for overlay like the minimaps
    glEnable(GL_SCISSOR_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glScissor(0, 0, rect().width(), colorBarHeight);
    glViewport(0, 0, rect().width(), colorBarHeight);
    glOrtho(0, rect().width(), 0, colorBarHeight, 0, 1);
    glMatrixMode(GL_MODELVIEW);

    // Drawing goes here
    QColor startColor, finishColor;
    glPushMatrix();
    {
        glLoadIdentity();
        int barWidth = (rect().width() - 400 > 0) ? rect().width() - 400
                                                  : rect().width() - 200;
        int barMargin = 5;
        int barHeight = colorBarHeight - 2*barMargin;
        float segment_size = float(barWidth) / 101.0;
        colorbar_offset = (rect().width() - segment_size * 101.0) / 2;
        for (int i = 0; i < 100; i++)
        {
            glPushMatrix();

            glTranslatef(colorbar_offset + i*segment_size, barMargin, 0);
            glBegin(GL_QUADS);
            startColor = options->colormap->color(i / 100.0 * maxMetric);
            glColor3f(startColor.red() / 255.0,
                      startColor.green() / 255.0,
                      startColor.blue() / 255.0);
            glVertex3f(0, 0, 0);
            glVertex3f(0, barHeight, 0);
            finishColor = options->colormap->color((i + 1) / 100.0 * maxMetric);
            glColor3f(finishColor.red() / 255.0,
                      finishColor.green() / 255.0,
                      finishColor.blue() / 255.0);
            glVertex3f(segment_size, barHeight, 0);
            glVertex3f(segment_size, 0, 0);
            glEnd();
            glPopMatrix();
        }
    }
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glDisable(GL_SCISSOR_TEST);
}

// Labels for colorbar
void StepVis::drawColorBarText(QPainter * painter)
{
    // Based on metric
    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = painter->fontMetrics();
    maxMetricTextWidth = font_metrics.width(maxMetricText);
    painter->drawText(rect().width() - colorbar_offset + 3,
                      rect().height() - colorBarHeight/2
                      + font_metrics.xHeight()/2,
                      maxMetricText);
    int metricWidth = font_metrics.width(options->metric);
    painter->drawText(colorbar_offset - metricWidth - 6,
                      rect().height() - colorBarHeight/2
                      + font_metrics.xHeight()/2,
                      options->metric);
}


// This may change the colorbar but only if the colormap
// is non-categorical. If it's categorical, changing the
// range would not make sense.
void StepVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    if (event->y() >= rect().height() - colorBarHeight)
    {
        if (event->x() < rect().width() - colorbar_offset
            && event->x() > colorbar_offset
            && !options->colormap->isCategorical())
        {
            options->colormap->setClamp(maxMetric
                                        * (event->x() - colorbar_offset)
                                        / (rect().width()
                                           - 2 * colorbar_offset));
            repaint();
        }
        else if (event->x() > rect().width() - colorbar_offset + 3
                 && event->x() < rect().width() - colorbar_offset + 3
                                 + maxMetricTextWidth
                 && !options->colormap->isCategorical())
        {
            metricdialog->show();

        }
    }
    else
    {
        TimelineVis::mouseDoubleClickEvent(event);
    }
}

// Hover on colorbar to see color value
void StepVis::drawColorValue(QPainter * painter)
{
    return;
    if (!visProcessed || hoverText.length() < 1)
        return;

    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = painter->fontMetrics();

    // Determine bounding box of FontMetrics
    QRect textRect = font_metrics.boundingRect(hoverText);

    // Draw bounding box
    painter->setPen(QPen(QColor(255, 255, 0, 150), 1.0, Qt::SolidLine));
    painter->drawRect(QRectF(mousex, mousey - 20,
                             textRect.width(), textRect.height()));
    painter->fillRect(QRectF(mousex, mousey - 20,
                             textRect.width(), textRect.height()),
                      QBrush(QColor(255, 255, 144, 150)));

    // Draw text
    painter->setPen(Qt::black);
    painter->drawText(mousex + 2, mousey + textRect.height() - 2 - 20,
                      hoverText);
}
