#ifndef EVENTRECORD_H
#define EVENTRECORD_H

// Holder for OTF info

class EventRecord
{
public:
    EventRecord();

    unsigned int process;
    unsigned int thread;
    unsigned long long time;
    unsigned int type;
    unsigned int value;
};

#endif // EVENTRECORD_H
