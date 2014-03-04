#ifndef PARTITIONCLUSTER_H
#define PARTITIONCLUSTER_H

#include <QSet>
#include <QList>

class PartitionCluster
{
public:
    PartitionCluster();
    PartitionCluster(long long int distance, PartitionCluster * c1, PartitionCluster * c2);
    ~PartitionCluster();
    PartitionCluster * get_root();
    void delete_tree();
    int max_depth();
    QString memberString();
    void print(QString indent = "");

    long long int max_distance;
    PartitionCluster * parent;
    QList<PartitionCluster *> * children;
    QList<int> * members;
};

#endif // PARTITIONCLUSTER_H
