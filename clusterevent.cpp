#include "clusterevent.h"

ClusterEvent::ClusterEvent(int _step)
    : step(_step), waitallrecvs(0), isends(0)
{
    for (int i = CE_EVENT_COMM; i <= CE_EVENT_AGG; i++)
    {
        for (int j = CE_COMM_SEND; j < CE_COMM_ALL; j++)
        {
            for (int k = CE_THRESH_LOW; k < CE_THRESH_BOTH; k++)
            {
                metric[i][j][k] = 0;
                counts[i][j][k] = 0;
            }
        }
    }
}

ClusterEvent::ClusterEvent(const ClusterEvent& copy)
{
    step = copy.step;
    waitallrecvs = copy.waitallrecvs;
    isends = copy.isends;
    for (int i = CE_EVENT_COMM; i <= CE_EVENT_AGG; i++)
    {
        for (int j = CE_COMM_SEND; j < CE_COMM_ALL; j++)
        {
            for (int k = CE_THRESH_LOW; k < CE_THRESH_BOTH; k++)
            {
                metric[i][j][k] = copy.metric[i][j][k];
                counts[i][j][k] = copy.counts[i][j][k];
            }
        }
    }
}

ClusterEvent::ClusterEvent(int _step, const ClusterEvent *copy1,
                           const ClusterEvent *copy2)
    : step(_step)
{
    waitallrecvs = copy1->waitallrecvs + copy2->waitallrecvs;
    isends = copy1->isends + copy2->isends;
    for (int i = CE_EVENT_COMM; i <= CE_EVENT_AGG; i++)
    {
        for (int j = CE_COMM_SEND; j < CE_COMM_ALL; j++)
        {
            for (int k = CE_THRESH_LOW; k < CE_THRESH_BOTH; k++)
            {
                metric[i][j][k] = copy1->metric[i][j][k] + copy2->metric[i][j][k];
                counts[i][j][k] = copy1->counts[i][j][k] + copy2->counts[i][j][k];
            }
        }
    }
}

void ClusterEvent::setMetric(int count, long long value,
                             EventType etype,
                             CommType ctype,
                             Threshhold thresh)
{
    metric[etype][ctype][thresh] = value;
    counts[etype][ctype][thresh] = count;
}

void ClusterEvent::addMetric(int count, long long value,
                             EventType etype,
                             CommType ctype,
                             Threshhold thresh)
{
    metric[etype][ctype][thresh] += value;
    counts[etype][ctype][thresh] += count;
}

long long int ClusterEvent::getMetric(EventType etype,
                                      CommType ctype,
                                      Threshhold thresh)
{
    if (ctype == CE_COMM_ALL && thresh == CE_THRESH_BOTH)
    {
        long long int value = 0;
        for (int i = CE_COMM_SEND; i < CE_COMM_ALL; i++)
            for (int j = CE_THRESH_LOW; j < CE_THRESH_BOTH; j++)
                value += metric[etype][i][j];
        return value;
    }
    else if (ctype == CE_COMM_ALL)
    {
        long long int value = 0;
        for (int i = CE_COMM_SEND; i < CE_COMM_ALL; i++)
            value += metric[etype][i][thresh];
        return value;
    }
    else if (thresh == CE_THRESH_BOTH)
    {
        long long int value = 0;
        for (int i = CE_THRESH_LOW; i < CE_THRESH_BOTH; i++)
            value += metric[etype][ctype][i];
        return value;
    }
    else
        return metric[etype][ctype][thresh];
}

int ClusterEvent::getCount(EventType etype,
                           CommType ctype,
                           Threshhold thresh)
{
    if (ctype == CE_COMM_ALL && thresh == CE_THRESH_BOTH)
    {
        int count = 0;
        for (int i = CE_COMM_SEND; i < CE_COMM_ALL; i++)
            for (int j = CE_THRESH_LOW; j < CE_THRESH_BOTH; j++)
                count += counts[etype][i][j];
        return count;
    }
    else if (ctype == CE_COMM_ALL)
    {
        int count = 0;
        for (int i = CE_COMM_SEND; i < CE_COMM_ALL; i++)
            count += counts[etype][i][thresh];
        return count;
    }
    else if (thresh == CE_THRESH_BOTH)
    {
        int count = 0;
        for (int i = CE_THRESH_LOW; i < CE_THRESH_BOTH; i++)
            count += counts[etype][ctype][i];
        return count;
    }
    else
        return counts[etype][ctype][thresh];
}
