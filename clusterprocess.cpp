#include "clusterprocess.h"
#include <float.h>

ClusterProcess::ClusterProcess(int _p, int _step)
    : process(_p),
      startStep(_step),
      metric_events(new QVector<long long int>())
{
}

ClusterProcess::ClusterProcess()
    : process(0),
      startStep(0),
      metric_events(new QVector<long long int>())
{

}

ClusterProcess::ClusterProcess(const ClusterProcess & other)
    : process(other.process),
      startStep(other.startStep),
      metric_events(new QVector<long long int>())
{
    for (int i = 0; i < other.metric_events->size(); i++)
        metric_events->append(other.metric_events->at(i));
}

ClusterProcess::~ClusterProcess()
{
    delete metric_events;
}

double ClusterProcess::calculateMetricDistance(const ClusterProcess& other) const
{
    int num_matches = metric_events->size();
    double total_difference = 0;
    int offset = 0;
    if (metric_events->size() && other.metric_events->size())
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

ClusterProcess& ClusterProcess::operator+(const ClusterProcess & other)
{
    int offset = 0;
    if (metric_events->size() && other.metric_events->size())
    {
        if (startStep < other.startStep)
        {
            offset = metric_events->size() - other.metric_events->size();
            for (int i = 0; i < other.metric_events->size(); i++)
                (*metric_events)[offset + i] += other.metric_events->at(i);
        }
        else
        {
            offset = other.metric_events->size() - metric_events->size();
            for (int i = 0; i < metric_events->size(); i++)
                (*metric_events)[i] += other.metric_events->at(offset + i);
            for (int i = offset - 1; i >= 0; i--)
                metric_events->prepend(other.metric_events->at(i));
            startStep = other.startStep;
        }
    }
    else if (other.metric_events->size())
    {
        startStep = other.startStep;
        for (int i = 0; i < other.metric_events->size(); i++)
            metric_events->append(other.metric_events->at(i));
    }


    return *this;
}

ClusterProcess& ClusterProcess::operator/(const int divisor)
{
    for (int i = 0; i < metric_events->size(); i++)
    {
        (*metric_events)[i] /= divisor;
    }
    return *this;
}

ClusterProcess& ClusterProcess::operator=(const ClusterProcess & other)
{
    process = other.process;
    startStep = other.startStep;
    metric_events->clear();
    for (int i = 0; i < other.metric_events->size(); i++)
        metric_events->append(other.metric_events->at(i));
    return *this;
}
