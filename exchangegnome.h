#ifndef EXCHANGEGNOME_H
#define EXCHANGEGNOME_H

#include <QMap>
#include <QList>
#include "gnome.h"
#include "partitioncluster.h"

class ExchangeGnome : public Gnome
{
public:
    ExchangeGnome();
    ~ExchangeGnome();

    bool detectGnome(Partition * part);
    Gnome * create();
    void preprocess();
    void drawGnomeQt(QPainter * painter, QRect extents, VisOptions *_options);
    void drawGnomeGL(QRect extents, VisOptions * _options);

private:
    // send-receive, sends-receives, sends-waitall
    enum ExchangeType { SRSR, SSRR, SSWA, UNKNOWN };
    ExchangeType type;
    QString metric;
    ExchangeType findType();

    class DistancePair {
    public:
        DistancePair(long long _d, int _p1, int _p2)
            : distance(_d), p1(_p1), p2(_p2) {}
        bool operator<(const DistancePair &dp) const { return distance < dp.distance; }

        long long distance;
        int p1;
        int p2;
    };
    long long int calculateMetricDistance(QList<Event *> * list1, QList<Event *> * list2);
    void findClusters();

    QMap<int, PartitionCluster * > * cluster_leaves;
    PartitionCluster * cluster_root;

    void drawGnomeQtCluster(QPainter * painter, QRect extents);
    void drawGnomeQtClusterBranch(QPainter * painter, QRect current, PartitionCluster * pc, int leafx,
                                                 int blockheight, int blockwidth, int barheight, int barwidth);
    void drawGnomeQtClusterLeaf(QPainter * painter, QRect startxy, QList<Event *> * elist, int blockwidth, int startStep);
};

#endif // EXCHANGEGNOME_H
