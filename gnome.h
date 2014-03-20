#ifndef GNOME_H
#define GNOME_H

#include "partition.h"
#include "visoptions.h"
#include "partitioncluster.h"
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <iostream>
#include <climits>
#include <cmath>

class Gnome
{
public:
    Gnome();
    ~Gnome();

    enum ChangeType { CLUSTER, SELECTION, NONE };

    virtual bool detectGnome(Partition * part);
    virtual Gnome * create();
    void setPartition(Partition * part) { partition = part; }
    virtual void preprocess();
    virtual void drawGnomeQt(QPainter * painter, QRect extents, VisOptions * _options);
    virtual void drawGnomeGL(QRect extents, VisOptions * _options) { Q_UNUSED(extents); options = _options; }
    virtual ChangeType handleDoubleClick(QMouseEvent * event);
    virtual void handleTreeDoubleClick(QMouseEvent * event);
    virtual void drawQtTree(QPainter * painter, QRect extents);
    virtual void setNeighbors(int _neighbors);
    PartitionCluster * getSelectedPartitionCluster() { return selected_pc; }
    void clearSelectedPartitionCluster() { selected_pc = NULL; }

protected:
    Partition * partition;
    VisOptions * options;

    QString metric;
    class DistancePair {
    public:
        DistancePair(long long _d, int _p1, int _p2)
            : distance(_d), p1(_p1), p2(_p2) {}
        bool operator<(const DistancePair &dp) const { return distance < dp.distance; }

        long long distance;
        int p1;
        int p2;
    };
    class CentroidDistance {
    public:
        CentroidDistance(long long int _d, int _p)
            : distance(_d), process(_p) {}
        bool operator<(const CentroidDistance &cd) const { return distance < cd.distance; }

        long long int distance;
        int process;
    };
    class AverageMetric {
    public:
        AverageMetric(long long int _m, int _s)
            : metric(_m), step(_s) {}

        long long int metric;
        int step;
    };

    long long int calculateMetricDistance(QList<Event *> * list1, QList<Event *> * list2);
    void findClusters();
    virtual void generateTopProcesses(PartitionCluster * pc = NULL);
    void generateTopProcessesWorker(int process);
    int findCentroidProcess(PartitionCluster * pc);
    int findMaxMetricProcess(PartitionCluster * pc);


    class DrawMessage {
    public:
        DrawMessage(QPoint _send, QPoint _recv, int _nsends)
            : send(_send), recv(_recv), nsends(_nsends), nrecvs(0) { }

        QPoint send;
        QPoint recv;
        int nsends;
        int nrecvs;
    };

    QMap<int, PartitionCluster * > * cluster_leaves;
    PartitionCluster * cluster_root;
    int max_metric_process;
    QList<int> top_processes;
    bool alternation;
    int neighbors;
    PartitionCluster * selected_pc;

    QSet<Message *> saved_messages;
    QMap<PartitionCluster *, QRect> drawnPCs;
    QMap<PartitionCluster *, QRect> drawnNodes;

    void drawGnomeQtCluster(QPainter * painter, QRect extents);
    void drawGnomeQtTopProcesses(QPainter * painter, QRect extents,
                                 int blockwidth, int barwidth);
    void drawGnomeQtClusterBranch(QPainter * painter, QRect current, PartitionCluster * pc,
                                  float blockheight, int blockwidth, int barheight, int barwidth);
    void drawGnomeQtClusterLeaf(QPainter * painter, QRect startxy, QList<Event *> * elist,
                                int blockwidth, int startStep); // Doesn't use blockheight
    void drawGnomeQtInterMessages(QPainter * painter, int blockwidth, int startStep, int startx); // Doesn't use blockheight
    virtual void drawGnomeQtClusterEnd(QPainter * painter, QRect clusterRect, PartitionCluster * pc,
                                    int barwidth, int barheight, int blockwidth, int blockheight,
                                    int startStep); // At this point, blockheight will just get recalculated anyway probably
    void drawTreeBranch(QPainter * painter, QRect current, PartitionCluster * pc,
                        int branch_length, int labelwidth, float blockheight, int leafx);
    int getTopHeight(QRect extents);

    static const int clusterMaxHeight = 76;
};

#endif // GNOME_H
