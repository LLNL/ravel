#include "entity.h"

Entity::Entity(long _id, QString _name, PrimaryEntityGroup *_primary)
    : id(_id),
      name(_name),
      primary(_primary)
{
}
