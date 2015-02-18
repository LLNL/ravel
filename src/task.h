#ifndef TASK_H
#define TASK_H

#include <QString>

class PrimaryTaskGroup;

class Task
{
public:
    Task(int _id, QString _name, PrimaryTaskGroup * _primary);

    int id;
    QString name;

    PrimaryTaskGroup * primary;
};

#endif // TASK_H
