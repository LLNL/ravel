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
    QVector<long long int> * metric_events; // Representative vector of events for clustering

    ClusterProcess& operator+(const ClusterProcess &);
    ClusterProcess& operator/(const int);
    ClusterProcess& operator=(const ClusterProcess &);

    double calculateMetricDistance(const ClusterProcess& other) const;

};

#endif // CLUSTERPROCESS_H
