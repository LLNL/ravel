#ifndef TRADITIONALVIS_H
#define TRADITIONALVIS_H

#include "timelinevis.h"

// Physical timeline
class TraditionalVis : public TimelineVis
{
    Q_OBJECT
public:
    TraditionalVis(QWidget * parent = 0,
                   VisOptions *_options = new VisOptions());
    ~TraditionalVis();
    void setTrace(Trace * t);

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void rightDrag(QMouseEvent * event);

    void drawMessage(QPainter * painter, Message * message);
    void drawCollective(QPainter * painter, CollectiveRecord * cr);

signals:
    void timeScaleString(QString);

public slots:
    void setSteps(float start, float stop, bool jump = false);

protected:
    void qtPaint(QPainter *painter);

    // Paint MPI events we handle directly and thus can color.
    void paintEvents(QPainter *painter);

    void prepaint();
    void drawNativeGL();

    // Paint all other events available
    void paintNotStepEvents(QPainter *painter, Event * evt, float position,
                            int task_spacing, float barheight,
                            float blockheight, QRect * extents);

private:
    // For keeping track of map betewen real time and step time
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
    QRect lassoRect;
    float blockheight;

    int getX(CommEvent * evt);
    int getY(CommEvent * evt);
    int getW(CommEvent * evt);
};

#endif // TRADITIONALVIS_H
