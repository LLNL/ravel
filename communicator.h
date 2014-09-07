#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <QString>
#include <QList>

class Communicator
{
public:
    Communicator(int _id, QString _name);
    ~Communicator() { delete processes; }

    int id;
    QString name;
    QList<unsigned int> * processes;

};

#endif // COMMUNICATOR_H
