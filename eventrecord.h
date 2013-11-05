#ifndef EVENTRECORD_H
#define EVENTRECORD_H

// Holder for OTF info

class EventRecord
{
public:
    EventRecord(unsigned int _p, unsigned long long int _t, unsigned int _v);

    unsigned int process;
    unsigned long long int time;
    unsigned int value;
};

#endif // EVENTRECORD_H
