//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Ravel.
// Written by Kate Isaacs, kisaacs@acm.org, All rights reserved.
// LLNL-CODE-663885
//
// For details, see https://github.com/scalability-llnl/ravel
// Please also see the LICENSE file for our notice and the LGPL.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//////////////////////////////////////////////////////////////////////////////
#include "clustervis.h"
#include "clustertreevis.h"
#include "trace.h"
#include "gnome.h"

#include <QMouseEvent>
#include <QWheelEvent>

#include <cmath>

ClusterVis::ClusterVis(ClusterTreeVis *ctv, QWidget* parent,
                       VisOptions *_options)
    : TimelineVis(parent, _options),
      drawnGnomes(QMap<Gnome *, QRect>()),
      selected(NULL),
      treevis(ctv),
      hover_gnome(NULL)
{
}

// Standard initialization
void ClusterVis::setTrace(Trace * t)
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
}

// Just like stepvis
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
    stepSpan = stop - start;
    jumped = jump;

    if (!closed)
    {
        repaint();
    }
}

// Handle dragging and hovering
void ClusterVis::mouseMoveEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    // We only drag in the horizontal for this vis
    if (mousePressed)
    {
        lastStartStep = startStep;
        int diffx = mousex - event->x();
        startStep += diffx / 1.0 / stepwidth;

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

        mousex = event->x();
        mousey = event->y();

        repaint();
        changeSource = true;
        emit stepsChanged(startStep, startStep + stepSpan, false);
    }
    else // potential hover - forward to containing gnome & possibly change focus gnome
    {
        mousex = event->x();
        mousey = event->y();
        Gnome * focus_gnome = treevis->getGnome();
        bool emit_flag = false;
        if (hover_gnome && drawnGnomes[hover_gnome].contains(mousex, mousey))
        {
            if (hover_gnome->handleHover(event))
                repaint();
            if (hover_gnome != focus_gnome)
            {
                treevis->setGnome(hover_gnome);
                emit_flag = true;
            }
        }
        else
        {
            hover_gnome = NULL;
            for (QMap<Gnome *, QRect>::Iterator grect = drawnGnomes.begin();
                 grect != drawnGnomes.end(); ++grect)
            {
                if (grect.value().contains(mousex, mousey))
                {
                    hover_gnome = grect.key();
                    hover_gnome->handleHover(event);
                    if (hover_gnome != focus_gnome)
                    {
                        treevis->setGnome(hover_gnome);
                        emit_flag = true;
                    }
                }
            }
            repaint();
        }
        if (emit_flag)
            emit(focusGnome());
    }

}

// Zoom, but only in the step direction
void ClusterVis::wheelEvent(QWheelEvent * event)
{
    if (!visProcessed)
        return;

    if (Qt::MetaModifier && event->modifiers()) {
        // Vertical - Doesn't make sense here so do nothing
        // so that it doesn't behave differently from the other vis
        //float avgProc = startEntity + entitySpan / 2.0;
        //entitySpan *= scale;
        //startEntity = avgProc - entitySpan / 2.0;
    } else {
        // Horizontal
        float scale = 1;
        int clicks = event->delta() / 8 / 15;
        scale = 1 + clicks * 0.05;

        lastStartStep = startStep;
        float avgStep = startStep + stepSpan / 2.0;
        stepSpan *= scale;
        startStep = avgStep - stepSpan / 2.0;

        repaint();
        changeSource = true;
        emit stepsChanged(startStep, startStep + stepSpan, false);
    }

}


// Forward click to gnome. Based on return value of gnome, possibly
// signal.
void ClusterVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    int x = event->x();
    int y = event->y();

    // Reset selection for other gnomes -- the gnomes can't tell if one is
    // selected and another is not, so this effectively clears the selection in
    // the old gnome when a different one has become selected. We should
    // probably just save the one that's currently selected, but this list is
    //fairly short and we need it anyway for other interactions.
    for (QMap<Gnome *, QRect>::Iterator gnome = drawnGnomes.begin();
         gnome != drawnGnomes.end(); ++gnome)
    {
        gnome.key()->setSelected(false);
    }

    for (QMap<Gnome *, QRect>::Iterator gnome = drawnGnomes.begin();
         gnome != drawnGnomes.end(); ++gnome)
    {
        if (gnome.value().contains(x,y))
        {
            Gnome * g = gnome.key();
            Gnome::ChangeType change = g->handleDoubleClick(event);
            repaint();
            if (change == Gnome::CHANGE_CLUSTER) // Clicked to open Cluster
            {
                changeSource = true;
                emit(clusterChange());
            }
            else if (change == Gnome::CHANGE_SELECTION) // Selected a cluster
            {
                changeSource = true;
                PartitionCluster * pc = g->getSelectedPartitionCluster();
                if (pc)
                {
                    changeSource = false;
                    g->setSelected(true);
                    emit(entitiesSelected(*(pc->members), g));

                }
                else
                {
                    emit(entitiesSelected(QList<int>(), NULL));
                }
            }
            return;
        }
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


// Called before drawing begins
void ClusterVis::prepaint()
{
    closed = false;
    drawnGnomes.clear();

    // Figure out what partitions we need to search in hopefully a more
    // efficient manner than iterating through them all by making use of
    // where we were before and the fact we probably haven't moved that much.

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
         // check earlier partitions in case wee need to move the start back
        if (startStep < lastStartStep)
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
             // See if we've advanced the start forward
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

}

void ClusterVis::qtPaint(QPainter *painter)
{
    if(!visProcessed)
        return;

    // In this case we haven't already drawn stuff with GL, so we QtPaint it.
    if (rect().width() / stepSpan >= 3)
        paintEvents(painter);

    // Hover is independent of how we drew things
    drawHover(painter);
}

// If there are too many objects, use GL
void ClusterVis::drawNativeGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!visProcessed)
        return;

    int effectiveHeight = rect().height();
    if (rect().width() / stepSpan >= 3)
        return;

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
    glOrtho(0, effectiveSpan, 0, entitySpan, 0, 1);

    float barwidth = 1.0;
    float barheight = 1.0;
    entityheight = height/ entitySpan;
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
        if (part->gnome)
        {
            // The y value here of 0 isn't general... we need another structure
            // to keep track of how much y is used when we're doing the gnome
            // thing.
            part->gnome->drawGnomeGL(QRect(barwidth
                                           * (part->min_global_step - startStep),
                                           0,
                                           barwidth,
                                           barheight),
                                     options);
            continue;
        }
    }

}

// If there are few enough objects, use Qt
void ClusterVis::paintEvents(QPainter * painter)
{

    int effectiveHeight = rect().height();
    int effectiveWidth = rect().width();

    // Figure out blockwidth to be observed across gnomes, if we're in Qt land,
    // that should be greater than 1.
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
    stepwidth = blockwidth;

    // In case there's no hover-gnome this gets used
    Gnome * leftmost = NULL;
    Gnome * nextgnome = NULL;

    // Given for each gnome
    float drawSpan;
    float drawStart;

    // Draw active partitions gnomes
    for (int i = startPartition; i < trace->partitions->length(); ++i)
    {
        part = trace->partitions->at(i);
        if (part->min_global_step > topStep)
            break;
        else if (part->max_global_step < bottomStep)
            continue;
        if (part->gnome) {
            // The y value here of 0 isn't general... we need another structure
            // to keep track of how much y is used when we're doing the gnome
            // thing.
            drawSpan = part->max_global_step - part->min_global_step
                       - aggOffset + 1;
            drawStart = part->min_global_step + aggOffset - startStep;
            if (!options->showAggregateSteps)
            {
                drawSpan = (part->max_global_step - part->min_global_step)
                           / 2 + 1;
                drawStart = (part->min_global_step - startStep) / 2;
            }
            QRect gnomeRect = QRect(labelWidth + blockwidth * drawStart, 0,
                                    blockwidth * (drawSpan),
                                    part->events->size() / 1.0
                                    / trace->num_entities * effectiveHeight);
            part->gnome->drawGnomeQt(painter, gnomeRect, options, blockwidth);
            drawnGnomes[part->gnome] = gnomeRect;
            if (!leftmost)
                leftmost = part->gnome;
            else if (!nextgnome)
                nextgnome = part->gnome;
            continue;
        }

    }

    if (!treevis->getGnome())
    {
        QRect left = drawnGnomes[leftmost];
        float left_steps = (std::min(rect().width(), left.x() + left.width())
                            - std::max(0, left.x())) / 1.0 / blockwidth;
        if (left_steps < 2)
            treevis->setGnome(nextgnome);
        else
            treevis->setGnome(leftmost);
    }

}

// When the neighborhood slider changes, this is called
void ClusterVis::changeNeighborRadius(int neighbors)
{
    if (treevis->getGnome())
    {
        treevis->getGnome()->setNeighbors(neighbors);
        emit(neighborChange(neighbors));
        repaint();
    }
}

// Selected event comes from other vis
void ClusterVis::selectEvent(Event * event, bool aggregate, bool overdraw)
{
    if (selected_gnome)
        selected_gnome->clearSelectedPartitionCluster();
    TimelineVis::selectEvent(event, aggregate, overdraw);

}
