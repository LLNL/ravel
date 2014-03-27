#ifndef GNOME_H
#define GNOME_H

#include "rpartition.h"
#include "function.h"
#include "visoptions.h"
#include "partitioncluster.h"
#include "clusterprocess.h"
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <iostream>
#include <climits>
#include <cmath>

#include "kmedoids.h"

class Gnome
{
public:
    Gnome();
    ~Gnome();

    enum ChangeType { CLUSTER, SELECTION, NONE };

    virtual bool detectGnome(Partition * part);
    virtual Gnome * create();
    void setPartition(Partition * part) { partition = part; }
    void setFunctions(QMap<int, Function *> * _functions) { functions = _functions; }
    virtual void preprocess();
    virtual void drawGnomeQt(QPainter * painter, QRect extents, VisOptions * _options, int blockwidth);
    virtual void drawGnomeGL(QRect extents, VisOptions * _options) { Q_UNUSED(extents); options = _options; }
    virtual ChangeType handleDoubleClick(QMouseEvent * event);
    virtual void handleTreeDoubleClick(QMouseEvent * event);
    virtual void drawQtTree(QPainter * painter, QRect extents);
    virtual void setNeighbors(int _neighbors);
    PartitionCluster * getSelectedPartitionCluster() { return selected_pc; }
    void clearSelectedPartitionCluster() { selected_pc = NULL; }
    bool handleHover(QMouseEvent * event);
    void drawHover(QPainter * painter);
    void setSelected(bool selected) { is_selected = selected; }


    struct process_distance {
        double operator()(ClusterProcess * cp1, ClusterProcess * cp2) const {
            return cp1->calculateMetricDistance(*cp2);
        }
    };

protected:
    Partition * partition;
    QMap<int, Function *> * functions;
    VisOptions * options;
    int mousex;
    int mousey;

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

    long long int calculateMetricDistance(int p1, int p2);
    long long int calculateMetricDistance2(QList<Event *> * list1, QList<Event *> * list2);
    void findMusters();
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
    bool is_selected;

    QSet<Message *> saved_messages;
    QMap<PartitionCluster *, QRect> drawnPCs;
    QMap<PartitionCluster *, QRect> drawnNodes;
    QMap<Event *, QRect> drawnEvents;
    Event * hover_event;
    bool hover_aggregate;
    int stepwidth;

    void drawGnomeQtCluster(QPainter * painter, QRect extents, int blockwidth);
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
