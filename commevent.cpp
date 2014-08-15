#include "commevent.h"

CommEvent::CommEvent(unsigned long long _enter, unsigned long long _exit,
                     int _function, int _process, int _phase)
    : Event(_enter, _exit, _function, _process),
      metrics(new QMap<QString, MetricPair *>()),
      partition(NULL),
      comm_next(NULL),
      comm_prev(NULL),
      last_send(NULL),
      next_send(NULL),
      last_recvs(NULL),
      last_step(-1),
      stride_parents(new QSet<CommEvent *>()),
      stride_children(new QSet<CommEvent *>()),
      stride(-1),
      step(-1),
      phase(_phase)
{
}

CommEvent::~CommEvent()
{
    for (QMap<QString, MetricPair *>::Iterator itr = metrics->begin();
         itr != metrics->end(); ++itr)
    {
        delete itr.value();
    }
    delete metrics;

    if (last_recvs)
        delete last_recvs;
    if (stride_children)
        delete stride_children;
    if (stride_parents)
        delete stride_parents;
}



void CommEvent::addMetric(QString name, double event_value,
                      double aggregate_value)
{
    (*metrics)[name] = new MetricPair(event_value, aggregate_value);
}

bool CommEvent::hasMetric(QString name)
{
    return metrics->contains(name);
}

double CommEvent::getMetric(QString name, bool aggregate)
{
    if (aggregate)
        return ((*metrics)[name])->aggregate;

    return ((*metrics)[name])->event;
}

void CommEvent::calculate_differential_metric(QString metric_name,
                                              QString base_name)
{
    long long max_parent = getMetric(base_name, true);
    long long max_agg_parent = 0;
    if (comm_prev)
        max_agg_parent = (comm_prev->getMetric(base_name));

    addMetric(metric_name,
              std::max(0.,
                       getMetric(base_name)- max_parent),
              std::max(0.,
                       getMetric(base_name, true)- max_agg_parent));
}
