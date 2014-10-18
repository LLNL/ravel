#ifndef CLUSTERPROCESS_H
#define CLUSTERPROCESS_H

#include <QVector>

class ClusterProcess
{
public:
    ClusterProcess();
    ClusterProcess(int _p, int _step);
    ClusterProcess(const ClusterProcess& other);
    ~ClusterProcess();

    int process;
    int startStep; // What step our metric_events starts at

    // Representative vector of events for clustering. This is contiguous
    // so all missing steps should be filled in with their previous lateness
    // value by whoever builds this ClusterProcess
    QVector<long long int> * metric_events;

    ClusterProcess& operator+(const ClusterProcess &);
    ClusterProcess& operator/(const int);
    ClusterProcess& operator=(const ClusterProcess &);

    double calculateMetricDistance(const ClusterProcess& other) const;

};

#endif // CLUSTERPROCESS_H
