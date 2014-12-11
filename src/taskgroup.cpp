#include "taskgroup.h"

TaskGroup::TaskGroup(int _id, QString _name)
    : id(_id),
      name(_name),
      tasks(new QList<unsigned int>()),
      taskorder(new QMap<unsigned int, int>())
{
}
