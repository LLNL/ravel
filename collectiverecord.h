#ifndef COLLECTIVERECORD_H
#define COLLECTIVERECORD_H

#include <QMap>

class Event;

// Information we get from OTF about collectives
class CollectiveRecord
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
    QList<Event *> * events;

};

#endif // COLLECTIVERECORD_H
