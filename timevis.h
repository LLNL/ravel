#ifndef TIMEVIS_H
#define TIMEVIS_H

#include "viswidget.h"

class TimeVis : public VisWidget
{
    Q_OBJECT
public:
    TimeVis(QWidget * parent = 0);
    ~TimeVis();
    void setTrace(Trace * t);
    void processVis();

    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);

public slots:
    void setSteps(float start, float stop);

protected:
    void qtPaint(QPainter *painter);

private:
    class TimePair {
    public:
        TimePair(unsigned long long _s1, unsigned long long _s2)
            : start(_s1), stop(_s2) {}

        unsigned long long start;
        unsigned long long stop;
    };

    bool mousePressed;
    int mousex;
    int mousey;
    int processheight;
    unsigned long long minTime;
    unsigned long long maxTime;
    unsigned long long startTime;
    unsigned long long timeSpan;
    float startStep;
    float stopStep;
    int maxStep;
    float startProcess;
    float processSpan;
    QMap<int, int> proc_to_order;
    QMap<int, int> order_to_proc;
    QVector<TimePair* > * stepToTime;

};

#endif // TIMEVIS_H
