#ifndef COLLECTIVEEVENT_H
#define COLLECTIVEEVENT_H

#include "commevent.h"

class CollectiveEvent : public CommEvent
{
public:
    CollectiveEvent(unsigned long long _enter, unsigned long long _exit,
                    int _function, int _process, int _phase,
                    CollectiveRecord * _collective);
    ~CollectiveEvent();
    bool isP2P() { return false; }
    bool isReceive() { return false; }
    virtual bool isCollective() { return true; }
    void set_stride_relationships(CommEvent * base);
    CollectiveRecord * getCollective() { return collective; }
    QSet<Partition *> * mergeForMessagesHelper();

    CollectiveRecord * collective;
};

#endif // COLLECTIVEEVENT_H
