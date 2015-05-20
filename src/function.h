#ifndef FUNCTION_H
#define FUNCTION_H

#include <QString>

// Information about function calls from OTF
class Function
{
public:
    Function(QString _n, int _g);

    QString name;
    int group;
    int comms; // max comms in a function
};

#endif // FUNCTION_H
