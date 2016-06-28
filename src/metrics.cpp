#include "metrics.h"

Metrics::Metrics()
    : metrics(new QMap<QString, MetricPair *>())
{
}

Metrics::~Metrics()
{
    for (QMap<QString, MetricPair *>::Iterator itr = metrics->begin();
         itr != metrics->end(); ++itr)
    {
        delete itr.value();
    }
    delete metrics;

}

void Metrics::addMetric(QString name, double event_value)
{
    (*metrics)[name] = new MetricPair(event_value);
}

void Metrics::setMetric(QString name, double event_value)
{
    MetricPair * mp = metrics->value(name);
    mp->event = event_value;
}

bool Metrics::hasMetric(QString name)
{
    return metrics->contains(name);
}

double Metrics::getMetric(QString name)
{
    return ((*metrics)[name])->event;
}

QList<QString> Metrics::getMetricList()
{
    QList<QString> names = QList<QString>();
    for (QMap<QString, MetricPair *>::Iterator counter = metrics->begin();
         counter != metrics->end(); ++counter)
    {
        names.append(counter.key());
    }
    return names;
}
