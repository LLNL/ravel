#include "timevis.h"
#include <iostream>

TimeVis::TimeVis(QWidget * parent) : VisWidget(parent = parent)
{
    // Set painting variables
    backgroundColor = QColor(255, 255, 255);
    mousePressed = false;
    trace = NULL;
    stepToTime = new QVector<TimePair *>();
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
    startProcess = 0;
    processSpan = trace->num_processes;

    // Determine time information
    minTime = ULLONG_MAX;
    maxTime = 0;
    startTime = ULLONG_MAX;
    unsigned long long stopTime = 0;
    maxStep = trace->global_max_step;
    for (QList<Partition*>::Iterator part = trace->partitions->begin(); part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list) {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt) {
                if ((*evt)->exit > maxTime)
                    maxTime = (*evt)->exit;
                if ((*evt)->enter < minTime)
                    minTime = (*evt)->enter;
                if ((*evt)->step >= boundStep(startStep) && (*evt)->enter < startTime)
                    startTime = (*evt)->enter;
                if ((*evt)->step <= boundStep(stopStep) && (*evt)->exit > stopTime)
                    stopTime = (*evt)->exit;
            }
        }
    }
    timeSpan = stopTime - startTime;

    for (QVector<TimePair *>::Iterator itr = stepToTime->begin(); itr != stepToTime->end(); itr++) {
        delete *itr;
        *itr = NULL;
    }
    delete stepToTime;

    stepToTime = new QVector<TimePair *>();
    for (int i = 0; i < maxStep/2 + 1; i++)
        stepToTime->insert(i, new TimePair(ULLONG_MAX, 0));
    int step;
    for (QList<Partition*>::Iterator part = trace->partitions->begin(); part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list) {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt) {
                if ((*evt)->step < 0)
                    continue;
                step = (*evt)->step / 2;
                if ((*stepToTime)[step]->start > (*evt)->enter)
                    (*stepToTime)[step]->start = (*evt)->enter;
                if ((*stepToTime)[step]->stop < (*evt)->exit)
                    (*stepToTime)[step]->stop = (*evt)->exit;
            }
        }
    }

}

void TimeVis::processVis()
{
    proc_to_order = QMap<int, int>();
    order_to_proc = QMap<int, int>();
    for (int i = 0; i < trace->num_processes; i++) {
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
        //startTime += rect().width() * diffx / 1.0 / timeSpan;
        startTime += timeSpan / 1.0 / rect().width() * diffx;
        startProcess += diffy / 1.0 / processheight;

        if (startTime < minTime)
            startTime = minTime;
        if (startTime > maxTime)
            startTime = maxTime;

        if (startProcess + processSpan > trace->num_processes + 1)
            startProcess = trace->num_processes - processSpan + 1;
        if (startProcess < 1)
            startProcess = 1;


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
    scale = 1 + clicks * 0.05;
    if (Qt::MetaModifier && event->modifiers()) {
        // Vertical
        float avgProc = startProcess + processSpan / 2.0;
        processSpan *= scale;
        startProcess = avgProc - processSpan / 2.0;
    } else {
        // Horizontal
        float middleTime = startTime + timeSpan / 2.0;
        timeSpan *= scale;
        startTime = middleTime - timeSpan / 2.0;
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
    startTime = (*stepToTime)[std::max(boundStep(start)/2, 0)]->start;
    timeSpan = (*stepToTime)[std::min(boundStep(stop)/2,  maxStep/2)]->stop - startTime;
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
    for (QList<Partition*>::Iterator part = trace->partitions->begin(); part != trace->partitions->end(); ++part)
    {
        for (QMap<int, QList<Event *> *>::Iterator event_list = (*part)->events->begin(); event_list != (*part)->events->end(); ++event_list)
        {
            for (QList<Event *>::Iterator evt = (event_list.value())->begin(); evt != (event_list.value())->end(); ++evt)
            {
                position = proc_to_order[(*evt)->process];
                if ((*evt)->exit < startTime || (*evt)->enter > stopTime) // Out of time span
                    continue;
                if (position < floor(startProcess) || position > ceil(startProcess + processSpan)) // Out of span
                    continue;

                // save step information for emitting
                step = (*evt)->step;
                if (step >= 0 && step > stopStep)
                    stopStep = step;
                if (step >= 0 && step < startStep) {
                    startStep = step;
                }


                w = ((*evt)->exit - (*evt)->enter) / 1.0 / timeSpan * rect().width();
                if (w < 2)
                    continue;

                y = floor((position - startProcess) * blockheight) + 1;
                x = floor(static_cast<long long>((*evt)->enter - startTime) / 1.0 / timeSpan * rect().width()) + 1;
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

                painter->fillRect(QRectF(x, y, w, h), QBrush(QColor(200, 200, 255)));
                if (complete)
                    painter->drawRect(QRectF(x,y,w,h));
                else
                    incompleteBox(painter, x, y, w, h);

                for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
                    drawMessages.insert((*msg));
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
        for (QSet<Message *>::Iterator msg = drawMessages.begin(); msg != drawMessages.end(); ++msg) {
            send_event = (*msg)->sender;
            recv_event = (*msg)->receiver;
            position = proc_to_order[send_event->process];
            y = floor((position - startProcess + 0.5) * blockheight) + 1;
            x = floor(static_cast<long long>(send_event->enter - startTime) / 1.0 / timeSpan * rect().width()) + 1;
            p1 = QPointF(x, y);
            position = proc_to_order[recv_event->process];
            y = floor((position - startProcess + 0.5) * blockheight) + 1;
            x = floor(static_cast<long long>(recv_event->exit - startTime) / 1.0 / timeSpan * rect().width()) + 1;
            p2 = QPointF(x, y);
            painter->drawLine(p1, p2);
        }
}
