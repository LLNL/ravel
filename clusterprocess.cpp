#include "clusterprocess.h"
#include <float.h>

ClusterProcess::ClusterProcess(int _p, int _step)
    : process(_p),
      startStep(_step),
      metric_events(new QVector<long long int>())
{
}

ClusterProcess::~ClusterProcess()
{
    delete metric_events;
}

double ClusterProcess::calculateMetricDistance(const ClusterProcess& other)
{
    int num_matches = metric_events->size();
    double total_difference = 0;
    int offset = 0;
    if (startStep < other.startStep)
    {
        num_matches = other.metric_events->size();
        offset = metric_events->size() - other.metric_events->size();
        for (int i = 0; i < other.metric_events->size(); i++)
            total_difference += (metric_events->at(offset + i) - other.metric_events->at(i)) * (metric_events->at(offset + i) - other.metric_events->at(i));
    }
    else
    {
        offset = other.metric_events->size() - metric_events->size();
        for (int i = 0; i < metric_events->size(); i++)
            total_difference += (other.metric_events->at(offset + i) - metric_events->at(i)) * (other.metric_events->at(offset + i) - metric_events->at(i));
    }
    if (num_matches <= 0)
        return DBL_MAX;
    return total_difference / num_matches;
}
