#ifndef OTFCOLLECTIVE_H
#define OTFCOLLECTIVE_H

#include <QString>

class OTFCollective
{
public:
    OTFCollective(int _id, int _type, QString _name);

    int id;
    int type;
    QString name;
};

#endif // OTFCOLLECTIVE_H
