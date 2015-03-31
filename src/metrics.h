#ifndef METRICS_H
#define METRICS_H

#include <QMap>
#include <QString>
#include <QList>

class Metrics
{
public:
    Metrics();
    ~Metrics();
    void addMetric(QString name, double event_value,
                   double aggregate_value = 0);
    void setMetric(QString name, double event_value,
                   double aggregate_value = 0);
    bool hasMetric(QString name);
    double getMetric(QString name, bool aggregate = false);
    QList<QString> getMetricList();

    class MetricPair {
    public:
        MetricPair(double _e, double _a)
            : event(_e), aggregate(_a) {}

        double event; // value at event
        double aggregate; // value at prev. aggregate event
    };

    QMap<QString, MetricPair *> * metrics; // Lateness or Counters etc
};

#endif // METRICS_H
