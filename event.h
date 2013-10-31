#ifndef EVENT_H
#define EVENT_H

#include <QWidget>
#include <QMap>
#include "message.h"

class Event
{
public:
    Event(unsigned long long _enter, unsigned long long _exit, int _function,
          int _process, int _step);
    ~Event();
    void addMetric(QString name, long long event_value, long long aggregate_value = 0);

    class MetricPair {
    public:
        MetricPair(long long _e, long long _a)
            : event(_e), aggregate(_a) {}

        long long event;
        long long aggregate;
    };

    QVector<Event *> * parents;
    QVector<Event *> * children;
    QVector<Message *> * messages;
    QMap<QString, MetricPair *> * metrics; // Lateness or Counters etc

    unsigned long long enter;
    unsigned long long exit;
    int function;
    int process;
    int step;

};

#endif // EVENT_H
