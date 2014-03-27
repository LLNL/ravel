#ifndef CLUSTERPROCESS_H
#define CLUSTERPROCESS_H

#include <QVector>

class ClusterProcess
{
public:
    ClusterProcess(int _p, int _step);
    ~ClusterProcess();

    int process;
    int startStep;
    QVector<long long int> * metric_events;

    double calculateMetricDistance(const ClusterProcess& other);

};

#endif // CLUSTERPROCESS_H
