#include "entitygroup.h"

EntityGroup::EntityGroup(int _id, QString _name)
    : id(_id),
      name(_name),
      entities(new QList<unsigned int>()),
      entityorder(new QMap<unsigned int, int>())
{
}
