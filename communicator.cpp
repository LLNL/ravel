#include "communicator.h"

Communicator::Communicator(int _id, QString _name)
    : id(_id),
      name(_name),
      processes(new QList<unsigned int>())
{
}
