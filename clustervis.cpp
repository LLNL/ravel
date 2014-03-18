#include "clustervis.h"

ClusterVis::ClusterVis(QWidget* parent, VisOptions *_options)
    : TimelineVis(parent, _options),
      drawnGnomes(QMap<Gnome *, QRect>())
{
}

void ClusterVis::setTrace(Trace * t)
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
}

void ClusterVis::setSteps(float start, float stop, bool jump)
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
    {
        repaint();
    }
}

void ClusterVis::mouseMoveEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    if (mousePressed)
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
void ClusterVis::wheelEvent(QWheelEvent * event)
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


void ClusterVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    int x = event->x();
    int y = event->y();
    for (QMap<Gnome *, QRect>::Iterator gnome = drawnGnomes.begin(); gnome != drawnGnomes.end(); ++gnome)
        if (gnome.value().contains(x,y))
        {
            Gnome * g = gnome.key();
            g->handleDoubleClick(event);
            repaint();
            changeSource = true;
            emit(clusterChange());
            return;
        }

    // If we get here, fall through to normal behavior
    TimelineVis::mouseDoubleClickEvent(event);
}

void ClusterVis::clusterChanged()
{
    if (changeSource)
    {
        changeSource = false;
        return;
    }

    repaint();
}

void ClusterVis::prepaint()
{
    drawnGnomes.clear();
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

}

void ClusterVis::qtPaint(QPainter *painter)
{
    if(!visProcessed)
        return;

    // In this case we haven't already drawn stuff with GL, so we paint it here.
    if (rect().height() / processSpan >= 3 && rect().width() / stepSpan >= 3)
      paintEvents(painter);

    // Hover is independent of how we drew things
    drawHover(painter);
}


void ClusterVis::drawNativeGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!visProcessed)
        return;

    int effectiveHeight = rect().height();

    // Setup viewport
    int width = rect().width();
    int height = effectiveHeight;
    float effectiveSpan = stepSpan;
    if (!(options->showAggregateSteps))
        effectiveSpan /= 2.0;

    glViewport(0,
               0,
               width,
               height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, effectiveSpan, 0, processSpan, 0, 1);

    float barwidth = 1.0;
    float barheight = 1.0;
    processheight = height/ processSpan;
    stepwidth = width / effectiveSpan;

    // Process events for values
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
        if (part->gnome) {
            // The y value here of 0 isn't general... we need another structure to keep track of how much
            // y is used when we're doing the gnome thing.
            part->gnome->drawGnomeGL(QRect(barwidth * (part->min_global_step - startStep), 0,
                                           barwidth, barheight),
                                     options);
            continue;
        }
    }

}

void ClusterVis::paintEvents(QPainter * painter)
{

    int effectiveHeight = rect().height();
    int effectiveWidth = rect().width();

    int partitionCount = 0;
    int aggOffset = 0;
    float blockwidth;
    if (options->showAggregateSteps)
    {
        blockwidth = floor(effectiveWidth / stepSpan);
        aggOffset = -1;
    }
    else
    {
        blockwidth = floor(effectiveWidth / (ceil(stepSpan / 2.0)));
    }

    painter->setPen(QPen(QColor(0, 0, 0)));
    Partition * part = NULL;
    int topStep = boundStep(startStep + stepSpan) + 1;
    int bottomStep = floor(startStep) - 1;
    //Gnome * leftmost = NULL;
    //Gnome * nextgnome = NULL;
    //std::cout << " Step span is " << bottomStep << " to " << topStep << " and startPartition is ";
    //std::cout << trace->partitions->at(startPartition)->min_global_step << " to " << trace->partitions->at(startPartition)->max_global_step << std::endl;
    for (int i = startPartition; i < trace->partitions->length(); ++i)
    {
        part = trace->partitions->at(i);
        if (part->min_global_step > topStep)
            break;
        else if (part->max_global_step < bottomStep)
            continue;
        if (part->gnome) {
            // The y value here of 0 isn't general... we need another structure to keep track of how much
            // y is used when we're doing the gnome thing.
            QRect gnomeRect = QRect(partitionCount * labelWidth + blockwidth * (part->min_global_step + aggOffset - startStep), 0,
                                    blockwidth * (part->max_global_step - part->min_global_step - aggOffset + 1),
                                    part->events->size() / 1.0 / trace->num_processes * effectiveHeight);
            part->gnome->drawGnomeQt(painter, gnomeRect, options);
            drawnGnomes[part->gnome] = gnomeRect;
            /*if (!leftmost)
                leftmost = part->gnome;
            else if (!nextgnome)
                nextgnome = part->gnome;*/
            continue;
        }

    }

    /*
    QRect left = drawnGnomes[leftmost];
    float left_steps = (std::min(rect().width(), left.x() + left.width()) - std::max(0, left.x())) / 1.0 / blockwidth;
    if (left_steps < 2)
        emit focusGnome(nextgnome);
    else
        emit focusGnome(leftmost);

    QList<Gnome *> max_gnomes = QList<Gnome *>();
    float max_step_portion = 0; // # of steps represented over the total for the partition
    float left_gnome_portion = 0;
    int steps, start, stop;
    float display_steps, portion;
    for (QMap<Gnome*, QRect>::Iterator gr = drawnGnomes->begin(); gr != drawnGnomes.end(); ++gr)
    {
        QRect grect = gr.value();
        Gnome * gnome = gr.key();
        steps = gnome->partition->max_global_step - gnome->partition->min_global_step + 1;
        start = grect.x();
        stop = grect.x() + grect.width();
        if (start < 0)
            start = 0;
        if (stop > rect().width())
            stop = rect().width();
        display_steps = (stop - start) / 1.0 / blockwidth;
        portion = display_steps / steps;
        if (portion > max_step_portion)
        {
            max_gnomes.clear();
            max_gnomes.append(gnome);
            max_step_portion = portion;
        }
        else if (fabs(max_step_portion - portion) < 1e-6)
        {
            max_gnomes.append(gnome);
        }
        if (gnome == leftmost)
            left_gnome_portion = portion;
    }
    */
}

