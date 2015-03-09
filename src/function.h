#ifndef FUNCTION_H
#define FUNCTION_H

#include <QString>

// Information about function calls from OTF
class Function
{
public:
    Function(QString _n, int _g, QString _s = "", int _c = 0);

    QString name;
    QString shortname;
    int group;
    int comms; // max comms in a function
    bool isMain;
};

#endif // FUNCTION_H
