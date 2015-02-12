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
