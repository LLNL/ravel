#ifndef STEPVIS_H
#define STEPVIS_H

#include "timelinevis.h"

class MetricRangeDialog;
class CommEvent;

// Logical timeline vis
class StepVis : public TimelineVis
{
    Q_OBJECT
public:
    StepVis(QWidget* parent = 0, VisOptions *_options = new VisOptions());
    ~StepVis();
    void setTrace(Trace * t);
    void processVis();

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void rightDrag(QMouseEvent * event);

    // Saves colorbar range information
    MetricRangeDialog * metricdialog;

    int getHeight() { return rect().height() - colorBarHeight; }
    void drawMessage(QPainter * painter, Message * message);
    void drawCollective(QPainter * painter, CollectiveRecord * cr);
    void drawDelayTracking(QPainter * painter, CommEvent * c);


public slots:
    void setSteps(float start, float stop, bool jump = false);
    void setMaxMetric(long long int new_max);

protected:
    void qtPaint(QPainter *painter);
    void drawNativeGL();
    void paintEvents(QPainter *painter);
    void prepaint();
    void overdrawSelected(QPainter *painter, QList<int> tasks);
    void drawColorBarGL();
    void drawColorBarText(QPainter * painter);
    void drawCollective(QPainter * painter, CollectiveRecord * cr,
                        int ellipse_width, int ellipse_height,
                        int effectiveHeight);
    void drawLine(QPainter * painter, QPointF * p1, QPointF * p2);
    void drawArc(QPainter * painter, QPointF * p1, QPointF * p2,
                 int width, bool forward = true);
    void setupMetric();
    void drawColorValue(QPainter * painter);
    int getX(CommEvent * evt);
    int getY(CommEvent * evt);
    void drawPrimaryLabels(QPainter * painter, int effectiveHeight,
                           float barHeight);

private:
    double maxMetric;
    QString cacheMetric;
    QString maxMetricText;
    QString hoverText;
    int maxMetricTextWidth;
    float colorbar_offset;
    QRect lassoRect;
    float blockwidth;
    float blockheight;
    int ellipse_width;
    int ellipse_height;
    QMap<int, int> * overdrawYMap;

    QMap<int, QColor> * groupColorMap;

    static const int colorBarHeight = 24;
};

#endif // STEPVIS_H
