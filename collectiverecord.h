#ifndef COLLECTIVERECORD_H
#define COLLECTIVERECORD_H

#include <QMap>
#include "commbundle.h"

class CollectiveEvent;

// Information we get from OTF about collectives
class CollectiveRecord : public CommBundle
{
public:
    CollectiveRecord(unsigned long long int _matching, unsigned int _root,
                     unsigned int _collective, unsigned int _taskgroup);

    unsigned long long int matchingId;
    unsigned int root;
    unsigned int collective;
    unsigned int taskgroup;
    bool mark;

    // Map from process to enter/leave times
    //QMap<int, std::pair<unsigned long long, unsigned long long> > * times;
    QList<CollectiveEvent *> * events;

    CommEvent * getDesignee();
    void draw(QPainter * painter, CommDrawInterface * vis);
};

#endif // COLLECTIVERECORD_H
