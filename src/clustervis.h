#ifndef CLUSTERVIS_H
#define CLUSTERVIS_H

#include "timelinevis.h"

class ClusterTreeVis;
class PartitionCluster;

class ClusterVis : public TimelineVis
{
    Q_OBJECT
public:
    ClusterVis(ClusterTreeVis * ctv, QWidget* parent = 0,
               VisOptions *_options = new VisOptions());
    ~ClusterVis() {}
    void setTrace(Trace * t);

    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);

signals:
    void focusGnome(); // used by clustertreevis
    void clusterChange(); // for cluster selection
    void neighborChange(int); // change neighborhood radius for top processes

public slots:
    void setSteps(float start, float stop, bool jump = false);
    void clusterChanged(); // for cluster selection
    void changeNeighborRadius(int neighbors); // neighborhood radius of top processes
    void selectEvent(Event * event, bool aggregate, bool overdraw);

protected:
    void qtPaint(QPainter *painter);
    void drawNativeGL();
    void paintEvents(QPainter *painter);
    void prepaint();
    void mouseDoubleClickEvent(QMouseEvent * event);

    QMap<Gnome *, QRect> drawnGnomes;
    PartitionCluster * selected;
    ClusterTreeVis * treevis;
    Gnome * hover_gnome;

};

#endif // CLUSTERVIS_H
