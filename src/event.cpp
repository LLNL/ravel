#include "event.h"
#include "function.h"
#include "metrics.h"
#include "rpartition.h"
#include <iostream>

Event::Event(unsigned long long _enter, unsigned long long _exit,
             int _function, unsigned long _entity, int _pe)
    : caller(NULL),
      callees(new QVector<Event *>()),
      enter(_enter),
      exit(_exit),
      function(_function),
      entity(_entity),
      pe(_pe),
      depth(-1),
      metrics(new Metrics())
{

}

Event::~Event()
{
    delete metrics;
    if (callees)
        delete callees;
}

bool Event::operator<(const Event &event)
{
    if (enter == event.enter)
    {
        if (this->isReceive())
            return true;
        else
            return false;
    }
    return enter < event.enter;
}

bool Event::operator>(const Event &event)
{
    if (enter == event.enter)
    {
        if (this->isReceive())
            return false;
        else
            return true;
    }
    return enter > event.enter;
}

bool Event::operator<=(const Event &event)
{
    return enter <= event.enter;
}

bool Event::operator>=(const Event &event)
{
    return enter >= event.enter;
}

bool Event::operator==(const Event &event)
{
    return enter == event.enter;
}

Event * Event::findChild(unsigned long long time)
{
    Event * result = NULL;
    Event * child_match = NULL;
    if (enter <= time && exit >= time)
    {
        result = this;
        for (QVector<Event *>::Iterator child = callees->begin();
             child != callees->end(); ++child)
        {
            child_match = (*child)->findChild(time);
            if (child_match)
            {
                result = child_match;
                break;
            }
        }
    }
    return result;
}

unsigned long long Event::getVisibleEnd(unsigned long long start)
{
    unsigned long long end = exit;
    for (QVector<Event *>::Iterator child = callees->begin();
         child != callees->end(); ++child)
    {
        if ((*child)->enter > start)
        {
            end = (*child)->enter;
            break;
        }
    }
    return end;
}

// Check if this Event and the argument Event share the same subtree:
// If they're not the same event, then either this event is in the
// subtree of the second or vice versa.
bool Event::same_subtree(Event * second)
{
    if (second == this)
        return true;

    Event * first = this;
    if (first->depth < second->depth)
    {
        while (first->depth < second->depth && second->caller)
        {
            second = second->caller;
            if (first == second)
                return true;
        }
    }
    else
    {
        while (first->depth > second->depth && first->caller)
        {
            first = first->caller;
            if (first == second)
                return true;
        }
    }

    return false;
}

// Find the ancestor that has at least two communications inside of it.
Event * Event::least_multiple_caller(QMap<Event *, int> * memo)
{
    Event * caller = this;
    while (caller && caller->comm_count(memo) <= 1)
    {
        caller = caller->caller;
    }
    return caller;
}

Event * Event::least_multiple_function_caller(QMap<int, Function *> * functions)
{
    if (functions->value(function)->comms > 1)
        return this;

    if (!caller)
        return NULL;

    return caller->least_multiple_function_caller(functions);
}


// Calculate the number of communications that fall under this node
// in the call tree.
int Event::comm_count(QMap<Event *, int> * memo)
{
    if (memo && memo->contains(this))
        return memo->value(this);

    int count = 0;
    for (QVector<Event *>::Iterator child = callees->begin();
         child != callees->end(); ++child)
    {
        count += (*child)->comm_count(memo);
    }
    memo->insert(this, count);
    return count;
}

void Event::writeToOTF2(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap)
{
    writeOTF2Enter(writer);

    for (QVector<Event *>::Iterator child = callees->begin();
         child != callees->end(); ++child)
    {
        (*child)->writeToOTF2(writer, attributeMap);
    }

    writeOTF2Leave(writer, attributeMap);
}

void Event::writeOTF2Leave(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap)
{
    Q_UNUSED(attributeMap);
    OTF2_EvtWriter_Leave(writer,
                         NULL,
                         exit,
                         function);
}

void Event::writeOTF2Enter(OTF2_EvtWriter * writer)
{
    OTF2_EvtWriter_Enter(writer,
                         NULL,
                         enter,
                         function);
}
