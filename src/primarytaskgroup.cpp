#include "primarytaskgroup.h"

PrimaryTaskGroup::PrimaryTaskGroup(int _id, QString _name)
    : id(_id),
      name(_name),
      tasks(new QList<Task *>())
{
}

PrimaryTaskGroup::~PrimaryTaskGroup()
{
    for (QList<Task *>::Iterator task = tasks->begin();
         task != tasks->end(); ++task)
    {
        delete *task;
        *task = NULL;
    }
    delete tasks;
}
