#ifndef TIMELINEVIS_H
#define TIMELINEVIS_H

#include <QLinkedList>
#include "viswidget.h"
#include "colormap.h"
#include "event.h"

class TimelineVis : public VisWidget
{
    Q_OBJECT
public:
    TimelineVis(QWidget* parent = 0, VisOptions * _options = new VisOptions());
    ~TimelineVis();
    void processVis();

    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void leaveEvent(QEvent * event);

public slots:
    void selectEvent(Event * event);

protected:
    void drawHover(QPainter *painter);
    void drawProcessLabels(QPainter * painter, int effectiveHeight, int barHeight);

    bool jumped;
    bool mousePressed;
    int mousex;
    int mousey;
    int pressx;
    int pressy;
    float stepwidth;
    float processheight;
    int labelWidth;
    int labelHeight;

    int maxStep;
    int startPartition;
    float startStep;
    float startProcess; // refers to order rather than process really
    float stepSpan;
    float processSpan;
    float lastStartStep;
    QMap<int, int> proc_to_order;
    QMap<int, int> order_to_proc;

};
#endif // TIMELINEVIS_H
