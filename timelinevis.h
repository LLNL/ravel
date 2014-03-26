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
    virtual void mouseDoubleClickEvent(QMouseEvent * event);
    void leaveEvent(QEvent * event);

public slots:
    virtual void selectEvent(Event * event, bool aggregate);
    void selectProcesses(QList<int> processes, Gnome *gnome);

protected:
    void drawHover(QPainter *painter);
    void drawProcessLabels(QPainter * painter, int effectiveHeight, float barHeight);

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

    static const int spacingMinimum = 12;

};
#endif // TIMELINEVIS_H
