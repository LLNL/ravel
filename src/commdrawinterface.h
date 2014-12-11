#ifndef COMMDRAWINTERFACE_H
#define COMMDRAWINTERFACE_H

class QPainter;
class Message;
class CollectiveRecord;

class CommDrawInterface
{
public:
    virtual void drawMessage(QPainter * painter, Message * message)=0;
    virtual void drawCollective(QPainter * painter, CollectiveRecord * cr)=0;
};

#endif // COMMDRAWINTERFACE_H
