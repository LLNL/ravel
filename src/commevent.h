#ifndef COMMEVENT_H
#define COMMEVENT_H

#include "event.h"
#include <QString>
#include <QList>
#include <QSet>
#include <QMap>

class Partition;
class ClusterEvent;
class CommBundle;
class Message;
class CollectiveRecord;

class CommEvent : public Event
{
public:
    CommEvent(unsigned long long _enter, unsigned long long _exit,
              int _function, int _task, int _pe, int _phase);
    ~CommEvent();

    void addMetric(QString name, double event_value,
                   double aggregate_value = 0);
    void setMetric(QString name, double event_value,
                   double aggregate_value = 0);
    bool hasMetric(QString name);
    double getMetric(QString name, bool aggregate = false);

    virtual int comm_count(QMap<Event *, int> *memo = NULL)=0;
    bool isCommEvent() { return true; }
    virtual bool isP2P() { return false; }
    virtual bool isReceive() { return false; }
    virtual bool isCollective() { return false; }
    virtual void writeOTF2Leave(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap);

    virtual void fixPhases()=0;
    virtual void calculate_differential_metric(QString metric_name,
                                               QString base_name,
                                               bool aggregates=true);
    virtual void initialize_strides(QList<CommEvent *> * stride_events,
                                    QList<CommEvent *> * recv_events)=0;
    virtual void update_strides() { return; }
    virtual void initialize_basic_strides(QSet<CollectiveRecord *> * collectives)=0;
    virtual void update_basic_strides()=0;
    virtual bool calculate_local_step()=0;

    virtual ClusterEvent * createClusterEvent(QString metric, long long divider)=0;
    virtual void addToClusterEvent(ClusterEvent * ce, QString metric,
                                   long long divider)=0;

    virtual void addComms(QSet<CommBundle *> * bundleset)=0;
    virtual QList<int> neighborTasks()=0;
    virtual QVector<Message *> * getMessages() { return NULL; }
    virtual CollectiveRecord * getCollective() { return NULL; }

    virtual QSet<Partition *> * mergeForMessagesHelper()=0;

    class MetricPair {
    public:
        MetricPair(double _e, double _a)
            : event(_e), aggregate(_a) {}

        double event; // value at event
        double aggregate; // value at prev. aggregate event
    };

    QMap<QString, MetricPair *> * metrics; // Lateness or Counters etc

    Partition * partition;
    CommEvent * comm_next;
    CommEvent * comm_prev;
    CommEvent * true_next;
    CommEvent * true_prev;

    // Used in stepping procedure
    CommEvent * last_stride;
    CommEvent * next_stride;
    QList<CommEvent *> * last_recvs;
    int last_step;
    QSet<CommEvent *> * stride_parents;
    QSet<CommEvent *> * stride_children;
    int stride;

    int step;
    int phase;

    // For graph drawing
    QString gvid;

};

#endif // COMMEVENT_H
