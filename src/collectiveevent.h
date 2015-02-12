#ifndef COLLECTIVEEVENT_H
#define COLLECTIVEEVENT_H

#include "commevent.h"
#include "collectiverecord.h"

class CollectiveEvent : public CommEvent
{
public:
    CollectiveEvent(unsigned long long _enter, unsigned long long _exit,
                    int _function, int _task, int _phase,
                    CollectiveRecord * _collective);
    ~CollectiveEvent();

    // We count the collective as two since it serves as both the beginning
    // and ending of some sort of communication while P2P communication
    // events are either the begin (send) or the end (recv)
    int comm_count(QMap<Event *, int> *memo = NULL) { Q_UNUSED(memo); return 2; }

    bool isP2P() { return false; }
    bool isReceive() { return false; }
    virtual bool isCollective() { return true; }
    void fixPhases();
    void initialize_strides(QList<CommEvent *> * stride_events,
                            QList<CommEvent *> * recv_events);
    void initialize_basic_strides(QSet<CollectiveRecord *> *collectives);
    void update_basic_strides();
    bool calculate_local_step();
    void writeToOTF2(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap);

    void addComms(QSet<CommBundle *> * bundleset) { bundleset->insert(collective); }
    QList<int> neighborTasks();
    CollectiveRecord * getCollective() { return collective; }
    QSet<Partition *> * mergeForMessagesHelper();

    ClusterEvent * createClusterEvent(QString metric, long long divider);
    void addToClusterEvent(ClusterEvent * ce, QString metric,
                           long long divider);

    CollectiveRecord * collective;

private:
    void set_stride_relationships();
};

#endif // COLLECTIVEEVENT_H
