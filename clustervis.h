#ifndef CLUSTERVIS_H
#define CLUSTERVIS_H

#include "timelinevis.h"
#include "clustertreevis.h"

class ClusterVis : public TimelineVis
{
    Q_OBJECT
public:
    ClusterVis(ClusterTreeVis * ctv, QWidget* parent = 0, VisOptions *_options = new VisOptions());
    ~ClusterVis() {}
    void setTrace(Trace * t);

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);

signals:
    void focusGnome(Gnome * gnome);
    void clusterChange();

public slots:
    void setSteps(float start, float stop, bool jump = false);
    void clusterChanged();
    void changeNeighborRadius(int neighbors);
    void selectEvent(Event * event);

protected:
    void qtPaint(QPainter *painter);
    void drawNativeGL();
    void paintEvents(QPainter *painter);
    void prepaint();
    //void drawLine(QPainter * painter, QPointF * p1, QPointF * p2, int effectiveHeight);
    //void setupMetric();
    void mouseDoubleClickEvent(QMouseEvent * event);

    QMap<Gnome *, QRect> drawnGnomes;
    PartitionCluster * selected;
    ClusterTreeVis * treevis;

};

#endif // CLUSTERVIS_H
