#ifndef EVENT_H
#define EVENT_H

#include <QWidget>
#include "message.h"

class Event
{
public:
    Event();

private:
    unsigned long long enter;
    unsigned long long exit;
    int function;
    int process;
    int step;
    long long lateness;
    QVector<Event *> parents;
    QVector<Event *> children;
    QVector<Message *> messages;

};

#endif // EVENT_H
