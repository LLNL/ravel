#ifndef OVERVIEWVIS_H
#define OVERVIEWVIS_H

#include "viswidget.h"

class OverviewVis : public VisWidget
{
public:
    OverviewVis();
    void setSteps(int start, int stop);
    void setTrace(Trace * t);
    void resizeEvent(QResizeEvent * event);
    void processVis();
    void paint(QPainter *painter, QPaintEvent *event, int elapsed);

private:
    unsigned long long minTime;
    unsigned long long maxTime;
    int maxStep;
    int startCursor;
    int stopCursor;
    int border;
    int height;
    QVector<float> heights;
    QVector<std::pair<int, int> > stepPositions;
};

#endif // OVERVIEWVIS_H
