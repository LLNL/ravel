#ifndef VISWIDGET_H
#define VISWIDGET_H

#include <QGLWidget>
#include <QVector>
#include <QList>
#include <QPaintEvent>
#include <QLocale>
#include <climits>
#include <cmath>

#include "trace.h"
#include "visoptions.h"
#include "commdrawinterface.h"

class VisWidget : public QGLWidget, public CommDrawInterface
{
    Q_OBJECT
public:
    VisWidget(QWidget *parent = 0, VisOptions * _options = new VisOptions());
    ~VisWidget();
    virtual void setTrace(Trace *t);
    Trace * getTrace() { return trace; }
    virtual void processVis();
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
    void stepsChanged(float start, float stop, bool jump);
    void eventClicked(Event * evt, bool aggregate);
    void processesSelected(QList<int> processes, Gnome * gnome);

public slots:
    virtual void setSteps(float start, float stop, bool jump = false);
    virtual void selectEvent(Event *, bool);
    virtual void selectProcesses(QList<int> processes, Gnome * gnome);

protected:
    void initializeGL();
    void paintEvent(QPaintEvent *event);
    void incompleteBox(QPainter *painter,
                       float x, float y, float w, float h, QRect *extents);
    int boundStep(float step); // Determine upper bound on step

    virtual void drawNativeGL();
    virtual void qtPaint(QPainter *painter);
    virtual void prepaint();
    void drawTimescale(QPainter * painter, unsigned long long start,
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
    QList<int> selected_processes;
    Gnome * selected_gnome;
    Event * selected_event;
    bool selected_aggregate;
    Event * hover_event;
    bool hover_aggregate;
    bool closed;

    static const int initStepSpan = 15;
    static const int timescaleHeight = 20;
    static const int timescaleTickHeight = 5;
};

#endif // VISWIDGET_H
