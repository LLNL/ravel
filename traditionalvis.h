#ifndef TRADITIONALVIS_H
#define TRADITIONALVIS_H

#include "timelinevis.h"

class TraditionalVis : public TimelineVis
{
    Q_OBJECT
public:
    TraditionalVis(QWidget * parent = 0, VisOptions *_options = new VisOptions());
    ~TraditionalVis();
    void setTrace(Trace * t);

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);

public slots:
    void setSteps(float start, float stop, bool jump = false);

protected:
    void qtPaint(QPainter *painter);
    void paintEvents(QPainter *painter);

private:
    class TimePair {
    public:
        TimePair(unsigned long long _s1, unsigned long long _s2)
            : start(_s1), stop(_s2) {}

        unsigned long long start;
        unsigned long long stop;
    };

    unsigned long long minTime;
    unsigned long long maxTime;
    unsigned long long startTime;
    unsigned long long timeSpan;
    QVector<TimePair* > * stepToTime;
};

#endif // TRADITIONALVIS_H
