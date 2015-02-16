#ifndef PRIMARYTASKGROUP_H
#define PRIMARYTASKGROUP_H

#include <QList>
#include <QMap>
#include <QString>

class Task;

// This is the main class for organizing tasks.
// In MPI traces, we'll only have one (essentially MPI_COMM_WORLD)
// but in something like Charm++ we'll have many
class PrimaryTaskGroup
{
public:
    PrimaryTaskGroup(int _id, QString _name);
    ~PrimaryTaskGroup();

    int id;
    QString name;

    // This order is important for communicator rank ID
    QList<Task *> * tasks;
};

#endif // PRIMARYTASKGROUP_H
