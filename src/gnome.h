#ifndef GNOME_H
#define GNOME_H

#include "commdrawinterface.h"
#include "rpartition.h"
#include "function.h"
#include "visoptions.h"
#include "partitioncluster.h"
#include "clusterentity.h"
#include <QPainter>
#include <QRect>

class Event;
class QMouseEvent;
class ClusterEntity;

// Our unit of clustering and drawing. The idea is the inheritance hierarchy
// will allow customization. However, this design with both a detector and
// a drawing style needs to be factored in some other way. The design right
// now is suspect.
class Gnome : CommDrawInterface
{
public:
    Gnome();
    ~Gnome();

    enum ChangeType { CHANGE_CLUSTER, CHANGE_SELECTION, CHANGE_NONE };

    virtual bool detectGnome(Partition * part);
    virtual Gnome * create();
    void set_seed(unsigned long s) { seed = s; }
    virtual void preprocess();
    void setPartition(Partition * part) { partition = part; }
    void setFunctions(QMap<int, Function *> * _functions)
        { functions = _functions; }

    // Tree GUI
    virtual void drawQtTree(QPainter * painter, QRect extents);
    virtual void setNeighbors(int _neighbors);
    void drawTopLabels(QPainter * painter, QRect extents);
    virtual void handleTreeDoubleClick(QMouseEvent * event);

    // Main GUI
    virtual void drawGnomeQt(QPainter * painter, QRect extents,
                             VisOptions * _options, int blockwidth);
    virtual void drawGnomeGL(QRect extents, VisOptions * _options)
        { Q_UNUSED(extents); options = _options; }
    virtual ChangeType handleDoubleClick(QMouseEvent * event);
    PartitionCluster * getSelectedPartitionCluster() { return selected_pc; }
    void setSelected(bool selected) { is_selected = selected; }
    void clearSelectedPartitionCluster() { selected_pc = NULL; }
    bool handleHover(QMouseEvent * event);
    void drawHover(QPainter * painter);

    virtual void drawMessage(QPainter * painter, Message * message) { Q_UNUSED(painter); Q_UNUSED(message); }
    virtual void drawCollective(QPainter * painter, CollectiveRecord * cr) { Q_UNUSED(painter); Q_UNUSED(cr); }
    virtual void drawDelayTracking(QPainter * painter, CommEvent * c)
        { Q_UNUSED(painter); Q_UNUSED(c); }

    // For clusterings
    struct entity_distance {
        double operator()(ClusterEntity * cp1, ClusterEntity * cp2) const {
            return cp1->calculateMetricDistance(*cp2);
        }
    };

    struct entity_distance_np {
        double operator()(const ClusterEntity& cp1,
                          const ClusterEntity& cp2) const {
            return cp1.calculateMetricDistance(cp2);
        }
    };

protected:
    Partition * partition;
    QMap<int, Function *> * functions;
    VisOptions * options;
    unsigned long seed;
    int mousex;
    int mousey;

    QString metric;
    class DistancePair {
    public:
        DistancePair(long long _d, int _p1, int _p2)
            : distance(_d), p1(_p1), p2(_p2) {}
        bool operator<(const DistancePair &dp) const
            { return distance < dp.distance; }

        long long distance;
        int p1;
        int p2;
    };
    class CentroidDistance {
    public:
        CentroidDistance(long long int _d, int _p)
            : distance(_d), entity(_p) {}
        bool operator<(const CentroidDistance &cd) const
            { return distance < cd.distance; }

        long long int distance;
        int entity;
    };
    class AverageMetric {
    public:
        AverageMetric(long long int _m, int _s)
            : metric(_m), step(_s) {}

        long long int metric;
        int step;
    };

    long long int calculateMetricDistance(int p1, int p2);
    long long int calculateMetricDistance2(QList<CommEvent *> * list1,
                                           QList<CommEvent *> * list2);
    void findMusters();
    void findClusters();
    void hierarchicalMusters();
    virtual void generateTopEntities(PartitionCluster * pc = NULL);
    void generateTopEntitiesWorker(int entity);
    int findCentroidEntity(PartitionCluster * pc);
    int findMaxMetricEntity(PartitionCluster * pc);


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
    QMap<int, PartitionCluster * > * cluster_map;
    PartitionCluster * cluster_root;
    int max_metric_entity;
    QList<int> top_entities; // focus entities really
    bool alternation; // cluster background
    int neighbors; // neighbor radius

    QSet<Message *> saved_messages;
    QMap<PartitionCluster *, QRect> drawnPCs;
    QMap<PartitionCluster *, QRect> drawnNodes;
    QMap<Event *, QRect> drawnEvents;
    PartitionCluster * selected_pc;
    bool is_selected;
    Event * hover_event;
    bool hover_aggregate;
    int stepwidth;

    void drawGnomeQtCluster(QPainter * painter, QRect extents, int blockwidth);
    void drawGnomeQtTopEntities(QPainter * painter, QRect extents,
                                 int blockwidth, int barwidth);
    void drawGnomeQtClusterBranch(QPainter * painter, QRect current,
                                  PartitionCluster * pc,
                                  float blockheight, int blockwidth, int barheight,
                                  int barwidth);
    void drawGnomeQtClusterLeaf(QPainter * painter, QRect startxy,
                                QList<CommEvent *> *elist,
                                int blockwidth, int startStep);
    void drawGnomeQtInterMessages(QPainter * painter, int blockwidth,
                                  int startStep, int startx);
    virtual void drawGnomeQtClusterEnd(QPainter * painter, QRect clusterRect,
                                       PartitionCluster * pc,
                                       int barwidth, int barheight,
                                       int blockwidth, int blockheight,
                                       int startStep); // blockheight not used
    void drawTreeBranch(QPainter * painter, QRect current,
                        PartitionCluster * pc,
                        int branch_length, int labelwidth, float blockheight,
                        int leafx);
    int getTopHeight(QRect extents);

    static const int clusterMaxHeight = 76;
};

#endif // GNOME_H
