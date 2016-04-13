#include "eventrecord.h"
#include "event.h"

EventRecord::EventRecord(unsigned int _entity, unsigned long long int _t,
                         unsigned int _v, bool _e)
    : entity(_entity),
      time(_t),
      value(_v),
      enter(_e),
      children(QList<Event *>()),
      metrics(NULL),
      ravel_info(NULL)
{
}

EventRecord::~EventRecord()
{
    if (metrics)
        delete metrics;
    if (ravel_info)
        delete ravel_info;
}

bool EventRecord::operator<(const EventRecord &event)
{
    return enter < event.enter;
}

bool EventRecord::operator>(const EventRecord &event)
{
    return enter > event.enter;
}

bool EventRecord::operator<=(const EventRecord &event)
{
    return enter <= event.enter;
}

bool EventRecord::operator>=(const EventRecord &event)
{
    return enter >= event.enter;
}

bool EventRecord::operator==(const EventRecord &event)
{
    return enter == event.enter;
}
