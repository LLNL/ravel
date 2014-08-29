#ifndef EVENTRECORD_H
#define EVENTRECORD_H

#include "event.h"

// Holder for OTF Event info
class EventRecord
{
public:
    EventRecord(unsigned int _p, unsigned long long int _t, unsigned int _v, bool _e = true);

    unsigned int process;
    unsigned long long int time;
    unsigned int value;
    bool enter;
    QList<Event *> children;
};

#endif // EVENTRECORD_H
