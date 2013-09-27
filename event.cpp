#include "event.h"

Event::Event(unsigned long long _enter, unsigned long long _exit,
             int _function, int _process, int _step, long long _lateness)
    : enter(_enter), exit(_exit), function(_function), process(_process),
    step(_step), lateness(_lateness)
{
    parents = new QVector<Event *>();
    children = new QVector<Event *>();
    messages = new QVector<Message* >();
}

Event::~Event()
{
    delete parents;
    delete children;
    for (QVector<Message *>::Iterator itr = messages->begin(); itr != messages->end(); itr++) {
            delete *itr;
            *itr = NULL;
    }
    delete messages;
}
