#ifndef OVERVIEWVIS_H
#define OVERVIEWVIS_H

#include "viswidget.h"
#include "event.h"

class OverviewVis : public VisWidget
{
    Q_OBJECT
public:
    OverviewVis(QWidget *parent = 0);
    void setTrace(Trace * t);
    void processVis();
    void resizeEvent(QResizeEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

public slots:
    void setSteps(float start, float stop);

protected:
    void qtPaint(QPainter *painter);

private:
    int roundeven(float step);

    unsigned long long minTime;
    unsigned long long maxTime;
    int maxStep;
    int startCursor;
    int stopCursor;
    unsigned long long startTime;
    unsigned long long stopTime;
    int height;
    bool mousePressed;
    QVector<float> heights;
    QVector<std::pair<int, int> > stepPositions;
};

#endif // OVERVIEWVIS_H
