#ifndef ENTITY_H
#define ENTITY_H

#include <QString>

class PrimaryEntityGroup;

class Entity
{
public:
    Entity(int _id, QString _name, PrimaryEntityGroup * _primary);

    int id;
    QString name;

    PrimaryEntityGroup * primary;
};

#endif // ENTITY_H
