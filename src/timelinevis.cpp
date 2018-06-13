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
#include "timelinevis.h"
#include <iostream>
#include <cmath>

#include <QLocale>
#include <QMouseEvent>
#include <QCursor>
#include <QBitmap>

#include "trace.h"
#include "event.h"
#include "function.h"
#include "entity.h"
#include "primaryentitygroup.h"

TimelineVis::TimelineVis(QWidget* parent, VisOptions * _options)
    : VisWidget(parent = parent, _options),
      jumped(false),
      mousePressed(false),
      rightPressed(false),
      mousex(0),
      mousey(0),
      pressx(0),
      pressy(0),
      stepwidth(0),
      entityheight(0),
      labelWidth(0),
      labelHeight(0),
      labelDescent(0),
      cursorWidth(0),
      maxTime(0),
      maxEntities(0),
      startPartition(0),
      startTime(0),
      startEntity(0),
      timeSpan(0),
      entitySpan(0),
      lastStartTime(0)
{
    setMouseTracking(true);
    cursorWidth = 16;
    if (cursor().bitmap())
    {
        cursorWidth = cursor().bitmap()->size().width() / 2;
    }
}

TimelineVis::~TimelineVis()
{

}

void TimelineVis::processVis()
{
    // Determine needs for entity labels
    int max_entity = pow(10,ceil(log10(maxEntities)) + 1) - 1;
    QPainter * painter = new QPainter();
    painter->begin(this);
    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    QLocale systemlocale = QLocale::system();
    QFontMetrics font_metrics = painter->fontMetrics();
    QString testString = systemlocale.toString(max_entity);
    labelWidth = font_metrics.width(testString);
    if (trace->processingElements)
    {
        for (QList<Entity *>::Iterator ent = trace->processingElements->entities->begin();
             ent != trace->processingElements->entities->end(); ++ent)
        {
            labelWidth = std::max(labelWidth, font_metrics.width((*ent)->name));
        }
        labelWidth += 2;
    }
    labelHeight = font_metrics.height();
    labelDescent = font_metrics.descent();
    painter->end();
    delete painter;

    visProcessed = true;
}

// Figure out which event has been selected.
// Relies on drawnEvents which is set by child classes.
void TimelineVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    int x = event->x();
    int y = event->y();
    for (QMap<Event *, QRect>::Iterator evt = drawnEvents.begin();
         evt != drawnEvents.end(); ++evt)
    {
        if (evt.value().contains(x,y)) // We've found the event
        {
            if (evt.key() == selected_event) // We were in this event
            {
                selected_event = NULL;
            }
            else // This is a new event to us
            {
                selected_event = evt.key();
            }
            break;
        }
    }

    changeSource = true;
    emit eventClicked(selected_event);
    repaint();
}


void TimelineVis::mousePressEvent(QMouseEvent * event)
{
    mousePressed = true;
    rightPressed = false;
    if (event->button() == Qt::RightButton)
    {
        rightPressed = true;
        if( Qt::ControlModifier && event->modifiers() )
            rightClickEvent(event);
    }
    mousex = event->x();
    mousey = event->y();
    pressx = mousex;
    pressy = mousey;
}

void TimelineVis::mouseReleaseEvent(QMouseEvent * event)
{
    mousePressed = false;
    // Treat single click as double for now
    if (event->x() == pressx && event->y() == pressy)
        mouseDoubleClickEvent(event);
    else if (rightPressed)
    {
        rightPressed = false;
        rightDrag(event);
    }
}

void TimelineVis::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hover_event = NULL;
}

// We can either select a single event exclusive-or select a
// number of entities in a gnome right now.
void TimelineVis::selectEvent(Event * event, bool aggregate, bool overdraw)
{
    selected_entities.clear();
    selected_event = event;
    if (changeSource) {
        changeSource = false;
        return;
    }
    if (!closed)
        repaint();
}

void TimelineVis::selectEntities(QList<int> entities)
{
    selected_entities = entities;
    selected_event = NULL;
    if (changeSource) {
        changeSource = false;
        return;
    }
    if (!closed)
        repaint();
}

void TimelineVis::drawHover(QPainter * painter)
{
    if (!visProcessed || hover_event == NULL)
        return;

    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = painter->fontMetrics();

    QString text = "";
    if (hover_event->caller)
    {
        text = trace->functions->value(hover_event->caller->function)->name;
        text += " : ";
    }
    text += trace->functions->value(hover_event->function)->name;


    // Determine bounding box of FontMetrics
    QRect textRect = font_metrics.boundingRect(text);

    // Draw bounding box
    painter->setPen(QPen(QColor(200, 200, 0, 150), 1.0, Qt::SolidLine));
    painter->drawRect(QRectF(mousex + cursorWidth, mousey,
                             textRect.width() + 4, textRect.height() + 4));
    painter->setPen(QPen(QColor(255, 255, 0, 150), 1.0, Qt::SolidLine));
    painter->fillRect(QRectF(mousex + cursorWidth, mousey,
                             textRect.width() + 4, textRect.height() + 4),
                      QBrush(QColor(255, 255, 144, 150)));

    // Draw text
    painter->setPen(Qt::black);
    painter->drawText(mousex + 2 + cursorWidth, mousey + textRect.height() - 2, text);
}

void TimelineVis::drawEntityLabels(QPainter * painter, int effectiveHeight,
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

    if (trace->processingElements)
    {
        for (int i = start; i <= end; i+= skip) // Do this by order
        {
            y = floor((i - startEntity) * barHeight) + 1 + barHeight / 2
                    + labelDescent;
            if (y < effectiveHeight)
                painter->drawText(1, y, trace->processingElements->entities->at(i)->name);
        }
    }
    else
    {
        for (int i = start; i <= end; i+= skip) // Do this by order
        {
            y = floor((i - startEntity) * barHeight) + 1 + barHeight / 2
                + labelDescent;
            if (y < effectiveHeight)
                painter->drawText(1, y, QString::number(i));
        }
    }
}
