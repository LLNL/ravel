#include "event.h"
#include <iostream>

Event::Event(unsigned long long _enter, unsigned long long _exit,
             int _function, int _task)
    : caller(NULL),
      callees(new QVector<Event *>()),
      enter(_enter),
      exit(_exit),
      function(_function),
      task(_task),
      depth(-1)
{

}

Event::~Event()
{
    if (callees)
        delete callees;
}

bool Event::operator<(const Event &event)
{
    return enter < event.enter;
}

bool Event::operator>(const Event &event)
{
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

Event * Event::least_common_caller(Event * second)
{
    // Search while we still can until they are equal or there
    // is nothing left we can do
    Event * first = this;
    while ((first != second) && (first->caller || second->caller))
    {
        if (first->depth > second->depth && first->caller)
        {
            first = first->caller;
        }
        else if (first->depth < second->depth && second->caller)
        {
            second = second->caller;
        }
        else if (first->depth == second->depth && first->caller && second->caller)
        {
            first = first->caller;
            second = second->caller;
        }
    }

    if (first == second)
        return first;
    return NULL;
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
