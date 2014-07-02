#ifndef P2PEVENT_H
#define P2PEVENT_H

#include "commevent.h"

class P2PEvent : public CommEvent
{
public:
    P2PEvent(unsigned long long _enter, unsigned long long _exit,
             int _function, int _process, int _phase,
             QVector<Message *> * _messages = NULL);
    ~P2PEvent();
    virtual bool isP2P() { return true; }
    bool isReceive();
    void set_stride_relationships(CommEvent * base);
    virtual QVector<Message *> * getMessages() { return messages; }

    // ISend coalescing
    QList<P2PEvent *> * subevents;

    // Messages involved wiht this event
    QVector<Message *> * messages;

    bool is_recv;
};

#endif // P2PEVENT_H
