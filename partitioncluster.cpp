#include "partitioncluster.h"
#include <iostream>

PartitionCluster::PartitionCluster()
    : max_distance(0),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QList<int>())
{
}

PartitionCluster::PartitionCluster(long long int distance, PartitionCluster * c1, PartitionCluster * c2)
    : max_distance(distance),
      parent(NULL),
      children(new QList<PartitionCluster *>()),
      members(new QList<int>())
{
    c1->parent = this;
    c2->parent = this;
    children->append(c1);
    children->append(c2);
    members->append(*(c1->members));
    members->append(*(c2->members));
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

int PartitionCluster::max_depth()
{
    int max = 0;
    for (QList<PartitionCluster *>::Iterator child = children->begin(); child != children->end(); ++child)
        max = std::max(max, (*child)->max_depth() + 1);
    return max;
}

QString PartitionCluster::memberString()
{
    QString ms = "[ ";
    if (!members->isEmpty())
    {
        ms += QString::number(members->at(0));
        for (int i = 1; i < members->size(); i++)
            ms += ", " + QString::number(members->at(i));
    }
    ms += " ]";
    return ms;
}

void PartitionCluster::print(QString indent)
{
    std::cout << indent.toStdString().c_str() << memberString().toStdString().c_str() << std::endl;
    QString myindent = indent + "   ";
    for (QList<PartitionCluster *>::Iterator child = children->begin(); child != children->end(); ++child)
        (*child)->print(myindent);
}
