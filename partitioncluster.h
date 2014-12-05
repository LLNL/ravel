#ifndef PARTITIONCLUSTER_H
#define PARTITIONCLUSTER_H

#include <QRect>
#include <QString>
#include <QSet>
#include <QList>

class CommEvent;
class ClusterTask;
class PartitionCluster;
class ClusterEvent;

// Cluster node of a hierarchical clustering, contains a subset of a partition
class PartitionCluster
{
public:
    PartitionCluster(int num_steps, int start, long long _divider = LLONG_MAX);
    PartitionCluster(int member, QList<CommEvent *> *elist, QString metric,
                     long long int _divider = LLONG_MAX);
    PartitionCluster(long long int distance, PartitionCluster * c1,
                     PartitionCluster * c2);
    ~PartitionCluster();
    long long int addMember(ClusterTask * cp, QList<CommEvent *> *elist,
                            QString metric);
    long long int distance(PartitionCluster * other);
    void makeClusterVectors();

    // Call at root before deconstructing
    void delete_tree();

    // Tree info, also used for vis
    PartitionCluster * get_root();
    PartitionCluster * get_closed_root();
    int max_depth();
    int max_open_depth();
    bool leaf_open();
    int visible_clusters();
    void close();

    // For debug
    QString memberString();
    void print(QString indent = "");



    int startStep;
    int max_task;
    bool open; // Whether children are drawn
    bool drawnOut;
    long long int max_distance; // max within cluster distance
    long long int max_metric;
    long long int divider; // For threshholding (currently unused)
    PartitionCluster * parent;
    QList<PartitionCluster *> * children;
    QList<int> * members; // Tasks in the cluster
    QList<ClusterEvent *> * events; // Represented events
    QRect extents; // Where it was drawn

    // Vector that has previous 'lateness' filling in steps without events
    QVector<long long int> * cluster_vector;
    int clusterStart; // Starting step of the cluster_vector
};

#endif // PARTITIONCLUSTER_H
