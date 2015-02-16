#include "task.h"

Task::Task(int _id, QString _name, PrimaryTaskGroup *_ptg)
    : id(_id),
      name(_name),
      primary(_ptg)
{
}
