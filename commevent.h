#ifndef COMMEVENT_H
#define COMMEVENT_H

#include "event.h"
#include "clusterevent.h"

class Partition;

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

    bool isCommEvent() { return true; }
    virtual bool isP2P() { return false; }
    virtual bool isReceive() { return false; }
    virtual bool isCollective() { return false; }
    virtual void fixPhases()=0;
    virtual void calculate_differential_metric(QString metric_name,
                                               QString base_name);
    virtual void initialize_strides(QList<CommEvent *> * stride_events,
                                    QList<CommEvent *> * recv_events)=0;
    virtual void update_strides() { return; }

    virtual ClusterEvent * createClusterEvent(QString metric, long long divider)=0;
    virtual void addToClusterEvent(ClusterEvent * ce, QString metric,
                                   long long divider)=0;

    virtual void addComms(QSet<CommBundle *> * bundleset)=0;
    virtual QVector<Message *> * getMessages() { return NULL; }
    virtual CollectiveRecord * getCollective() { return NULL; }

    virtual QSet<Partition *> * mergeForMessagesHelper()=0;

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