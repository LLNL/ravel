#ifndef EVENT_H
#define EVENT_H

#include <QVector>
#include <QMap>
#include <QString>
#include <otf2/otf2.h>


class Partition;

class Event
{
public:
    Event(unsigned long long _enter, unsigned long long _exit, int _function,
          int _task);
    ~Event();

    // Based on enter time
    bool operator<(const Event &);
    bool operator>(const Event &);
    bool operator<=(const Event &);
    bool operator>=(const Event &);
    bool operator==(const Event &);

    Event * findChild(unsigned long long time);
    unsigned long long getVisibleEnd(unsigned long long start);
    Event * least_common_caller(Event * other);
    bool same_subtree(Event * other);
    Event * least_multiple_caller(QMap<Event *, int> * memo = NULL);
    virtual int comm_count(QMap<Event *, int> * memo = NULL);
    virtual bool isCommEvent() { return false; }
    virtual bool isReceive() { return false; }
    virtual bool isCollective() { return false; }
    virtual void writeToOTF2(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap);
    virtual void writeOTF2Leave(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap);
    virtual void writeOTF2Enter(OTF2_EvtWriter * writer);

    // Call tree info
    Event * caller;
    QVector<Event *> * callees;

    unsigned long long enter;
    unsigned long long exit;
    int function;
    int task;
    int depth;
};

static bool eventTaskLessThan(const Event * evt1, const Event * evt2)
{
    return evt1->task < evt2->task;
}

#endif // EVENT_H
