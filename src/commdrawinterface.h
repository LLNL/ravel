#ifndef COMMDRAWINTERFACE_H
#define COMMDRAWINTERFACE_H

class QPainter;
class Message;
class CollectiveRecord;
class CommEvent;

class CommDrawInterface
{
public:
    virtual void drawMessage(QPainter * painter, Message * message)=0;
    virtual void drawCollective(QPainter * painter, CollectiveRecord * cr)=0;
    virtual void drawDelayTracking(QPainter * painter, CommEvent * p2p)=0;
};

#endif // COMMDRAWINTERFACE_H
