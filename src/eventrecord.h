#ifndef EVENTRECORD_H
#define EVENTRECORD_H

#include <QList>

class Event;

// Holder for OTF Event info
class EventRecord
{
public:
    EventRecord(unsigned int _task, unsigned long long int _t, unsigned int _v, bool _e = true);

    unsigned int task;
    unsigned long long int time;
    unsigned int value;
    bool enter;
    QList<Event *> children;
};

#endif // EVENTRECORD_H
