#include "stepvis.h"
#include <iostream>

StepVis::StepVis(QWidget* parent) : VisWidget(parent = parent)
{
    // Set painting variables
    backgroundColor = QColor(255, 255, 255);
    mousePressed = false;
    showAggSteps = true;
    trace = NULL;

    // Create color map
    colormap = new ColorMap(QColor(173, 216, 230), 0);
    colormap->addColor(QColor(240, 230, 140), 0.5);
    colormap->addColor(QColor(178, 34, 34), 1);
}

StepVis::~StepVis()
{
    delete colormap;
}

void StepVis::setTrace(Trace * t)
{
    VisWidget::setTrace(t);
    maxStep = 0;
    maxLateness = 0;
    for (QVector<Event *>::Iterator itr = trace->events->begin(); itr != trace->events->end(); itr++) {
        if ((*itr)->step > maxStep)
            maxStep = (*itr)->step;
        if ((*itr)->lateness > maxLateness)
            maxLateness = (*itr)->lateness;
    }
    colormap->setRange(0, maxLateness);

    // Initial conditions
    startStep = 0;
    stepSpan = initStepSpan;
    startProcess = 1;
    processSpan = trace->num_processes;
}

void StepVis::setSteps(float start, float stop)
{
    if (changeSource) {
        changeSource = false;
        return;
    }
    startStep = start;
    stepSpan = stop - start + 1;
    repaint();
}

void StepVis::processVis()
{
    proc_to_order = QMap<int, int>();
    order_to_proc = QMap<int, int>();
    for (int i = 1; i <= trace->num_processes; i++) { //TODO: Change to zero at some point
        proc_to_order[i] = i;
        order_to_proc[i] = i;
    }
    visProcessed = true;
}


void StepVis::mousePressEvent(QMouseEvent * event)
{
    mousePressed = true;
    mousex = event->x();
    mousey = event->y();
}

void StepVis::mouseReleaseEvent(QMouseEvent * event)
{
    Q_UNUSED(event);
    mousePressed = false;
}

void StepVis::mouseMoveEvent(QMouseEvent * event)
{
    if (mousePressed) {
        int diffx = mousex - event->x();
        int diffy = mousey - event->y();
        startStep += diffx / 1.0 / stepwidth;
        startProcess += diffy / 1.0 / processheight;

        if (startStep < 0)
            startStep = 0;
        if (startStep > maxStep)
            startStep = maxStep;

        if (startProcess < 1)
            startProcess = 1;
        if (startProcess + processSpan > trace->num_processes + 1)
            startProcess = trace->num_processes - processSpan + 1;

        mousex = event->x();
        mousey = event->y();

        changeSource = true;
        //std::cout << "Emitting " << startStep << ", " << (startStep + stepSpan) << std::endl;
        emit stepsChanged(startStep, startStep + stepSpan);
    }
    repaint();
}

// zooms, but which zoom?
void StepVis::wheelEvent(QWheelEvent * event)
{
    float scale = 1;
    int clicks = event->delta() / 8 / 15;
    std::cout << "wheel clicks " << clicks << std::endl;
    scale = 1 + clicks * 0.05;
    if (Qt::MetaModifier && event->modifiers()) {
        // Vertical
        float avgProc = startProcess + processSpan / 2.0;
        processSpan *= scale;
        startProcess = avgProc - processSpan / 2.0;
        std::cout << "Verti Scale " << startProcess << ", " << processSpan << std::endl;
    } else {
        // Horizontal
        float avgStep = startStep + stepSpan / 2.0;
        stepSpan *= scale;
        startStep = avgStep - stepSpan / 2.0;
        std::cout << "Horiz Scale " << startStep << ", " << stepSpan << std::endl;
    }
    repaint();
    changeSource = true;
    emit stepsChanged(startStep, startStep + stepSpan);
}

void StepVis::qtPaint(QPainter *painter)
{
    painter->fillRect(rect(), backgroundColor);

    if(!visProcessed)
        return;

    // We don't know how to draw this small yet -- will go to gl, probably
    if (rect().height() / processSpan < 3 || rect().width() / stepSpan < 3)
        return;

    int process_spacing = 0;
    if (rect().height() / processSpan > 12)
        process_spacing = 3;

    int step_spacing = 0;
    if (rect().width() / stepSpan > 12)
        step_spacing = 3;


    float x, y, w, h, xa, wa, blockwidth;
    float blockheight = floor(rect().height() / processSpan);
    if (showAggSteps)
        blockwidth = floor(rect().width() / stepSpan);
    else
        blockwidth = floor(rect().width() / (ceil(stepSpan / 2.0)));
    float barheight = blockheight - process_spacing;
    float barwidth = blockwidth - step_spacing;
    processheight = blockheight;
    stepwidth = blockwidth;

    int position;
    bool complete, aggcomplete;
    QSet<Message *> drawMessages = QSet<Message *>();
    painter->setPen(QPen(QColor(0, 0, 0)));

        for (QVector<Event *>::Iterator itr = trace->events->begin(); itr != trace->events->end(); ++itr)
        {
            position = proc_to_order[(*itr)->process];
            if ((*itr)->step < floor(startStep) || (*itr)->step > boundStep(startStep + stepSpan)) // Out of span
                continue;
            if (position < floor(startProcess) || position > ceil(startProcess + processSpan)) // Out of span
                continue;
            // 0 = startProcess, rect().height() = stopProcess (startProcess + processSpan)
            // 0 = startStep, rect().width() = stopStep (startStep + stepSpan)
            y = floor((position - startProcess) * blockheight) + 1;
            if (showAggSteps)
                x = floor(((*itr)->step - startStep) * blockwidth) + 1;
            else
                x = floor(((*itr)->step - startStep) / 2 * blockwidth) + 1;
            w = barwidth;
            h = barheight;

            // Corrections for partially drawn
            complete = true;
            if (y < 0) {
                h = barheight - fabs(y);
                y = 0;
                complete = false;
            } else if (y + barheight > rect().height()) {
                h = rect().height() - y;
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
            painter->fillRect(QRectF(x, y, w, h), QBrush(colormap->color((*itr)->lateness)));
            if (complete)
                painter->drawRect(QRectF(x,y,w,h));
            else
                incompleteBox(painter, x, y, w, h);

            for (QVector<Message *>::Iterator mitr = (*itr)->messages->begin(); mitr != (*itr)->messages->end(); ++mitr)
                drawMessages.insert((*mitr));

            if (showAggSteps) {
                xa = floor(((*itr)->step - startStep - 1) * blockwidth) + 1;
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
                painter->fillRect(QRectF(xa, y, wa, h), QBrush(colormap->color((*itr)->lateness)));
                if (aggcomplete)
                    painter->drawRect(QRectF(xa, y, wa, h));
                else
                    incompleteBox(painter, xa, y, wa, h);
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
        for (QSet<Message *>::Iterator itr = drawMessages.begin(); itr != drawMessages.end(); ++itr) {
            send_event = (*itr)->sender;
            recv_event = (*itr)->receiver;
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
