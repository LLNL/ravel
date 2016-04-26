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
class Metrics;

class CommEvent : public Event
{
public:
    CommEvent(unsigned long long _enter, unsigned long long _exit,
              int _function, int _entity, int _pe, int _phase);
    ~CommEvent();

    bool operator<(const CommEvent &);
    bool operator>(const CommEvent &);
    bool operator<=(const CommEvent &);
    bool operator>=(const CommEvent &);
    bool operator==(const CommEvent &);

    // Compare based on strides... if there is a tie, take the one
    // that comes from a less entity id. If the entity ids are the same
    // we prioriize the send as that is probably connected to a preceding
    // recv. Otherwise, we just go by time
    static bool eventStrideLessThan(const CommEvent * evt1, const CommEvent * evt2)
    {
        if (evt1->last_stride->stride == evt2->last_stride->stride)
        {
            // This should only happen on receives, but just in case
            if (evt1->last_stride->next_stride && evt2->last_stride->next_stride)
            {
                if (evt1->last_stride->next_stride->entity == evt2->last_stride->next_stride->entity)
                {
                    // Happens in the case where entity X and entity Y send to entity Z
                    // and both of those Z entries send back to entity X and entity Y
                    // recv X and recv Y have the same number and were sent by
                    // the same entity and have the same stride.
                    if (evt1->stride == evt2->stride)
                    {
                        // Need to go back to the senders to figure it out
                        if (evt1->last_stride->next_stride
                            && evt2->last_stride->next_stride)
                            return eventStrideLessThan(evt1->last_stride->next_stride,
                                                       evt2->last_stride->next_stride);


                        if (evt1->isReceive() && !evt2->isReceive())
                            return evt2;
                        else if (!evt1->isReceive() && evt2->isReceive())
                            return evt1;
                        else
                            return evt1->enter < evt2->enter;

                    }
                    else
                    {
                        return evt1->stride < evt2->stride;
                    }
                }
                else
                {
                    return evt1->last_stride->next_stride->entity < evt2->last_stride->next_stride->entity;
                }
            }
            else
            {
                if (evt1->stride == evt2->stride)
                {
                    // Need to go back to the senders to figure it out
                    if (evt1->last_stride->next_stride
                        && evt2->last_stride->next_stride)
                        return eventStrideLessThan(evt1->last_stride->next_stride,
                                                   evt2->last_stride->next_stride);

                    if (evt1->isReceive() && !evt2->isReceive())
                        return evt2;
                    else if (!evt1->isReceive() && evt2->isReceive())
                        return evt1;
                    else
                        return evt1->enter < evt2->enter;

                }
                else
                {
                    return evt1->stride < evt2->stride;
                }
            }
        }

        return evt1->last_stride->stride < evt2->last_stride->stride;
    }

    static bool eventStrideLessThanMPI(const CommEvent * evt1, const CommEvent * evt2)
    {
        if (evt1->stride == evt2->stride)
        {
            if (evt1->next_stride && evt2->next_stride)
            {
                return evt1->next_stride->entity < evt2->next_stride->entity;
            }
            else
            {
                if (evt1->isReceive() && !evt2->isReceive())
                    return evt2;
                else if (!evt1->isReceive() && evt2->isReceive())
                    return evt1;
                else
                    return evt1->enter < evt2->enter;
            }
        }
        return evt1->stride < evt2->stride;
    }

    bool hasMetric(QString name);
    double getMetric(QString name, bool aggregate = false);

    virtual int comm_count(QMap<Event *, int> *memo = NULL)=0;
    bool isCommEvent() { return true; }
    virtual bool isP2P() { return false; }
    virtual bool isReceive() const { return false; }
    virtual bool isCollective() { return false; }
    virtual void writeOTF2Leave(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap);

    virtual void fixPhases()=0;
    virtual void calculate_differential_metric(QString metric_name,
                                               QString base_name,
                                               bool aggregates=true);
    virtual void initialize_strides(QList<CommEvent *> * stride_events,
                                    QList<CommEvent *> * recv_events)=0;
    virtual void update_strides() { return; }
    virtual void set_reorder_strides(QMap<unsigned long, QList<CommEvent *> *> * stride_map,
                                     int offset, CommEvent * last = NULL, int debug = -1)
                                    { Q_UNUSED(stride_map); Q_UNUSED(offset); Q_UNUSED(last); Q_UNUSED(debug); return; }
    virtual void initialize_basic_strides(QSet<CollectiveRecord *> * collectives)=0;
    virtual void update_basic_strides()=0;
    virtual bool calculate_local_step()=0;

    virtual ClusterEvent * createClusterEvent(QString metric, long long divider)=0;
    virtual void addToClusterEvent(ClusterEvent * ce, QString metric,
                                   long long divider)=0;

    virtual CommEvent * compare_to_sender(CommEvent * prev) { return prev; }

    virtual void addComms(QSet<CommBundle *> * bundleset)=0;
    virtual QList<int> neighborEntities()=0;
    virtual QVector<Message *> * getMessages() { return NULL; }
    virtual CollectiveRecord * getCollective() { return NULL; }

    virtual QSet<Partition *> * mergeForMessagesHelper()=0;

    Partition * partition;
    CommEvent * comm_next;
    CommEvent * comm_prev;
    CommEvent * true_next;
    CommEvent * true_prev;
    CommEvent * pe_next;
    CommEvent * pe_prev;

    // For sorting when overlaps
    int add_order;

    // For duration and other issues
    unsigned long long extent_begin;
    unsigned long long extent_end;

    // Charm atomic
    int atomic;
    long matching;

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
