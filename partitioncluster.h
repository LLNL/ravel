#ifndef PARTITIONCLUSTER_H
#define PARTITIONCLUSTER_H

#include "clusterevent.h"
#include "clusterprocess.h"
#include "event.h"
#include <QSet>
#include <QList>
#include <climits>

class PartitionCluster
{
public:
    PartitionCluster(int num_steps, int start, long long _divider = LLONG_MAX);
    PartitionCluster(int member, QList<Event *> *elist, QString metric, long long int _divider = LLONG_MAX);
    PartitionCluster(long long int distance, PartitionCluster * c1, PartitionCluster * c2);
    ~PartitionCluster();
    long long int addMember(ClusterProcess * cp, QList<Event *> * elist, QString metric);
    long long int distance(PartitionCluster * other);
    void makeClusterVectors();
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

    int startStep;
    int max_process;
    bool open;
    bool drawnOut;
    long long int max_distance;
    long long int max_metric;
    long long int divider;
    PartitionCluster * parent;
    QList<PartitionCluster *> * children;
    QList<int> * members;
    QList<ClusterEvent *> * events;
    QRect extents;
    QVector<long long int> * cluster_vector;
    int clusterStart;
};

#endif // PARTITIONCLUSTER_H
