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

class VisWidget : public QGLWidget
{
    Q_OBJECT
public:
    VisWidget(QWidget *parent = 0, VisOptions * _options = new VisOptions());
    ~VisWidget();
    virtual void setTrace(Trace *t);
    virtual void processVis();
    QSize sizeHint() const;

    void setClosed(bool _closed);
    void setVisOptions(VisOptions * _options);
    QWidget * container;

signals:
    void repaintAll();
    void stepsChanged(float start, float stop, bool jump);
    void eventClicked(Event * evt);


public slots:
    virtual void setSteps(float start, float stop, bool jump = false);
    virtual void selectEvent(Event *);

protected:
    void initializeGL();
    void paintEvent(QPaintEvent *event);
    void incompleteBox(QPainter *painter, float x, float y, float w, float h, QRect *extents);
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
    QMap<Event *, QRect> drawnEvents;
    Event * selected_event;
    Event * hover_event;
    bool closed;

    static const int initStepSpan = 15;
    static const int timescaleHeight = 20;
    static const int timescaleTickHeight = 5;
};

#endif // VISWIDGET_H
