#ifndef PARTITIONCLUSTER_H
#define PARTITIONCLUSTER_H

#include "clusterevent.h"
#include "event.h"
#include <QSet>
#include <QList>
#include <climits>

class PartitionCluster
{
public:
    PartitionCluster(int member, QList<Event *> *elist, QString metric, long long int divider = LLONG_MAX);
    PartitionCluster(long long int distance, PartitionCluster * c1, PartitionCluster * c2);
    ~PartitionCluster();
    PartitionCluster * get_root();
    PartitionCluster * get_closed_root();
    void delete_tree();
    int max_depth();
    int max_open_depth();
    bool leaf_open();
    int visible_clusters();
    QString memberString();
    void print(QString indent = "");
    void close();

    bool open;
    bool drawnOut;
    long long int max_distance;
    long long int max_metric;
    PartitionCluster * parent;
    QList<PartitionCluster *> * children;
    QList<int> * members;
    QList<ClusterEvent *> * events;
    QRect extents;
};

#endif // PARTITIONCLUSTER_H
