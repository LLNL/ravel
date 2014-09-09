#ifndef EVENT_H
#define EVENT_H

#include <QWidget>
#include <QMap>
#include <QSet>
#include "message.h"
#include "collectiverecord.h"

class Partition;

class Event
{
public:
    Event(unsigned long long _enter, unsigned long long _exit, int _function,
          int _process);
    ~Event();

    // Based on enter time
    bool operator<(const Event &);
    bool operator>(const Event &);
    bool operator<=(const Event &);
    bool operator>=(const Event &);
    bool operator==(const Event &);

    Event * findChild(unsigned long long time);
    unsigned long long getVisibleEnd(unsigned long long start);
    virtual bool isCommEvent() { return false; }
    virtual bool isReceive() { return false; }
    virtual bool isCollective() { return false; }

    // Call tree info
    Event * caller;
    QVector<Event *> * callees;

    unsigned long long enter;
    unsigned long long exit;
    int function;
    int process;
    int depth;
};

static bool eventProcessLessThan(const Event * evt1, const Event * evt2)
{
    return evt1->process < evt2->process;
}

#endif // EVENT_H
