#ifndef STEPVIS_H
#define STEPVIS_H

#include "timelinevis.h"

class StepVis : public TimelineVis
{
    Q_OBJECT
public:
    StepVis(QWidget* parent = 0, VisOptions *_options = new VisOptions());
    ~StepVis();
    void setTrace(Trace * t);

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);

public slots:
    void setSteps(float start, float stop, bool jump = false);

protected:
    void qtPaint(QPainter *painter);
    void drawNativeGL();
    void paintEvents(QPainter *painter);
    void prepaint();
    void drawColorBarGL();
    void drawColorBarText(QPainter * painter);
    void drawLine(QPainter * painter, QPointF * p1, QPointF * p2, int effectiveHeight);
    void setupMetric();

private:
    long long maxMetric;
    QString cacheMetric;
    QString maxMetricText;
    int maxMetricTextWidth;
    float colorbar_offset;

    static const int spacingMinimum = 12;
    static const int colorBarHeight = 24;
};

#endif // STEPVIS_H
