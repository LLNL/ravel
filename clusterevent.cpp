#include "clusterevent.h"

ClusterEvent::ClusterEvent(int _step, long long int _low, long long int _high,
                           int _nlow, int _nhigh, long long int _alow,
                           long long int _ahigh, int _nagglow, int _nagghigh,
                           int _nsend, int _nrecv)
    : step(_step),
      low_metric(_low),
      high_metric(_high),
      num_low(_nlow),
      num_high(_nhigh),
      agg_low(_alow),
      agg_high(_ahigh),
      num_agg_low(_nagglow),
      num_agg_high(_nagghigh),
      num_send(_nsend),
      num_recv(_nrecv)
{
}

ClusterEvent::ClusterEvent(const ClusterEvent& copy)
{
    step = copy.step;
    low_metric = copy.low_metric;
    high_metric = copy.high_metric;
    num_low = copy.num_low;
    num_high = copy.num_high;
    agg_low = copy.agg_low;
    agg_high = copy.agg_high;
    num_agg_low = copy.num_agg_low;
    num_agg_high = copy.num_agg_high;
    num_send = copy.num_send;
    num_recv = copy.num_recv;
}
