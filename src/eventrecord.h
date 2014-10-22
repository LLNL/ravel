#ifndef EVENTRECORD_H
#define EVENTRECORD_H

#include <QList>
#include <QString>
#include <QMap>

class Event;

// Holder for OTF Event info
class EventRecord
{
public:
    EventRecord(unsigned int _task, unsigned long long int _t, unsigned int _v, bool _e = true);
    ~EventRecord();

    unsigned int task;
    unsigned long long int time;
    unsigned int value;
    bool enter;
    QList<Event *> children;
    QMap<QString, unsigned long long> * metrics;
    QMap<QString, int> * ravel_info;

    // Based on time
    bool operator<(const EventRecord &);
    bool operator>(const EventRecord &);
    bool operator<=(const EventRecord &);
    bool operator>=(const EventRecord &);
    bool operator==(const EventRecord &);
};

#endif // EVENTRECORD_H
