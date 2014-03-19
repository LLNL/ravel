#include "clustertreevis.h"

ClusterTreeVis::ClusterTreeVis(QWidget *parent, VisOptions * _options)
    : VisWidget(parent, _options),
      gnome(NULL)
{
    backgroundColor = palette().color(QPalette::Background);
    setAutoFillBackground(true);
}

QSize ClusterTreeVis::sizeHint() const
{
    return QSize(50, 50);
}


void ClusterTreeVis::setTrace(Trace * t)
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

void ClusterTreeVis::setSteps(float start, float stop, bool jump)
{
    if (!visProcessed)
        return;

    lastStartStep = startStep;
    startStep = start;
    stepSpan = stop - start + 1;
    jumped = jump;

    if (!closed)
        repaint();
}

void ClusterTreeVis::setGnome(Gnome * _gnome)
{
    gnome = _gnome;
    repaint();
}

void ClusterTreeVis::qtPaint(QPainter *painter)
{
    if (!visProcessed)
        return;

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
    Gnome * leftmost = NULL;
    Gnome * nextgnome = NULL;
    QMap<Gnome *, QRect> drawnGnomes = QMap<Gnome *, QRect>();
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
            drawnGnomes[part->gnome] = gnomeRect;
            if (!leftmost)
                leftmost = part->gnome;
            else if (!nextgnome)
                nextgnome = part->gnome;
            continue;
        }

    }

    QRect left = drawnGnomes[leftmost];
    float left_steps = (std::min(rect().width(), left.x() + left.width()) - std::max(0, left.x())) / 1.0 / blockwidth;
    if (left_steps < 2)
        gnome = nextgnome;
    else
        gnome = leftmost;

    if (gnome)
        gnome->drawQtTree(painter, rect());
}

void ClusterTreeVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (gnome)
    {
        gnome->handleTreeDoubleClick(event);
        changeSource = true;
        emit(clusterChange());
        repaint();
    }
}

void ClusterTreeVis::mousePressEvent(QMouseEvent * event)
{
    mouseDoubleClickEvent(event);
}

void ClusterTreeVis::clusterChanged()
{
    if (changeSource)
    {
        changeSource = false;
        return;
    }

    repaint();
}

void ClusterTreeVis::prepaint()
{
    if (!visProcessed)
        return;

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
