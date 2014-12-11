#ifndef COMMBUNDLE_H
#define COMMBUNDLE_H

class QPainter;
class CommEvent;
class CommDrawInterface;

class CommBundle
{
public:
    virtual CommEvent * getDesignee()=0; // Event responsible
    virtual void draw(QPainter * painter, CommDrawInterface * vis)=0;
};

#endif // COMMBUNDLE_H
