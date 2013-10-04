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
    virtual void setSteps(int start, int stop);
    virtual void paint(QPainter *painter, QPaintEvent *event, int elapsed);
    virtual void processVis();

signals:
    void repaintAll();

public slots:

protected:
    void paintEvent(QPaintEvent *event);

protected:
    Trace * trace;
    bool visProcessed;
    QBrush backgroundColor;
    QBrush selectColor;
};

#endif // VISWIDGET_H
