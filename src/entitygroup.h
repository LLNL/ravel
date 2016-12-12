#ifndef ENTITYGROUP_H
#define ENTITYGROUP_H

#include <QString>
#include <QList>
#include <QMap>

// This class is for sub-groupings and reorderings of existing PrimaryEntityGroups.
class EntityGroup
{
public:
    EntityGroup(int _id, QString _name);
    ~EntityGroup() { delete entities; delete entityorder; }

    int id;
    QString name;
    // May need to keep additional names if we have several that are the same

    // This order is important for communicator rank ID
    QList<unsigned long long> * entities;
    QMap<unsigned long, int> * entityorder;
};

#endif // ENTITYGROUP_H
