#ifndef PRIMARYENTITYGROUP_H
#define PRIMARYENTITYGROUP_H

#include <QList>
#include <QMap>
#include <QString>

class Entity;

// This is the main class for organizing entities.
// In MPI traces, we'll only have one (essentially MPI_COMM_WORLD)
// but in something like Charm++ we'll have many
class PrimaryEntityGroup
{
public:
    PrimaryEntityGroup(int _id, QString _name);
    ~PrimaryEntityGroup();

    int id;
    QString name;

    // This order is important for communicator rank ID
    QList<Entity *> * entities;
};

#endif // PRIMARYENTITYGROUP_H
