#ifndef VISWIDGET_H
#define VISWIDGET_H

#include <QGLWidget>
#include <QVector>
#include <QList>
#include <QPaintEvent>
#include <climits>
#include <cmath>

#include "trace.h"

class VisWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit VisWidget(QWidget *parent = 0);
    ~VisWidget();
    virtual void setTrace(Trace *t);
    virtual void processVis();
    QSize sizeHint() const;

    void setClosed(bool _closed);
    QWidget * container;

signals:
    void repaintAll();
    void stepsChanged(float start, float stop);
    void eventClicked(Event * evt);


public slots:
    virtual void setSteps(float start, float stop);
    virtual void selectEvent(Event *);

protected:
    void initializeGL();
    void paintEvent(QPaintEvent *event);
    void incompleteBox(QPainter *painter, float x, float y, float w, float h);
    int boundStep(float step); // Determine upper bound on step

    virtual void drawNativeGL();
    virtual void qtPaint(QPainter *painter);

private:
    void beginNativeGL();
    void endNativeGL();

protected:
    Trace * trace;
    bool visProcessed;
    QColor backgroundColor;
    QBrush selectColor;
    bool changeSource;
    int border;
    QMap<Event *, QRect> drawnEvents;
    Event * selected_event;
    Event * hover_event;
    bool closed;

    static const int initStepSpan = 15;
};

#endif // VISWIDGET_H
