#include "clusterevent.h"

ClusterEvent::ClusterEvent(int _step)
    : step(_step)
{
    for (int i = COMM; i <= AGG; i++)
    {
        for (int j = SEND; j < BOTH; j++)
        {
            for (int k = LOW; k < ALL; k++)
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
    for (int i = COMM; i <= AGG; i++)
    {
        for (int j = SEND; j < BOTH; j++)
        {
            for (int k = LOW; k < ALL; k++)
            {
                metric[i][j][k] = copy.metric[i][j][k];
                counts[i][j][k] = copy.counts[i][j][k];
            }
        }
    }
}

ClusterEvent::ClusterEvent(int _step, const ClusterEvent *copy1, const ClusterEvent *copy2)
    : step(_step)
{
    for (int i = COMM; i <= AGG; i++)
    {
        for (int j = SEND; j < BOTH; j++)
        {
            for (int k = LOW; k < ALL; k++)
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

long long int ClusterEvent::getMetric(EventType etype,
                        CommType ctype,
                        Threshhold thresh)
{
    if (ctype == BOTH && thresh == ALL)
    {
        long long int value = 0;
        for (int i = SEND; i < NEITHER; i++)
            for (int j = LOW; j < ALL; j++)
                value += metric[etype][i][j];
        return value;
    }
    else if (ctype == BOTH)
    {
        long long int value = 0;
        for (int i = SEND; i < NEITHER; i++)
            value += metric[etype][i][thresh];
        return value;
    }
    else if (thresh == ALL)
    {
        long long int value = 0;
        for (int i = LOW; i < ALL; i++)
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
    if (ctype == BOTH && thresh == ALL)
    {
        int count = 0;
        for (int i = SEND; i < NEITHER; i++)
            for (int j = LOW; j < ALL; j++)
                count += counts[etype][i][j];
        return count;
    }
    else if (ctype == BOTH)
    {
        int count = 0;
        for (int i = SEND; i < NEITHER; i++)
            count += counts[etype][i][thresh];
        return count;
    }
    else if (thresh == ALL)
    {
        int count = 0;
        for (int i = LOW; i < ALL; i++)
            count += counts[etype][ctype][i];
        return count;
    }
    else
        return counts[etype][ctype][thresh];
}
