#ifndef P2PEVENT_H
#define P2PEVENT_H

#include "commevent.h"

class P2PEvent : public CommEvent
{
public:
    P2PEvent(unsigned long long _enter, unsigned long long _exit,
             int _function, int _task, int _pe, int _phase,
             QVector<Message *> * _messages = NULL);
    P2PEvent(QList<P2PEvent *> * _subevents);
    ~P2PEvent();
    int comm_count(QMap<Event *, int> *memo = NULL) { Q_UNUSED(memo); return 1; }
    bool isP2P() { return true; }
    bool isReceive() const;
    void fixPhases();
    void initialize_strides(QList<CommEvent *> * stride_events,
                            QList<CommEvent *> * recv_events);
    void update_strides();
    void set_reorder_strides(QMap<int, QList<CommEvent *> *> * stride_map,
                             int offset);
    void initialize_basic_strides(QSet<CollectiveRecord *> * collectives);
    void update_basic_strides();
    bool calculate_local_step();
    void calculate_differential_metric(QString metric_name,
                                       QString base_name,
                                       bool aggregates);
    void writeToOTF2(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap);

    void addComms(QSet<CommBundle *> * bundleset);
    QList<int> neighborTasks();
    QVector<Message *> * getMessages() { return messages; }
    QSet<Partition *> * mergeForMessagesHelper();

    ClusterEvent * createClusterEvent(QString metric, long long divider);
    void addToClusterEvent(ClusterEvent * ce, QString metric,
                           long long divider);

    // ISend coalescing
    QList<P2PEvent *> * subevents;

    // Messages involved wiht this event
    QVector<Message *> * messages;

    bool is_recv;

private:
    void set_stride_relationships(CommEvent * base);
};

#endif // P2PEVENT_H
