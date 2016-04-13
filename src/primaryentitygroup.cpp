#include "primaryentitygroup.h"
#include "entity.h"

PrimaryEntityGroup::PrimaryEntityGroup(int _id, QString _name)
    : id(_id),
      name(_name),
      entities(new QList<Entity *>())
{
}

PrimaryEntityGroup::~PrimaryEntityGroup()
{
    for (QList<Entity *>::Iterator entity = entities->begin();
         entity != entities->end(); ++entity)
    {
        delete *entity;
        *entity = NULL;
    }
    delete entities;
}
