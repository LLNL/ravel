#ifndef COUNTER_H
#define COUNTER_H

#include <QString>

class Counter
{
public:
    Counter(int _id, QString _name, QString _unit);

    unsigned int id;
    QString name;
    QString unit;
};

#endif // COUNTER_H
