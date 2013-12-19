#include "stepvis.h"
#include <iostream>

StepVis::StepVis(QWidget* parent) : TimelineVis(parent = parent),
    showAggSteps(true),
    maxLateness(0),
    maxLatenessText(""),
    maxLatenessTextWidth(0),
    colorbar_offset(0)
{

}

StepVis::~StepVis()
{

}

void StepVis::setTrace(Trace * t)
{
    VisWidget::setTrace(t);
    maxStep = trace->global_max_step;
    maxLateness = 0;
    QString lateness("Lateness");
    for (QList<Partition *>::Iterator part = trace->partitions->begin(); part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list) {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt) {
                if ((*evt)->hasMetric(lateness))
                {
                    if ((*evt)->getMetric(lateness) > maxLateness)
                        maxLateness = (*evt)->getMetric(lateness);
                    if ((*evt)->getMetric(lateness, true) > maxLateness)
                        maxLateness = (*evt)->getMetric(lateness, true);
                }
            }
        }
    }
    colormap->setRange(0, maxLateness);

    // Initial conditions
    startStep = 0;
    stepSpan = initStepSpan;
    startProcess = 0;
    processSpan = trace->num_processes;

    // For colorbar
    QLocale systemlocale = QLocale::system();
    maxLatenessText = systemlocale.toString(maxLateness);
}

void StepVis::setSteps(float start, float stop)
{
    if (!visProcessed)
        return;

    if (changeSource) {
        changeSource = false;
        return;
    }
    startStep = start;
    stepSpan = stop - start + 1;

    if (!closed)
        repaint();
}

void StepVis::mouseMoveEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    if (mousePressed)
    {
        int diffx = mousex - event->x();
        int diffy = mousey - event->y();
        startStep += diffx / 1.0 / stepwidth;
        startProcess += diffy / 1.0 / processheight;

        if (startStep < 0)
            startStep = 0;
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
        emit stepsChanged(startStep, startStep + stepSpan);
    }
    else // potential hover
    {
        mousex = event->x();
        mousey = event->y();
        if (hover_event == NULL || !drawnEvents[hover_event].contains(mousex, mousey))
        {
            hover_event = NULL;
            for (QMap<Event *, QRect>::Iterator evt = drawnEvents.begin(); evt != drawnEvents.end(); ++evt)
                if (evt.value().contains(mousex, mousey))
                    hover_event = evt.key();

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
        float avgProc = startProcess + processSpan / 2.0;
        processSpan *= scale;
        startProcess = avgProc - processSpan / 2.0;
    } else {
        // Horizontal
        float avgStep = startStep + stepSpan / 2.0;
        stepSpan *= scale;
        startStep = avgStep - stepSpan / 2.0;
    }
    repaint();
    changeSource = true;
    emit stepsChanged(startStep, startStep + stepSpan);
}

void StepVis::prepaint()
{
    drawnEvents.clear();
}

void StepVis::qtPaint(QPainter *painter)
{
    if(!visProcessed)
        return;

    // In this case we haven't already drawn stuff with GL, so we paint it here.
    if (rect().height() / processSpan >= 3 && rect().width() / stepSpan >= 3)
      paintEvents(painter);

    drawColorBarText(painter);

    // Hover is independent of how we drew things
    drawHover(painter);
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

    QString metric("Lateness");

    // Setup viewport
    int width = rect().width();
    int height = effectiveHeight;

    glViewport(0,
               colorBarHeight,
               width,
               height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, 0, 1);
    //glTranslatef(0, -colorBarHeight, 0);

    // Figure out if either height/process or width/step is bigger than 1
    float barwidth = 1;
    int effectiveStepSpan = stepSpan;
    if (!showAggSteps)
        effectiveStepSpan = ceil(stepSpan / 2.0);
    if (width / float(effectiveStepSpan) > 1)
    {
        barwidth = width / float(effectiveStepSpan);
    }

    float barheight = 1;
    if (height / float(processSpan) > 1)
    {
        barheight = height / float(processSpan);
    }

    processheight = barheight;
    stepwidth = barwidth;

    // Generate buffers to hold/accumulate information for anti-aliasing
    // We do this by pixel, each needs x,y
    //QVector<GLfloat> bars = QVector<GLfloat>(2 * width * height);
    // Each "bar" has 4 vertices and each vertex has a color which is 3 floats
    // So our color vector must be 3 * height * width
    // Initialized with 1.0s (white)
    QVector<GLfloat> colors = QVector<GLfloat>(3 * width * height, 1.0);
    // Per bar metrics saved and # affecting that slot, both initialized to zero
    QVector<GLfloat> metrics = QVector<GLfloat>(width * height, 0.0);
    QVector<GLfloat> contributors = QVector<GLfloat>(width * height, 0.0);

    // Set the points
    int base_index, index;
    /*for (int y = 0; y < height; y++)
    {
        base_index = 2*y*width;
        for (int x = 0; x < width; x++)
        {
            index = base_index + 2*x;
            bars[index] = x;
            bars[index + 1] = y;
        }
    }*/

    // Process events for values
    float x, y; // true position
    float fx, fy; // fractions
    int sx, tx, sy, ty; // max extents
    float position; // placement of process
    for (QList<Partition *>::Iterator part = trace->partitions->begin(); part != trace->partitions->end(); ++part)
    {
        if ((*part)->min_global_step > boundStep(startStep + stepSpan)|| (*part)->max_global_step < floor(startStep))
            continue;
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list) {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
            {
                position = proc_to_order[(*evt)->process];
                if ((*evt)->step < floor(startStep) || (*evt)->step > boundStep(startStep + stepSpan)) // Out of span
                    continue;
                if (position < floor(startProcess) || position > ceil(startProcess + processSpan)) // Out of span
                    continue;

                // Calculate position of this bar in float space
                y = (position - startProcess) * barheight + 1;
                if (showAggSteps)
                    x = ((*evt)->step - startStep) * barwidth + 1;
                else
                    x = ((*evt)->step - startStep) / 2 * barwidth + 1;

                // Find actually drawn extents and/or skip
                bool draw = true;
                sx = floor(x);
                if (sx < 0)
                    sx = 0;
                if (sx > width - 1)
                    if (showAggSteps)
                        draw = false;
                    else
                        continue;
                tx = ceil(x + barwidth);
                if (tx < 0)
                    continue;
                if (tx > width - 1)
                    tx = width - 1;
                sy = floor(y);
                if (sy < 0)
                    sy = 0;
                if (sy > height - 1)
                    continue;
                ty = ceil(y + barheight);
                if (ty < 0)
                    continue;
                if (ty > height - 1)
                    ty = height - 1;

                // Make contributions to each pixel space
                if (draw)
                    for (int j = sy; j <= ty; j++)
                    {
                        fy = 1.0;
                        if (j < y && j + 1 > y + barheight)
                            fy = barheight;
                        else if (j < y)
                            fy = j + 1 - y;
                        else if (y + barheight < j + 1)
                            fy = j + 1 - y - barheight;
                        for (int i = sx; i <= tx; i++)
                        {
                            fx = 1.0;
                            if (i < x && i + 1 > x + barwidth )
                                fx = barwidth;
                            else if (i < x)
                                fx = i + 1 - x;
                            else if (x + barwidth < i + 1)
                                fx = i + 1 - x - barwidth;

                            metrics[width*j + i] += (*(*evt)->metrics)[metric]->event * fx * fy;

                            contributors[width*j + i] += fx * fy;
                        }
                    }

                if (showAggSteps) // repeat!
                {
                    x = ((*evt)->step - startStep - 1) * barwidth + 1;
                    if (x + barwidth <= 0)
                        continue;

                    // Not as many checks since we know this comes before the preivous event
                    sx = floor(x);
                    if (sx < 0)
                        sx = 0;

                    for (int j = sy; j <= ty; j++)
                    {
                        fy = 1.0;
                        if (j < y && j + 1 > y + barheight)
                            fy = barheight;
                        else if (j < y)
                            fy = j + 1 - y;
                        else if (y + barheight < j + 1)
                            fy = j + 1 - y - barheight;
                        for (int i = sx; i <= tx; i++)
                        {
                            fx = 1.0;
                            if (i < x && i + 1 > x + barwidth )
                                fx = barwidth;
                            else if (i < x)
                                fx = i + 1 - x;
                            else if (x + barwidth < i + 1)
                                fx = i + 1 - x - barwidth;

                            metrics[width*j + i] += (*(*evt)->metrics)[metric]->aggregate * fx * fy;

                            contributors[width*j + i] += fx * fy;
                        }
                    }
                }

            }
        }
    }

    // Convert colors
    QColor color;
    int metric_index;
    for (int y = 0; y < height; y++)
    {
        base_index = 3*y*width;
        metric_index = y*width;
        for (int x = 0; x < width; x++)
        {
            index = base_index + 3*x;
            if (contributors[metric_index + x] > 0)
            {
                color = colormap->color(metrics[metric_index + x] / contributors[metric_index + x]);
                colors[index] = color.red() / 255.0;
                colors[index + 1] = color.green() / 255.0;
                colors[index + 2] = color.blue() / 255.0;
            }
            else
            {
                colors[index] = backgroundColor.red() / 255.0;
                colors[index + 1] = backgroundColor.green() / 255.0;
                colors[index + 2] = backgroundColor.blue() / 255.0;
            }
        }
    }

    // Draw
    glPointSize(1.0f);

    // Points
    /*glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColorPointer(3,GL_FLOAT,0,colors.constData());
    glVertexPointer(2,GL_FLOAT,0,bars.constData());
    glDrawArrays(GL_POINTS,0,bars.size()/2);*/
    //glRasterPos2i(0, colorBarHeight);
    glDrawPixels(width, height, GL_RGB, GL_FLOAT, colors.constData());
    glRasterPos2i(0,0);
}

void StepVis::paintEvents(QPainter * painter)
{
    //painter->fillRect(rect(), backgroundColor);

    int effectiveHeight = rect().height() - colorBarHeight;

    int process_spacing = 0;
    if (effectiveHeight / processSpan > spacingMinimum)
        process_spacing = 3;

    int step_spacing = 0;
    if (rect().width() / stepSpan > spacingMinimum)
        step_spacing = 3;


    float x, y, w, h, xa, wa, blockwidth;
    float blockheight = floor(effectiveHeight / processSpan);
    if (showAggSteps)
        blockwidth = floor(rect().width() / stepSpan);
    else
        blockwidth = floor(rect().width() / (ceil(stepSpan / 2.0)));
    float barheight = blockheight - process_spacing;
    float barwidth = blockwidth - step_spacing;
    processheight = blockheight;
    stepwidth = blockwidth;

    QString metric("Lateness");
    int position;
    bool complete, aggcomplete;
    QSet<Message *> drawMessages = QSet<Message *>();
    painter->setPen(QPen(QColor(0, 0, 0)));
    // TODO: Replace this part where we look through all partitions with a part where we look through
    // our own active partition list that we determin whenever a setSteps happens.
    for (QList<Partition *>::Iterator part = trace->partitions->begin(); part != trace->partitions->end(); ++part)
    {
        if ((*part)->min_global_step > boundStep(startStep + stepSpan) || (*part)->max_global_step < floor(startStep))
        {
            //std::cout << (*part)->min_global_step << ", " << (*part)->max_global_step << " : " << startStep << ", " << stepSpan << std::endl;
            continue;
        }
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list) {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
            {
                position = proc_to_order[(*evt)->process];
                if ((*evt)->step < floor(startStep) || (*evt)->step > boundStep(startStep + stepSpan)) // Out of span
                {
                    //std::cout << (*evt)->step << ", " << startStep << ", " << boundStep(startStep + stepSpan) << std::endl;
                    continue;
                }
                if (position < floor(startProcess) || position > ceil(startProcess + processSpan)) // Out of span
                    continue;
                // 0 = startProcess, effectiveHeight = stopProcess (startProcess + processSpan)
                // 0 = startStep, rect().width() = stopStep (startStep + stepSpan)
                y = floor((position - startProcess) * blockheight) + 1;
                if (showAggSteps)
                    x = floor(((*evt)->step - startStep) * blockwidth) + 1;
                else
                    x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1;
                w = barwidth;
                h = barheight;

                // Corrections for partially drawn
                complete = true;
                if (y < 0) {
                    h = barheight - fabs(y);
                    y = 0;
                    complete = false;
                } else if (y + barheight > effectiveHeight) {
                    h = effectiveHeight - y;
                    complete = false;
                }
                if (x < 0) {
                    w = barwidth - fabs(x);
                    x = 0;
                    complete = false;
                } else if (x + barwidth > rect().width()) {
                    w = rect().width() - x;
                    complete = false;
                }

                // Draw the event
                if ((*evt)->hasMetric(metric))
                    painter->fillRect(QRectF(x, y, w, h), QBrush(colormap->color((*evt)->getMetric(metric))));
                else
                    painter->fillRect(QRectF(x, y, w, h), QBrush(QColor(180, 180, 180)));
                // Change pen color if selected
                if (*evt == selected_event)
                    painter->setPen(QPen(Qt::yellow));
                // Draw border but only if we're doing spacing, otherwise too messy
                if (step_spacing > 0 && process_spacing > 0)
                    if (complete)
                        painter->drawRect(QRectF(x,y,w,h));
                    else
                        incompleteBox(painter, x, y, w, h);
                // Revert pen color
                if (*evt == selected_event)
                    painter->setPen(QPen(QColor(0, 0, 0)));

                // For selection
                drawnEvents[*evt] = QRect(x, y, w, h);

                for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
                    drawMessages.insert((*msg));

                if (showAggSteps) {
                    xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1;
                    wa = barwidth;
                    if (xa + wa <= 0)
                        continue;

                    aggcomplete = true;
                    if (xa < 0) {
                        wa = barwidth - fabs(xa);
                        xa = 0;
                        aggcomplete = false;
                    } else if (xa + barwidth > rect().width()) {
                        wa = rect().width() - xa;
                        aggcomplete = false;
                    }

                    aggcomplete = aggcomplete && complete;
                    if ((*evt)->hasMetric(metric))
                        painter->fillRect(QRectF(xa, y, wa, h), QBrush(colormap->color((*evt)->getMetric(metric, true))));
                    else
                        painter->fillRect(QRectF(xa, y, wa, h), QBrush(QColor(180, 180, 180)));
                    if (aggcomplete && step_spacing > 0 && process_spacing > 0)
                        painter->drawRect(QRectF(xa, y, wa, h));
                    else
                        incompleteBox(painter, xa, y, wa, h);
                }

            }
        }
    }

        // Messages
        // We need to do all of the message drawing after the event drawing
        // for overlap purposes
        painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
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
            if (showAggSteps)
                x = floor((send_event->step - startStep) * blockwidth) + 1;
            else
                x = floor((send_event->step - startStep) / 2 * blockwidth) + 1;
            p1 = QPointF(x + w/2.0, y + h/2.0);
            position = proc_to_order[recv_event->process];
            y = floor((position - startProcess) * blockheight) + 1;
            if (showAggSteps)
                x = floor((recv_event->step - startStep) * blockwidth) + 1;
            else
                x = floor((recv_event->step - startStep) / 2 * blockwidth) + 1;
            p2 = QPointF(x + w/2.0, y + h/2.0);
            painter->drawLine(p1, p2);
        }
}

void StepVis::drawColorBarGL()
{
    // Setup stuff for overlay like the minimaps
    glEnable(GL_SCISSOR_TEST);
    glMatrixMode(GL_PROJECTION);
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
            startColor = colormap->color(i / 100.0 * maxLateness);
            glColor3f(startColor.red() / 255.0, startColor.green() / 255.0, startColor.blue() / 255.0);
            glVertex3f(0, 0, 0);
            glVertex3f(0, barHeight, 0);
            finishColor = colormap->color((i + 1) / 100.0 * maxLateness);
            glColor3f(finishColor.red() / 255.0, finishColor.green() / 255.0, finishColor.blue() / 255.0);
            glVertex3f(segment_size, barHeight, 0);
            glVertex3f(segment_size, 0, 0);
            glEnd();
            glPopMatrix();
        }
    }
    glPopMatrix();

    glDisable(GL_SCISSOR_TEST);
}

void StepVis::drawColorBarText(QPainter * painter)
{
    // Based on Lateness
    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = this->fontMetrics();
    maxLatenessTextWidth = font_metrics.width(maxLatenessText);
    painter->drawText(rect().width() - colorbar_offset + 3,
                      rect().height() - colorBarHeight/2 + font_metrics.xHeight()/2,
                      maxLatenessText);
}
