#ifndef COMMBUNDLE_H
#define COMMBUNDLE_H

#include <QPainter>
#include "visoptions.h"

class VisWidget;
class CommEvent;

class CommBundle
{
public:
    CommBundle();
    virtual CommEvent * getDesignee()=0; // Event responsible
    virtual void draw(QPainter * painter, VisWidget * vis)=0;
};

#endif // COMMBUNDLE_H
