#ifndef CLUSTERENTITY_H
#define CLUSTERENTITY_H

#include <QVector>

class ClusterEntity
{
public:
    ClusterEntity();
    ClusterEntity(long _e, int _step);
    ClusterEntity(const ClusterEntity& other);
    ~ClusterEntity();

    long entity;
    int startStep; // What step our metric_events starts at

    // Representative vector of events for clustering. This is contiguous
    // so all missing steps should be filled in with their previous lateness
    // value by whoever builds this ClusterEntity
    QVector<long long int> * metric_events;

    ClusterEntity& operator+(const ClusterEntity &);
    ClusterEntity& operator/(const int);
    ClusterEntity& operator=(const ClusterEntity &);

    double calculateMetricDistance(const ClusterEntity& other) const;

};

#endif // CLUSTERENTITY_H
