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
    virtual void draw(QPainter * painter, VisWidget * vis, VisOptions * options,
                      int w, int h)=0;

    void drawLine(QPainter * painter, VisWidget * vis, QPointF * p1, QPointF * p2);
};

#endif // COMMBUNDLE_H
