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
                     unsigned int _collective, unsigned int _communicator);

    unsigned long long int matchingId;
    unsigned int root;
    unsigned int collective;
    unsigned int communicator;
    bool mark;

    // Map from process to enter/leave times
    //QMap<int, std::pair<unsigned long long, unsigned long long> > * times;
    QList<CollectiveEvent *> * events;

    CommEvent * getDesignee();
    void draw(QPainter * painter, VisWidget * vis, VisOptions * options,
              int w, int h);
    void drawArc(QPainter * painter, VisWidget * vis,
                 QPointF * p1, QPointF * p2,
                 int width, bool forward = true);
};

#endif // COLLECTIVERECORD_H
