#include "partitioncluster.h"

PartitionCluster::PartitionCluster()
    : max_distance(0),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QSet<int>())
{
}

PartitionCluster::PartitionCluster(long long int distance, PartitionCluster * c1, PartitionCluster * c2)
    : max_distance(distance),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QSet<int>())
{
    c1->parent = this;
    c2->parent = this;
    children->append(c1);
    children->append(c2);
    members->unite(*(c1->members));
    members->unite(*(c2->members));
}

PartitionCluster::~PartitionCluster()
{
    delete children;
    delete members;
}

void PartitionCluster::delete_tree()
{
    for (QList<PartitionCluster *>::Iterator child = children->begin(); child != children->end(); ++child)
    {
        (*child)->delete_tree();
        delete *child;
    }
}

PartitionCluster * PartitionCluster::get_root()
{
    if (parent)
        return parent->get_root();

    return this;
}
