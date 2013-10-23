#ifndef VISWIDGET_H
#define VISWIDGET_H

#include <QGLWidget>
#include <QVector>
#include <climits>

#include "trace.h"

class VisWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit VisWidget(QWidget *parent = 0);
    virtual void setTrace(Trace *t);
    virtual void paint(QPainter *painter, QPaintEvent *event, int elapsed);
    virtual void processVis();

signals:
    void repaintAll();
    void stepsChanged(float start, float stop);


public slots:
    virtual void setSteps(float start, float stop);

protected:
    void paintEvent(QPaintEvent *event);

protected:
    Trace * trace;
    bool visProcessed;
    QBrush backgroundColor;
    QBrush selectColor;
    bool changeSource;
};

#endif // VISWIDGET_H
