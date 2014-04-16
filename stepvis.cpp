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
    lassoRect(QRect())
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
    maxMetric = 0;
    QString metric(options->metric);
    for (QList<Partition *>::Iterator part = trace->partitions->begin(); part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
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
    maxMetricText = systemlocale.toString(maxMetric) + " ns";

    delete metricdialog;
    metricdialog = new MetricRangeDialog(this, maxMetric, maxMetric);
    connect(metricdialog, SIGNAL(valueChanged(long long int)), this, SLOT(setMaxMetric(long long int)));
    metricdialog->hide();
}

void StepVis::setMaxMetric(long long int new_max)
{
    options->setRange(0, new_max);
    QLocale systemlocale = QLocale::system();
    maxMetricText = systemlocale.toString(new_max) + " ns";
    repaint();
}

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
    stepSpan = stop - start + 1;
    jumped = jump;

    if (!closed)
        repaint();
}

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
        startProcess = startProcess + processSpan * pressy / (rect().height() - colorBarHeight);
        processSpan = processSpan * (event->y() - pressy) / (rect().height() - colorBarHeight);
    }
    else
    {
        startProcess = startProcess + processSpan * event->y() / (rect().height() - colorBarHeight);
        processSpan = processSpan * (pressy - event->y()) / (rect().height() - colorBarHeight);
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
    if (mousePressed && !rightPressed)
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
        //std::cout << "Emitting " << startStep << ", " << (startStep + stepSpan) << std::endl;
        emit stepsChanged(startStep, startStep + stepSpan, false);
    }
    else if (mousePressed && rightPressed)
    {
        lassoRect = QRect(std::min(pressx, event->x()), std::min(pressy, event->y()),
                     abs(pressx - event->x()), abs(pressy - event->y()));
        repaint();
    }
    else // potential hover
    {
        mousex = event->x();
        mousey = event->y();
        hoverText = "";
        if (mousey >= rect().height() - colorBarHeight)
        {
            if (event->x() < rect().width() - colorbar_offset && event->x() > colorbar_offset)
            {
                long long int colormax = options->colormap->getMax();
                hoverText = QString::number(int(colormax * (event->x() - colorbar_offset) / (rect().width() - 2 * colorbar_offset)));
                repaint();
            }
        }
        else if (options->showAggregateSteps && hover_event && drawnEvents[hover_event].contains(mousex, mousey))
        {
            if (!hover_aggregate && mousex <= drawnEvents[hover_event].x() + stepwidth)
            {
                hover_aggregate = true;
                repaint();
            }
            else if (hover_aggregate && mousex >=  drawnEvents[hover_event].x() + stepwidth)
            {
                hover_aggregate = false;
                repaint();
            }
        }
        else if (hover_event == NULL || !drawnEvents[hover_event].contains(mousex, mousey))
        {
            hover_event = NULL;
            for (QMap<Event *, QRect>::Iterator evt = drawnEvents.begin(); evt != drawnEvents.end(); ++evt)
                if (evt.value().contains(mousex, mousey))
                {
                    hover_aggregate = false;
                    if (options->showAggregateSteps && mousex <= evt.value().x() + stepwidth)
                        hover_aggregate = true;
                    hover_event = evt.key();
                }

            repaint();
        }
    }

}

// zooms, but which zoom?
void StepVis::wheelEvent(QWheelEvent * event)
{
    if (!visProcessed)
        return;

    float scale = 1;
    int clicks = event->delta() / 8 / 15;
    scale = 1 + clicks * 0.05;
    if (Qt::MetaModifier && event->modifiers()) {
        // Vertical
        std::cout << " old span " << processSpan << " old start " << startProcess << std::endl;
        float avgProc = startProcess + processSpan / 2.0;
        processSpan *= scale;
        startProcess = avgProc - processSpan / 2.0;
        std::cout << "Avg proc " << avgProc << " new process span " << processSpan << " and new start " << startProcess << std::endl;
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
            for (int i = startPartition; i >= 0; --i) // Keep setting the one before until its right
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
            for (int i = startPartition; i < trace->partitions->length(); ++i) // As soon as we find one, we're good
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

    if (trace && options->metric.compare(cacheMetric) != 0)
        setupMetric();
}

void StepVis::qtPaint(QPainter *painter)
{
    if(!visProcessed)
        return;

    // In this case we haven't already drawn stuff with GL, so we paint it here.
    if ((rect().height() - colorBarHeight) / processSpan >= 3 && rect().width() / stepSpan >= 3)
      paintEvents(painter);

    drawProcessLabels(painter, rect().height() - colorBarHeight, processheight);
    drawColorBarText(painter);

    // Hover is independent of how we drew things
    drawHover(painter);
    drawColorValue(painter);

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
    double density = num_events / 1.0 / (stepSpan * processSpan); // 1 if an event at every step, small if less.
    float opacity = 1.0;
    float xoffset = 0;
    float yoffset = 0;
    if (density)
    {
        if (processheight < 1.0)
        {
            yoffset = 1.0 / processheight / 2 / (10 * density);
            //opacity = 0.4;
        }
        if (stepwidth < 1.0)
        {
            xoffset = 1.0 / stepwidth / 2 / (10 * density);
            //opacity = 0.4;
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
        for (QMap<int, QList<Event *> *>::Iterator event_list = part->events->begin(); event_list != part->events->end(); ++event_list)
        {
            bool selected = false;
            if (part->gnome == selected_gnome && selected_processes.contains(proc_to_order[event_list.key()]))
                selected = true;
            position = proc_to_order[event_list.key()];
            if (position < floor(startProcess) || position > ceil(startProcess + processSpan)) // Out of span
                 continue;
            y = (maxProcess - position) * barheight - 1;
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
            {

                if ((*evt)->step < bottomStep || (*evt)->step > topStep) // Out of span
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

                //std::cout << "Opacity is..." << myopacity << std::endl;

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

void StepVis::paintEvents(QPainter * painter)
{
    //painter->fillRect(rect(), backgroundColor);

    int effectiveHeight = rect().height() - colorBarHeight;
    int effectiveWidth = rect().width();

    int process_spacing = 0;
    if (effectiveHeight / processSpan > spacingMinimum)
        process_spacing = 3;

    int step_spacing = 0;
    if (effectiveWidth / stepSpan > spacingMinimum)
        step_spacing = 3;

    float x, y, w, h, xa, wa, blockwidth;
    float blockheight = floor(effectiveHeight / processSpan);
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
    QSet<Message *> drawMessages = QSet<Message *>();
    painter->setPen(QPen(QColor(0, 0, 0)));
    Partition * part = NULL;
    int topStep = boundStep(startStep + stepSpan) + 1;
    int bottomStep = floor(startStep) - 1;
    //std::cout << " Step span is " << bottomStep << " to " << topStep << " and startPartition is ";
    //std::cout << trace->partitions->at(startPartition)->min_global_step << " to " << trace->partitions->at(startPartition)->max_global_step << std::endl;
    float myopacity, opacity = 1.0;
    if (selected_gnome && !selected_processes.isEmpty())
        opacity = 0.50;
    for (int i = startPartition; i < trace->partitions->length(); ++i)
    {
        part = trace->partitions->at(i);
        if (part->min_global_step > topStep)
            break;
        else if (part->max_global_step < bottomStep)
            continue;


        for (QMap<int, QList<Event *> *>::Iterator event_list = part->events->begin(); event_list != part->events->end(); ++event_list)
        {
            bool selected = false;
            if (part->gnome == selected_gnome && selected_processes.contains(proc_to_order[event_list.key()]))
                selected = true;
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
            {
                position = proc_to_order[(*evt)->process]; // Move this into the out loop!
                if ((*evt)->step < bottomStep || (*evt)->step > topStep) // Out of span
                {
                    //std::cout << (*evt)->step << ", " << startStep << ", " << boundStep(startStep + stepSpan) << std::endl;
                    continue;
                }
                if (position < floor(startProcess) || position > ceil(startProcess + processSpan)) // Out of span
                    continue;
                // 0 = startProcess, effectiveHeight = stopProcess (startProcess + processSpan)
                // 0 = startStep, rect().width() = stopStep (startStep + stepSpan)
                y = floor((position - startProcess) * blockheight) + 1;
                if (options->showAggregateSteps)
                    x = floor(((*evt)->step - startStep) * blockwidth) + 1 + labelWidth;
                else
                    x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1 + labelWidth;
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
                    painter->fillRect(QRectF(x, y, w, h), QBrush(options->colormap->color((*evt)->getMetric(metric), myopacity)));
                else
                    painter->fillRect(QRectF(x, y, w, h), QBrush(QColor(180, 180, 180)));
                // Change pen color if selected
                if (*evt == selected_event && !selected_aggregate)
                    painter->setPen(QPen(Qt::yellow));
                // Draw border but only if we're doing spacing, otherwise too messy
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



                for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
                    drawMessages.insert((*msg));

                if (options->showAggregateSteps) {
                    xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1 + labelWidth;
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
                        painter->fillRect(QRectF(xa, y, wa, h), QBrush(options->colormap->color((*evt)->getMetric(metric, true), myopacity)));
                    else
                        painter->fillRect(QRectF(xa, y, wa, h), QBrush(QColor(180, 180, 180)));

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
        if (processSpan <= 32)
            painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
        else
            painter->setPen(QPen(Qt::black, 1, Qt::SolidLine));
        Event * send_event;
        Event * recv_event;
        QPointF p1, p2;
        w = barwidth;
        h = barheight;
        for (QSet<Message *>::Iterator msg = drawMessages.begin(); msg != drawMessages.end(); ++msg) {
            send_event = (*msg)->sender;
            recv_event = (*msg)->receiver;
            position = proc_to_order[send_event->process];
            y = floor((position - startProcess) * blockheight) + 1;
            if (options->showAggregateSteps)
                x = floor((send_event->step - startStep) * blockwidth) + 1 + labelWidth;
            else
                x = floor((send_event->step - startStep) / 2 * blockwidth) + 1 + labelWidth;
            if (options->showMessages == VisOptions::TRUE)
            {
                p1 = QPointF(x + w/2.0, y + h/2.0);
                position = proc_to_order[recv_event->process];
                y = floor((position - startProcess) * blockheight) + 1;
                if (options->showAggregateSteps)
                    x = floor((recv_event->step - startStep) * blockwidth) + 1 + labelWidth;
                else
                    x = floor((recv_event->step - startStep) / 2 * blockwidth) + 1 + labelWidth;
                p2 = QPointF(x + w/2.0, y + h/2.0);
            }
            else
            {
                p1 = QPointF(x, y + h/2.0);
                position = proc_to_order[recv_event->process];
                y = floor((position - startProcess) * blockheight) + 1;
                p2 = QPointF(x + w, y + h/2.0);
            }
            drawLine(painter, &p1, &p2, effectiveHeight);
        }
    }
}

void StepVis::drawLine(QPainter * painter, QPointF * p1, QPointF * p2, int effectiveHeight)
{
    if (p1->y() > effectiveHeight && p2->y() > effectiveHeight)
    {
        return;
    }
    else if (p1->y() > effectiveHeight)
    {
        float slope = float(p1->y() - p2->y()) / (p1->x() - p2->x());
        float intercept = p1->y() - slope * p1->x();
        painter->drawLine(QPointF((effectiveHeight - intercept) / slope, effectiveHeight), *p2);
    }
    else if (p2->y() > effectiveHeight)
    {
        float slope = float(p1->y() - p2->y()) / (p1->x() - p2->x());
        float intercept = p1->y() - slope * p1->x();
        painter->drawLine(*p1, QPointF((effectiveHeight - intercept) / slope, effectiveHeight));
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
        int barWidth = (rect().width() - 400 > 0) ? rect().width() - 400 : rect().width() - 200;
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
            glColor3f(startColor.red() / 255.0, startColor.green() / 255.0, startColor.blue() / 255.0);
            glVertex3f(0, 0, 0);
            glVertex3f(0, barHeight, 0);
            finishColor = options->colormap->color((i + 1) / 100.0 * maxMetric);
            glColor3f(finishColor.red() / 255.0, finishColor.green() / 255.0, finishColor.blue() / 255.0);
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

void StepVis::drawColorBarText(QPainter * painter)
{
    // Based on metric
    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = painter->fontMetrics();
    maxMetricTextWidth = font_metrics.width(maxMetricText);
    painter->drawText(rect().width() - colorbar_offset + 3,
                      rect().height() - colorBarHeight/2 + font_metrics.xHeight()/2,
                      maxMetricText);
    int metricWidth = font_metrics.width(options->metric);
    painter->drawText(colorbar_offset - metricWidth - 6,
                      rect().height() - colorBarHeight/2 + font_metrics.xHeight()/2,
                      options->metric);
}

void StepVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    if (event->y() >= rect().height() - colorBarHeight)
    {
        if (event->x() < rect().width() - colorbar_offset && event->x() > colorbar_offset)
        {
            options->colormap->setClamp(maxMetric * (event->x() - colorbar_offset) / (rect().width() - 2 * colorbar_offset));
            repaint();
        }
        else if (event->x() > rect().width() - colorbar_offset + 3 && event->x() < rect().width() - colorbar_offset + 3 + maxMetricTextWidth)
        {
            metricdialog->show();

        }
    }
    else
    {
        TimelineVis::mouseDoubleClickEvent(event);
    }
}

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
    painter->drawRect(QRectF(mousex, mousey - 20, textRect.width(), textRect.height()));
    painter->fillRect(QRectF(mousex, mousey - 20, textRect.width(), textRect.height()), QBrush(QColor(255, 255, 144, 150)));

    // Draw text
    painter->setPen(Qt::black);
    painter->drawText(mousex + 2, mousey + textRect.height() - 2 - 20, hoverText);
}
