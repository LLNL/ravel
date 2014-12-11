#ifndef TIMELINEVIS_H
#define TIMELINEVIS_H

#include "viswidget.h"

// Parent class for those who pan and zoom like a timeline view
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
    virtual void rightDrag(QMouseEvent * event) { Q_UNUSED(event); }

public slots:
    virtual void selectEvent(Event * event, bool aggregate, bool overdraw);
    void selectTasks(QList<int> tasks, Gnome *gnome);

protected:
    void drawHover(QPainter *painter);
    void drawTaskLabels(QPainter * painter, int effectiveHeight,
                           float barHeight);

    bool jumped;
    bool mousePressed;
    bool rightPressed;
    int mousex;
    int mousey;
    int pressx;
    int pressy;
    float stepwidth;
    float taskheight;
    int labelWidth;
    int labelHeight;
    int labelDescent;

    int maxStep;
    int startPartition;
    float startStep;
    float startTask; // refers to order rather than process really
    float stepSpan;
    float taskSpan;
    float lastStartStep;
    QMap<int, int> proc_to_order;
    QMap<int, int> order_to_proc;

    static const int spacingMinimum = 12;

};
#endif // TIMELINEVIS_H
