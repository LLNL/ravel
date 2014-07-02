#ifndef COMMEVENT_H
#define COMMEVENT_H

#include "event.h"

class CommEvent : public Event
{
public:
    CommEvent(unsigned long long _enter, unsigned long long _exit,
              int _function, int _process, int _phase);
    ~CommEvent();

    void addMetric(QString name, long long event_value,
                   long long aggregate_value = 0);
    bool hasMetric(QString name);
    long long getMetric(QString name, bool aggregate = false);

    virtual bool isP2P();
    virtual bool isReceive();
    virtual void set_stride_relationships(CommEvent * base);
    virtual QVector<Message *> * getMessages() { return NULL; }
    virtual CollectiveRecord * getCollective() { return NULL; }

    class MetricPair {
    public:
        MetricPair(long long _e, long long _a)
            : event(_e), aggregate(_a) {}

        long long event; // value at event
        long long aggregate; // value at prev. aggregate event
    };

    QMap<QString, MetricPair *> * metrics; // Lateness or Counters etc

    Partition * partition;
    CommEvent * comm_next;
    CommEvent * comm_prev;

    // Used in stepping procedure
    CommEvent * last_send;
    CommEvent * next_send;
    QList<CommEvent *> * last_recvs;
    int last_step;
    QSet<CommEvent *> * stride_parents;
    QSet<CommEvent *> * stride_children;
    int stride;

    int step;
    int phase;

};

#endif // COMMEVENT_H
