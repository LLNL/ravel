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
#ifndef VISWIDGET_H
#define VISWIDGET_H

#include <QGLWidget>
#include <QList>
#include <QMap>
#include <QString>
#include <QRect>
#include <QColor>

#include "visoptions.h"
#include "commdrawinterface.h"

class VisOptions;
class Trace;
class QPainter;
class Message;
class QPaintEvent;
class Event;

class VisWidget : public QGLWidget, public CommDrawInterface
{
    Q_OBJECT
public:
    VisWidget(QWidget *parent = 0, VisOptions * _options = new VisOptions());
    ~VisWidget();
    virtual void setTrace(Trace *t);
    Trace * getTrace() { return trace; }
    virtual void processVis();
    void clear();
    virtual QSize sizeHint() const;

    void setClosed(bool _closed);
    bool isClosed() { return closed; }
    void setVisOptions(VisOptions * _options);
    QWidget * container;

    virtual int getHeight() { return rect().height(); }
    virtual void drawMessage(QPainter * painter, Message * msg)
        { Q_UNUSED(painter); Q_UNUSED(msg); }
    virtual void drawCollective(QPainter * painter, CollectiveRecord * cr)
        { Q_UNUSED(painter); Q_UNUSED(cr); }

signals:
    void repaintAll();
    void timeChanged(float start, float stop, bool jump);
    void eventClicked(Event * evt);
    void entitiesSelected(QList<int> processes);

public slots:
    virtual void setTime(float start, float stop, bool jump = false);
    virtual void selectEvent(Event *);
    virtual void selectEntities(QList<int> entities);

protected:
    void initializeGL();
    void paintEvent(QPaintEvent *event);
    void incompleteBox(QPainter *painter,
                       float x, float y, float w, float h, QRect *extents);
    int boundTime(float time); // Determine upper bound on step

    virtual void drawNativeGL();
    virtual void qtPaint(QPainter *painter);
    virtual void prepaint();
    QString drawTimescale(QPainter * painter, unsigned long long start,
                       unsigned long long span, int margin = 0);

private:
    void beginNativeGL();
    void endNativeGL();

protected:
    Trace * trace;
    bool visProcessed;
    VisOptions * options;
    QColor backgroundColor;
    QBrush selectColor;
    bool changeSource;
    int border;

    // Interactions
    QMap<Event *, QRect> drawnEvents;
    QList<int> selected_entities;
    Event * selected_event;
    Event * hover_event;
    bool closed;

    static const int initTimeSpan = 90000;
    static const int timescaleHeight = 20;
    static const int timescaleTickHeight = 5;
};

#endif // VISWIDGET_H
