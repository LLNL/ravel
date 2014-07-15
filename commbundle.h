#ifndef COMMBUNDLE_H
#define COMMBUNDLE_H

#include <QPainter>
#include "visoptions.h"
#include "commdrawinterface.h"

class CommEvent;

class CommBundle
{
public:
    virtual CommEvent * getDesignee()=0; // Event responsible
    virtual void draw(QPainter * painter, CommDrawInterface * vis)=0;
};

#endif // COMMBUNDLE_H
