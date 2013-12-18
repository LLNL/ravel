#ifndef TIMELINEVIS_H
#define TIMELINEVIS_H


#include "viswidget.h"
#include "colormap.h"
#include "event.h"

class TimelineVis : public VisWidget
{
    Q_OBJECT
public:
    TimelineVis(QWidget* parent = 0);
    ~TimelineVis();
    void processVis();

    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void leaveEvent(QEvent * event);
    void setColorMap(ColorMap * cm);

public slots:
    void selectEvent(Event * event);

protected:
    void drawHover(QPainter *painter);

    bool mousePressed;
    int mousex;
    int mousey;
    int pressx;
    int pressy;
    int stepwidth;
    int processheight;

    int maxStep;
    float startStep;
    float startProcess; // refers to order rather than process really
    float stepSpan;
    float processSpan;
    QMap<int, int> proc_to_order;
    QMap<int, int> order_to_proc;
    ColorMap * colormap;

};
#endif // TIMELINEVIS_H
