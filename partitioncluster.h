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

    long long int max_distance;
    PartitionCluster * parent;
    QList<PartitionCluster *> * children;
    QSet<int> * members;
};

#endif // PARTITIONCLUSTER_H
