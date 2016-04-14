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
class Gnome;
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
    virtual void drawDelayTracking(QPainter * painter, CommEvent * c)
        { Q_UNUSED(painter); Q_UNUSED(c); }

signals:
    void repaintAll();
    void stepsChanged(float start, float stop, bool jump);
    void eventClicked(Event * evt, bool aggregate, bool overdraw);
    void entitiesSelected(QList<int> processes, Gnome * gnome);

public slots:
    virtual void setSteps(float start, float stop, bool jump = false);
    virtual void selectEvent(Event *, bool, bool);
    virtual void selectEntities(QList<int> entities, Gnome * gnome);

protected:
    void initializeGL();
    void paintEvent(QPaintEvent *event);
    void incompleteBox(QPainter *painter,
                       float x, float y, float w, float h, QRect *extents);
    int boundStep(float step); // Determine upper bound on step

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
    Gnome * selected_gnome;
    Event * selected_event;
    bool selected_aggregate;
    bool overdraw_selected;
    Event * hover_event;
    bool hover_aggregate;
    bool closed;

    static const int initStepSpan = 15;
    static const int timescaleHeight = 20;
    static const int timescaleTickHeight = 5;
};

#endif // VISWIDGET_H
