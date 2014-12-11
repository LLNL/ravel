#ifndef TASK_H
#define TASK_H

#include <QString>

class Task
{
public:
    Task(int _id, QString _name);

    int id;
    QString name;
};

#endif // TASK_H
