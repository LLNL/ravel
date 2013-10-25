#include "timevis.h"
#include <iostream>

TimeVis::TimeVis(QWidget * parent) : VisWidget(parent = parent)
{
    // Set painting variables
    backgroundColor = QColor(255, 255, 255);
    mousePressed = false;
    trace = NULL;
}

TimeVis::~TimeVis()
{
    for (QVector<TimePair *>::Iterator itr = stepToTime->begin(); itr != stepToTime->end(); itr++) {
        delete *itr;
        *itr = NULL;
    }
    delete stepToTime;
}


void TimeVis::setTrace(Trace * t)
{
    VisWidget::setTrace(t);

    // Initial conditions
    startStep = 0;
    stopStep = startStep + initStepSpan;
    startProcess = 1;
    processSpan = trace->num_processes;

    // Determine time information
    minTime = ULLONG_MAX;
    maxTime = 0;
    startTime = ULLONG_MAX;
    unsigned long long stopTime = 0;
    maxStep = 0;
    for (QVector<Event *>::Iterator itr = trace->events->begin(); itr != trace->events->end(); itr++) {
        if ((*itr)->exit > maxTime)
            maxTime = (*itr)->exit;
        if ((*itr)->enter < minTime)
            minTime = (*itr)->enter;
        if ((*itr)->step >= boundStep(startStep) && (*itr)->enter < startTime)
            startTime = (*itr)->enter;
        if ((*itr)->step <= boundStep(stopStep) && (*itr)->exit > stopTime)
            stopTime = (*itr)->exit;
        if ((*itr)->step > maxStep)
            maxStep = (*itr)->step;
    }
    timeSpan = stopTime - startTime;

    if (stepToTime) {
        for (QVector<TimePair *>::Iterator itr = stepToTime->begin(); itr != stepToTime->end(); itr++) {
            delete *itr;
            *itr = NULL;
        }
        delete stepToTime;
    }

    stepToTime = new QVector<TimePair *>();
    for (int i = 0; i < maxStep/2; i++)
        stepToTime->insert(i, new TimePair(ULLONG_MAX, 0));
    int step;
    for (QVector<Event *>::Iterator itr = trace->events->begin(); itr != trace->events->end(); itr++) {
        if ((*itr)->step < 0)
            continue;
        step = (*itr)->step / 2;
        if ((*stepToTime)[step]->start > (*itr)->enter)
            (*stepToTime)[step]->start = (*itr)->enter;
        if ((*stepToTime)[step]->stop < (*itr)->exit)
            (*stepToTime)[step]->stop = (*itr)->exit;
    }

}

void TimeVis::processVis()
{
    proc_to_order = QMap<int, int>();
    order_to_proc = QMap<int, int>();
    for (int i = 1; i <= trace->num_processes; i++) { //TODO: Change to zero at some point
        proc_to_order[i] = i;
        order_to_proc[i] = i;
    }
    visProcessed = true;
}

void TimeVis::mousePressEvent(QMouseEvent * event)
{
    mousePressed = true;
    mousex = event->x();
    mousey = event->y();
}

void TimeVis::mouseReleaseEvent(QMouseEvent * event)
{
    Q_UNUSED(event);
    mousePressed = false;
}

void TimeVis::mouseMoveEvent(QMouseEvent * event)
{
    if (mousePressed) {
        int diffx = mousex - event->x();
        int diffy = mousey - event->y();
        startTime += diffx / 1.0 / timeSpan;
        startProcess += diffy / 1.0 / processheight;

        if (startTime < minTime)
            startStep = minTime;
        if (startStep > maxTime)
            startStep = maxTime - timeSpan;

        if (startProcess < 1)
            startProcess = 1;
        if (startProcess + processSpan > trace->num_processes + 1)
            startProcess = trace->num_processes - processSpan + 1;

        mousex = event->x();
        mousey = event->y();
    }
    repaint();
    if (mousePressed) {
        changeSource = true;
        emit stepsChanged(startStep, stopStep); // Calculated during painting
    }
}

void TimeVis::wheelEvent(QWheelEvent * event)
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
        float middleTime = startTime + timeSpan / 2.0;
        timeSpan *= scale;
        startStep = middleTime - timeSpan / 2.0;
        std::cout << "Horiz Scale " << startTime << ", " << timeSpan << std::endl;
    }
    repaint();
    changeSource = true;
    emit stepsChanged(startStep, stopStep); // Calculated during painting
}

void TimeVis::setSteps(float start, float stop)
{
    if (changeSource) {
        changeSource = false;
        return;
    }
    startTime = (*stepToTime)[boundStep(start)/2]->start;
    timeSpan = (*stepToTime)[boundStep(stop)/2]->stop - startTime;
    repaint();
}

void TimeVis::qtPaint(QPainter *painter)
{
    painter->fillRect(rect(), backgroundColor);

    if(!visProcessed)
        return;

    // We don't know how to draw this small yet -- will go to gl, probably
    if (rect().height() / processSpan < 3)
        return;

    int process_spacing = 0;
    if (rect().height() / processSpan > 12)
        process_spacing = 3;

    float x, y, w, h;
    float blockheight = floor(rect().height() / processSpan);
    float barheight = blockheight - process_spacing;
    processheight = blockheight;
    startStep = maxStep;
    stopStep = 0;

    int position, step;
    bool complete;
    QSet<Message *> drawMessages = QSet<Message *>();
    unsigned long long stopTime = startTime + timeSpan;
    painter->setPen(QPen(QColor(0, 0, 0)));

        for (QVector<Event *>::Iterator itr = trace->events->begin(); itr != trace->events->end(); ++itr)
        {
            position = proc_to_order[(*itr)->process];
            if ((*itr)->exit < startTime || (*itr)->enter > stopTime) // Out of time span
                continue;
            if (position < floor(startProcess) || position > ceil(startProcess + processSpan)) // Out of span
                continue;

            // save step information for emitting
            step = (*itr)->step;
            if (step > stopStep)
                stopStep = step;
            if (step < startStep)
                startStep = step;


            w = ((*itr)->exit - (*itr)->enter) / 1.0 / timeSpan;
            if (w < 2)
                continue;

            y = floor((position - startProcess) * blockheight) + 1;
            x = floor(((*itr)->enter - startTime) / 1.0 / timeSpan * rect().width()) + 1;
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
                w -= fabs(x);
                x = 0;
                complete = false;
            } else if (x + w > rect().width()) {
                w = rect().width() - x;
                complete = false;
            }
            painter->fillRect(QRectF(x, y, w, h), QBrush(QColor(250, 250, 255)));
            if (complete)
                painter->drawRect(QRectF(x,y,w,h));
            else
                incompleteBox(painter, x, y, w, h);

            for (QVector<Message *>::Iterator mitr = (*itr)->messages->begin(); mitr != (*itr)->messages->end(); ++mitr)
                drawMessages.insert((*mitr));
        }

        // Messages
        // We need to do all of the message drawing after the event drawing
        // for overlap purposes
        painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
        Event * send_event;
        Event * recv_event;
        QPointF p1, p2;
        for (QSet<Message *>::Iterator itr = drawMessages.begin(); itr != drawMessages.end(); ++itr) {
            send_event = (*itr)->sender;
            recv_event = (*itr)->receiver;
            position = proc_to_order[send_event->process];
            y = floor((position - startProcess + 0.5) * blockheight) + 1;
            x = floor((send_event->enter - startTime) / 1.0 / timeSpan * rect().width()) + 1;
            p1 = QPointF(x, y);
            position = proc_to_order[recv_event->process];
            y = floor((position - startProcess + 0.5) * blockheight) + 1;
            x = floor((recv_event->exit - startTime) / 1.0 / timeSpan * rect().width()) + 1;
            p2 = QPointF(x, y);
            painter->drawLine(p1, p2);
        }
}
