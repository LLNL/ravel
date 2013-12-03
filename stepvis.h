#ifndef STEPVIS_H
#define STEPVIS_H

#include "timelinevis.h"
#include "colormap.h"
#include "event.h"

class StepVis : public TimelineVis
{
    Q_OBJECT
public:
    StepVis(QWidget* parent = 0);
    ~StepVis();
    void setTrace(Trace * t);

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);

public slots:
    void setSteps(float start, float stop);

protected:
    void qtPaint(QPainter *painter);
    void drawNativeGL();
    void paintEvents(QPainter *painter);
    void prepaint();

private:
    bool showAggSteps;
    long long maxLateness;
    ColorMap * colormap;
};

#endif // STEPVIS_H
