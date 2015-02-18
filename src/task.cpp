#include "task.h"

Task::Task(int _id, QString _name, PrimaryTaskGroup *_primary)
    : id(_id),
      name(_name),
      primary(_primary)
{
}
