#include "clusterevent.h"

ClusterEvent::ClusterEvent(int _step)
    : step(_step), waitallrecvs(0), isends(0)
{
    for (int i = COMM; i <= AGG; i++)
    {
        for (int j = SEND; j < ALL; j++)
        {
            for (int k = LOW; k < BOTH; k++)
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
    for (int i = COMM; i <= AGG; i++)
    {
        for (int j = SEND; j < ALL; j++)
        {
            for (int k = LOW; k < BOTH; k++)
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
    for (int i = COMM; i <= AGG; i++)
    {
        for (int j = SEND; j < ALL; j++)
        {
            for (int k = LOW; k < BOTH; k++)
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
    if (ctype == ALL && thresh == BOTH)
    {
        long long int value = 0;
        for (int i = SEND; i < ALL; i++)
            for (int j = LOW; j < BOTH; j++)
                value += metric[etype][i][j];
        return value;
    }
    else if (ctype == ALL)
    {
        long long int value = 0;
        for (int i = SEND; i < BOTH; i++)
            value += metric[etype][i][thresh];
        return value;
    }
    else if (thresh == BOTH)
    {
        long long int value = 0;
        for (int i = LOW; i < BOTH; i++)
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
    if (ctype == ALL && thresh == BOTH)
    {
        int count = 0;
        for (int i = SEND; i < ALL; i++)
            for (int j = LOW; j < BOTH; j++)
                count += counts[etype][i][j];
        return count;
    }
    else if (ctype == ALL)
    {
        int count = 0;
        for (int i = SEND; i < ALL; i++)
            count += counts[etype][i][thresh];
        return count;
    }
    else if (thresh == BOTH)
    {
        int count = 0;
        for (int i = LOW; i < BOTH; i++)
            count += counts[etype][ctype][i];
        return count;
    }
    else
        return counts[etype][ctype][thresh];
}
