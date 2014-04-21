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
    int startStep;
    QVector<long long int> * metric_events;

    ClusterProcess& operator+(const ClusterProcess &);
    ClusterProcess& operator/(const int);
    ClusterProcess& operator=(const ClusterProcess &);

    double calculateMetricDistance(const ClusterProcess& other) const;

};

#endif // CLUSTERPROCESS_H
