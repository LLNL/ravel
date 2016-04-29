#ifndef ENTITY_H
#define ENTITY_H

#include <QString>

class PrimaryEntityGroup;

class Entity
{
public:
    Entity(unsigned long _id, QString _name, PrimaryEntityGroup * _primary);

    unsigned long id;
    QString name;

    PrimaryEntityGroup * primary;
};

#endif // ENTITY_H
