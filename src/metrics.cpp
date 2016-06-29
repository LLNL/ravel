#include "metrics.h"

Metrics::Metrics()
    : metrics(new QMap<QString, double>())
{
}

Metrics::~Metrics()
{
    delete metrics;
}

void Metrics::addMetric(QString name, double event_value)
{
    (*metrics)[name] = event_value;
}

void Metrics::setMetric(QString name, double event_value)
{
    (*metrics)[name] = event_value;
}

bool Metrics::hasMetric(QString name)
{
    return metrics->contains(name);
}

double Metrics::getMetric(QString name)
{
    return ((*metrics)[name]);
}

QList<QString> Metrics::getMetricList()
{
    QList<QString> names = QList<QString>();
    for (QMap<QString, double>::Iterator counter = metrics->begin();
         counter != metrics->end(); ++counter)
    {
        names.append(counter.key());
    }
    return names;
}
