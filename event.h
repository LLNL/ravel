#ifndef EVENT_H
#define EVENT_H

#include <QWidget>
#include "message.h"

class Event
{
public:
    Event(unsigned long long _enter, unsigned long long _exit, int _function,
          int _process, int _step, long long _lateness);
    ~Event();

    QVector<Event *> * parents;
    QVector<Event *> * children;
    QVector<Message *> * messages;

    unsigned long long enter;
    unsigned long long exit;
    int function;
    int process;
    int step;
    long long lateness;

};

#endif // EVENT_H
