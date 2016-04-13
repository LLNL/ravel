#include "stepvis.h"
#include "metricrangedialog.h"
#include "trace.h"
#include "rpartition.h"
#include "commevent.h"
#include "colormap.h"
#include "message.h"
#include "collectiverecord.h"
#include "otfcollective.h"
#include "event.h"
#include "p2pevent.h"
#include "collectiveevent.h"
#include "primaryentitygroup.h"
#include "entity.h"
#include <iostream>
#include <cmath>
#include <QLocale>
#include <QMouseEvent>
#include <QWheelEvent>

#include "function.h"

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
      ellipse_height(0),
      overdrawYMap(new QMap<int, int>()),
      groupColorMap(new QMap<int, QColor>())
{

}

StepVis::~StepVis()
{
    delete overdrawYMap;
    delete groupColorMap;
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
    startEntity = 0;
    entitySpan = trace->num_entities;
    maxEntities = trace->num_entities;
    startPartition = 0;

    maxStep = trace->global_max_step;
    setupMetric();

    int num_groups = trace->primaries->size();
    if (num_groups == 1)
    {
        groupColorMap->insert(0, QColor(Qt::white));
    }
    else
    {
        int top = 255;
        int bottom = 55;
        int separation = 200 / num_groups;
        if (separation > 16)
        {
            separation = 16;
            bottom = 255 - num_groups * separation;
        }
        bool flip = false;
        int offset = 0;
        int color;
        for (QMap<int, PrimaryEntityGroup *>::Iterator tg = trace->primaries->begin();
             tg != trace->primaries->end(); ++tg)
        {
            if (flip)
            {
                color = top - offset;
                groupColorMap->insert(tg.key(), QColor(color, color, color));
                offset += separation;
            }
            else
            {
                color = bottom + offset;
                groupColorMap->insert(tg.key(), QColor(color, color, color));
            }
            flip = !flip;
        }
    }
}

void StepVis::processVis()
{
    TimelineVis::processVis();

    QPainter * painter = new QPainter();
    painter->begin(this);
    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = painter->fontMetrics();
    if (trace->options.origin == ImportOptions::OF_CHARM)
    {
        int primary, entity;
        for (QMap<int, PrimaryEntityGroup *>::Iterator tg = trace->primaries->begin();
             tg != trace->primaries->end(); ++tg)
        {
            primary = font_metrics.width((*tg)->name);
            if (primary > labelWidth)
                labelWidth = primary;
            for (QList<Entity *>::Iterator t = (*tg)->entities->begin();
                 t != (*tg)->entities->end(); ++t)
            {
                entity = font_metrics.width((*t)->name);
                if (entity > labelWidth)
                    labelWidth = entity;
            }
        }
    } else {
        int idWidth;
        for (QMap<int, PrimaryEntityGroup *>::Iterator tg = trace->primaries->begin();
             tg != trace->primaries->end(); ++tg)
        {
            idWidth = font_metrics.width((*tg)->entities->length());
            if (idWidth  > labelWidth)
                labelWidth = idWidth ;
        }
    }
    painter->end();
    delete painter;
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
        maxMetricText = systemlocale.toString(maxMetric) + " "
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
        maxMetricText = systemlocale.toString(new_max) + " "
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
        startEntity = startEntity + entitySpan * pressy
                       / (rect().height() - colorBarHeight);
        entitySpan = entitySpan * (event->y() - pressy)
                      / (rect().height() - colorBarHeight);
    }
    else
    {
        startEntity = startEntity + entitySpan * event->y()
                       / (rect().height() - colorBarHeight);
        entitySpan = entitySpan * (pressy - event->y())
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
        startEntity += diffy / 1.0 / entityheight;

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

        if (startEntity < 0)
            startEntity = 0;
        if (startEntity + entitySpan > trace->num_entities)
            startEntity = trace->num_entities - entitySpan;

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
        if (mousey >= rect().height() - colorBarHeight) // color bar
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
                 && drawnEvents[hover_event].contains(mousex, mousey)) // fxn
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
        float avgProc = startEntity + entitySpan / 2.0;
        entitySpan *= scale;
        startEntity = avgProc - entitySpan / 2.0;
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
    closed = false;
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
    if ((rect().height() - colorBarHeight) / entitySpan >= 3
        && rect().width() / stepSpan >= 3)
    {
      paintEvents(painter);
    }

    // Always done by qt
    drawPrimaryLabels(painter, rect().height() - colorBarHeight,
                      entityheight);
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
    if (effectiveHeight / entitySpan >= 3 && rect().width() / stepSpan >= 3)
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
    glOrtho(0, effectiveSpan, 0, entitySpan, 0, 1);

    float barwidth = 1.0;
    float barheight = 1.0;
    entityheight = height/ entitySpan;
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
    double density = num_events / 1.0 / (stepSpan * entitySpan);
    float opacity = 1.0;
    float xoffset = 0;
    float yoffset = 0;

    // How we use density is by feel, we should improve this.
    // If density is one, we want low opacity for proper blending.
    // If density is very small, we want high opacity to make things
    // more visible.
    if (density)
    {
        if (entityheight < 1.0)
        {
            double entities_per_pixel = 1.0 / entityheight;
            yoffset = entities_per_pixel / 2;
            opacity = entityheight / density;
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
    float position; // placement of entity
    QColor color;
    float maxEntity = entitySpan + startEntity;
    float myopacity, opacity_multiplier = 1.0;
    if (selected_gnome && !selected_entities.isEmpty())
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
                && selected_entities.contains(proc_to_order[event_list.key()]))
            {
                selected = true;
            }

            position = proc_to_order[event_list.key()];
            // Out of entity span check
            if (position < floor(startEntity)
                || position > ceil(startEntity + entitySpan))
            {
                 continue;
            }
            y = (maxEntity - position) * barheight - 1;

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

                color = options->colormap->color((*evt)->getMetric(metric));  //(*(*evt)->metrics)[metric]->event);
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

                    color = options->colormap->color((*evt)->getMetric(metric, false)); //(*(*evt)->metrics)[metric]->aggregate);

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

    int entity_spacing = 0;
    if (effectiveHeight / entitySpan > spacingMinimum)
        entity_spacing = 3;

    int step_spacing = 0;
    if (effectiveWidth / stepSpan > spacingMinimum)
        step_spacing = 3;

    float x, y, w, h, xa, wa;
    blockheight = floor(effectiveHeight / entitySpan);
    if (options->showAggregateSteps)
    {
        blockwidth = floor(effectiveWidth / stepSpan);
    }
    else
    {
        blockwidth = floor(effectiveWidth / (ceil(stepSpan / 2.0)));
    }
    float barheight = blockheight - entity_spacing;
    float barwidth = blockwidth - step_spacing;
    entityheight = blockheight;
    stepwidth = blockwidth;
    QRect extents = QRect(0, 0, rect().width(), effectiveHeight);

    QString metric(options->metric);
    int position;
    bool complete, aggcomplete;
    QSet<CommBundle *> drawComms = QSet<CommBundle *>();
    QSet<CommBundle *> selectedComms = QSet<CommBundle *>();
    painter->setPen(QPen(QColor(0, 0, 0)));
    Partition * part = NULL;
    int topStep = boundStep(startStep + stepSpan) + 1;
    int bottomStep = floor(startStep) - 1;
    float myopacity, opacity = 1.0;
    if (selected_gnome && !selected_entities.isEmpty())
        opacity = 0.50;

    QList<int> overdraw_entities;

    // Draw backgrounds for primaries
    int tg_extent = 0;
    for (QMap<int, PrimaryEntityGroup *>::Iterator tg = trace->primaries->begin();
         tg != trace->primaries->end(); ++tg)
    {
        y = floor((tg_extent - startEntity) * blockheight) + 1;
        if (y > effectiveHeight)
            continue;
        h = tg.value()->entities->size() * entityheight;
        if (y + h > effectiveHeight)
            h = effectiveHeight - y;
        painter->fillRect(QRectF(labelWidth,
                                 y,
                                 effectiveWidth - labelWidth,
                                 h),
                          QBrush(groupColorMap->value(tg.key())));
        tg_extent += tg.value()->entities->size();
    }


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
                && selected_entities.contains(proc_to_order[event_list.key()]))
            {
                selected = true;
            }

            // Out of span test
            position = proc_to_order[event_list.key()];
            if (position < floor(startEntity)
                || position > ceil(startEntity + entitySpan))
            {
                continue;
            }
            y = floor((position - startEntity) * blockheight) + 1;

            for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
                 evt != (event_list.value())->end(); ++evt)
            {
                 // Out of step span test
                if ((*evt)->step < bottomStep || (*evt)->step > topStep)
                {
                    continue;
                }
                // 0 = startEntity, effectiveHeight = stopEntity (startEntity + entitySpan)
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
                if (step_spacing > 0 && entity_spacing > 0)
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
                if (*evt == selected_event)
                {
                    if (overdraw_selected)
                        overdraw_entities = (*evt)->neighborEntities();
                    (*evt)->addComms(&selectedComms);
                }

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
                    if (step_spacing > 0 && entity_spacing > 0)
                    {
                        if (aggcomplete)
                            painter->drawRect(QRectF(xa, y, wa, h));
                        else
                            incompleteBox(painter, xa, y, wa, h, &extents);
                    }
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
    if (options->showMessages != VisOptions::MSG_NONE)
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
            if (!selectedComms.contains(*comm))
                (*comm)->draw(painter, this);
        }

        // Now draw selected
        for (QSet<CommBundle *>::Iterator comm = selectedComms.begin();
             comm != selectedComms.end(); ++comm)
        {
            (*comm)->draw(painter, this);
        }
    }

    if (selected_event && options->traceBack)
    {
        selected_event->track_delay(painter, this);
    }

    if (overdraw_selected)
        overdrawSelected(painter, overdraw_entities);
}

// Might want to use something like this to also have a lens feature
void StepVis::overdrawSelected(QPainter * painter, QList<int> entities)
{
    // Figure out how big the overdraw window should be
    int selected_position = proc_to_order[selected_event->entity];
    QList<int> before_entities = QList<int>();
    QList<int> after_entities = QList<int>();
    QMap<int, int> position_map = QMap<int, int>();
    for (int i = 0; i < entities.size(); i++)
    {
        if (proc_to_order[entities[i]] < selected_position)
        {
            before_entities.append(proc_to_order[entities[i]]);
        }
        else
        {
            after_entities.append(proc_to_order[entities[i]]);
        }
    }
    qSort(before_entities);
    qSort(after_entities);
    for (int i = 0; i < before_entities.size(); i++)
        position_map.insert(before_entities[i], i);
    for (int i = 0; i < after_entities.size(); i++)
        position_map.insert(after_entities[i], i);

    if (blockheight > 3) // keep same height for non-GL
    {
        // We leave the selected where it is and draw the rest as floating

        // Prepare for comms
        // QSet<CommBundle *> drawBundles = QSet<CommBundle *>();

        // First we draw the background as white rectangles
        // Above
        painter->fillRect(QRect(labelWidth,
                                selected_position - blockheight * before_entities.size(),
                                rect().width(), blockheight * before_entities.size()),
                          QBrush(Qt::white));
        // Below
        painter->fillRect(QRect(labelWidth,
                                selected_position + blockheight,
                                rect().width(),
                                blockheight * after_entities.size()),
                          QBrush(Qt::white));

    }
    else // do something palatable for GL
    {
        // We will move the selected as needed since this acts much more like
        // a special lens

        // We may want to make this relative to how much screen space we have
        // but I'm not sure we want to use all of it because then we'll lose
        // context
        // int overdraw_height = 12;
    }


}

int StepVis::getY(CommEvent * evt)
{
    int y = 0;
    int position = proc_to_order[evt->entity];
    y = floor((position - startEntity) * blockheight) + 1;
    return y;
}

int StepVis::getX(CommEvent *evt)
{
    int x = 0;
    if (options->showAggregateSteps || !trace->use_aggregates) // Factor these calcs into own fxn!
        x = floor((evt->step - startStep) * blockwidth) + 1
            + labelWidth;
    else
        x = floor((evt->step - startStep) / 2 * blockwidth) + 1
            + labelWidth;

    return x;
}

void StepVis::drawDelayTracking(QPainter * painter, CommEvent * c)
{
    int penwidth = 1;
    if (entitySpan <= 32)
        penwidth = 2;

    CommEvent * current = c;
    CommEvent * backward = NULL;
    Qt::GlobalColor pencolor = Qt::green;
    while (current)
    {
        backward = current->compare_to_sender(current->pe_prev);

        if (!backward)
            break;

        pencolor = Qt::magenta; // queueing in magenta

        QPointF p1, p2;
        int y = getY(current);
        int x = getX(current);
        int w = blockwidth;
        int h = blockheight;

        if (options->showMessages == VisOptions::MSG_TRUE)
        {
            p1 = QPointF(x + w/2.0, y + h/2.0);
            y = getY(backward);
            x = getX(backward);
            p2 = QPointF(x + w/2.0, y + h/2.0);
        }
        else
        {
            p1 = QPointF(x, y + h/2.0);
            y = getY(backward);
            p2 = QPointF(x + w, y + h/2.0);
        }
        if (backward != current->pe_prev) // messages dashed over black
            painter->setPen(QPen(pencolor, penwidth, Qt::DotLine));
        else
            painter->setPen(QPen(pencolor, penwidth, Qt::SolidLine));
        drawLine(painter, &p1, &p2);

        current = backward;
    }
}


void StepVis::drawMessage(QPainter * painter, Message * msg)
{
    int penwidth = 1;
    if (entitySpan <= 32)
        penwidth = 2;

    Qt::GlobalColor pencolor = Qt::black;
    if (!selected_aggregate
            && (selected_event == msg->sender || selected_event == msg->receiver))
        pencolor = Qt::yellow;

    QPointF p1, p2;
    int y = getY(msg->sender);
    int x = getX(msg->sender);
    int w = blockwidth;
    int h = blockheight;

    if (options->showMessages == VisOptions::MSG_TRUE)
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
    painter->setPen(QPen(pencolor, penwidth, Qt::SolidLine));
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
        if (coll_event->entity == root)
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

        if (rooted && coll_event->entity == root)
        {
            painter->setBrush(QBrush());
        }

        y = getY(coll_event);
        x = getX(coll_event);
        if (rooted)
        {
            if (coll_event->entity == root)
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
        else if (rooted && coll_event->entity == root)
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
            if (other->entity == root)
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

    // Special handling for color bar
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


void StepVis::drawPrimaryLabels(QPainter * painter, int effectiveHeight,
                                float barHeight)
{
    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    painter->fillRect(0,0,labelWidth,effectiveHeight, QColor(Qt::white));
    int total_labels = floor(effectiveHeight / labelHeight);
    int y;
    int skip = 1;
    if (total_labels < entitySpan)
    {
        skip = ceil(float(entitySpan) / total_labels);
    }

    int start = std::max(floor(startEntity), 0.0);
    int end = std::min(ceil(startEntity + entitySpan),
                       maxEntities - 1.0);

    int current = start;
    int offset = 0;
    if (trace->options.origin == ImportOptions::OF_CHARM)
    {
        for (QMap<int, PrimaryEntityGroup *>::Iterator tg = trace->primaries->begin();
             tg != trace->primaries->end(); ++tg)
        {
            while (current < offset + (*tg)->entities->size() && current <= end)
            {
                y = floor((current - startEntity) * barHeight) + 1
                        + barHeight / 2 + labelDescent;
                if (y < effectiveHeight)
                {
                    if (current == offset)
                        painter->drawText(1, y, (*tg)->name);
                    else
                        painter->drawText(1, y,
                                          (*tg)->entities->at(current - offset)->name);
                }
                current += skip;
            }
            if (current > end)
                break;

            offset += (*tg)->entities->size();
        }
    } else {
        for (QMap<int, PrimaryEntityGroup *>::Iterator tg = trace->primaries->begin();
             tg != trace->primaries->end(); ++tg)
        {
            while (current < offset + (*tg)->entities->size() && current <= end)
            {
                y = floor((current - startEntity) * barHeight) + 1
                        + barHeight / 2 + labelDescent;
                if (y < effectiveHeight)
                {
                    painter->drawText(1, y,
                                          QString::number((*tg)->entities->at(current - offset)->id));
                }
                current += skip;
            }
            if (current > end)
                break;

            offset += (*tg)->entities->size();
        }
    }
}
