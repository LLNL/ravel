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
    void delete_tree();
    int max_depth();
    QString memberString();
    void print(QString indent = "");

    bool open;
    long long int max_distance;
    PartitionCluster * parent;
    QList<PartitionCluster *> * children;
    QList<int> * members;
    QList<ClusterEvent *> * events;
};

#endif // PARTITIONCLUSTER_H
