#ifndef CLUSTEREVENT_H
#define CLUSTEREVENT_H

class ClusterEvent
{
public:
    ClusterEvent(int _step, long long int _low, long long int _high,
                 int _nlow, int _nhigh, long long int _alow,
                 long long int _ahigh, int _nagglow, int _nagghigh, int _nsend, int _nrecv);

    ClusterEvent(const ClusterEvent& copy);

    int step;
    // On the way up, use metrics as total, then once the root, go back down and average them
    long long int low_metric;
    long long int high_metric;
    int num_low;
    int num_high;
    long long int agg_low;
    long long int agg_high;
    int num_agg_low;
    int num_agg_high;
    int num_send;
    int num_recv;
};

#endif // CLUSTEREVENT_H
