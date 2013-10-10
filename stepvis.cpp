#include "stepvis.h"
#include <QPaintEvent>
#include <iostream>
#include <cmath>

StepVis::StepVis(QWidget* parent) : VisWidget(parent = parent)
{
    // GLWidget options
    setMinimumSize(200, 200);
    setAutoFillBackground(false);

    // Set painting variables
    backgroundColor = QBrush(QColor(255, 255, 255));
    visProcessed = false;
    border = 20;
    mousePressed = false;
    showAggSteps = true;
    trace = NULL;

    // Create color map
    colormap = new ColorMap(QColor(), 0);
    colormap->addColor(QColor(), 0.5);
    colormap->addColor(QColor(), 1);
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
    stopStep = 16;
    startProcess = 1;
    stopProcess = trace->num_processes;
    stepSpan = stopStep - startStep + 1;
    processSpan = stopProcess - startProcess + 1;
}

void StepVis::setSteps(float start, float stop)
{

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
}

void StepVis::mouseReleaseEvent(QMouseEvent * event)
{
    mousePressed = false;
}

void StepVis::mouseMoveEvent(QMouseEvent * event)
{
    if (mousePressed) {

    }
}

void StepVis::wheelEvent(QWheelEvent * event)
{

}

void StepVis::paint(QPainter *painter, QPaintEvent *event, int elapsed)
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


    float blockheight = rect().height() / processSpan;
    float blockwidth = rect().width() / stepSpan;
    float barheight = blockheight - process_spacing;
    float barwidth = blockwidth - step_spacing;
    float x, y, w, h;
    int position;
    bool complete;
    painter->setPen(QPen(QColor(0, 0, 0)));
    for (QVector<Event *>::Iterator itr = trace->events->begin(); itr != trace->events->end(); itr++) {
        position = proc_to_order[(*itr)->process];
        if ((*itr)->step < floor(startStep) || (*itr)->step > ceil(stopStep)) // Out of span
            continue;
        if (position < floor(startProcess) || position > ceil(stopProcess)) // Out of span
            continue;
        // 0 = startProcess, rect().height() = stopProcess
        // 0 = startStep, rect().width() = stopStep
        y = (position - startProcess) * blockheight;
        x = ((*itr)->step - startStep) * blockwidth;
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

    }

}
