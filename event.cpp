#include "event.h"
#include <iostream>

Event::Event(unsigned long long _enter, unsigned long long _exit,
             int _function, int _process)
    : caller(NULL),
      callees(new QVector<Event *>()),
      enter(_enter),
      exit(_exit),
      function(_function),
      process(_process),
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
