#ifndef TRADITIONALVIS_H
#define TRADITIONALVIS_H

#include "timelinevis.h"
#include <QLocale>

class TraditionalVis : public TimelineVis
{
    Q_OBJECT
public:
    TraditionalVis(QWidget * parent = 0);
    ~TraditionalVis();
    void setTrace(Trace * t);

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);

public slots:
    void setSteps(float start, float stop);

protected:
    void qtPaint(QPainter *painter);
    void paintEvents(QPainter *painter);
    void drawTimescale(QPainter *painter);

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

    static const int timescaleHeight = 20;
    static const int timescaleTickHeight = 5;

};

#endif // TRADITIONALVIS_H
