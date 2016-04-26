#ifndef EVENT_H
#define EVENT_H

#include <QVector>
#include <QMap>
#include <QString>
#include <otf2/otf2.h>

class Function;
class QPainter;
class CommDrawInterface;
class Metrics;

class Event
{
public:
    Event(unsigned long long _enter, unsigned long long _exit, int _function,
          unsigned long _entity, int _pe);
    ~Event();

    // Based on enter time
    bool operator<(const Event &);
    bool operator>(const Event &);
    bool operator<=(const Event &);
    bool operator>=(const Event &);
    bool operator==(const Event &);
    static bool eventEntityLessThan(const Event * evt1, const Event * evt2)
    {
        return evt1->entity < evt2->entity;
    }

    Event * findChild(unsigned long long time);
    unsigned long long getVisibleEnd(unsigned long long start);
    bool same_subtree(Event * other);
    Event * least_multiple_caller(QMap<Event *, int> * memo = NULL);
    Event * least_multiple_function_caller(QMap<int, Function *> * functions);
    virtual int comm_count(QMap<Event *, int> * memo = NULL);
    virtual void track_delay(QPainter *painter, CommDrawInterface * vis)
        { Q_UNUSED(painter); Q_UNUSED(vis); }
    virtual bool isCommEvent() { return false; }
    virtual bool isReceive() const { return false; }
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
    unsigned long entity;
    int pe;
    int depth;

    Metrics * metrics; // Lateness or Counters etc
};

#endif // EVENT_H
