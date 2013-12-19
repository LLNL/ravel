#include "event.h"
#include <iostream>

Event::Event(unsigned long long _enter, unsigned long long _exit,
             int _function, int _process, int _step)
    : parents(new QVector<Event *>()),
      children(new QVector<Event *>()),
      messages(new QVector<Message *>()),
      metrics(new QMap<QString, MetricPair *>()),
      caller(NULL),
      callees(new QVector<Event *>()),
      partition(NULL),
      comm_next(NULL),
      comm_prev(NULL),
      is_recv(false),
      last_send(NULL),
      next_send(NULL),
      last_recvs(NULL),
      last_step(-1),
      enter(_enter),
      exit(_exit),
      function(_function),
      process(_process),
      step(_step),
      depth(-1),
      phase(-1),
      mark(false)
{

}

Event::~Event()
{
    delete parents;
    delete children;

    for (QVector<Message *>::Iterator itr = messages->begin(); itr != messages->end(); ++itr) {
            delete *itr;
            *itr = NULL;
    }
    delete messages;

    for (QMap<QString, MetricPair *>::Iterator itr = metrics->begin(); itr != metrics->end(); ++itr) {
        delete itr.value();
    }
    delete metrics;

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


void Event::addMetric(QString name, long long event_value, long long aggregate_value)
{
    (*metrics)[name] = new MetricPair(event_value, aggregate_value);
}
