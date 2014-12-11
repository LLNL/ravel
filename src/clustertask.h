#ifndef CLUSTERTASK_H
#define CLUSTERTASK_H

#include <QVector>

class ClusterTask
{
public:
    ClusterTask();
    ClusterTask(int _t, int _step);
    ClusterTask(const ClusterTask& other);
    ~ClusterTask();

    int task;
    int startStep; // What step our metric_events starts at

    // Representative vector of events for clustering. This is contiguous
    // so all missing steps should be filled in with their previous lateness
    // value by whoever builds this ClusterTask
    QVector<long long int> * metric_events;

    ClusterTask& operator+(const ClusterTask &);
    ClusterTask& operator/(const int);
    ClusterTask& operator=(const ClusterTask &);

    double calculateMetricDistance(const ClusterTask& other) const;

};

#endif // CLUSTERTASK_H
