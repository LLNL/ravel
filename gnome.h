#ifndef GNOME_H
#define GNOME_H

#include "partition.h"
#include "visoptions.h"

class Gnome
{
public:
    Gnome();

    virtual bool detectGnome(Partition * part);
    virtual Gnome * create();
    void setPartition(Partition * part) { partition = part; }
    virtual void preprocess() {}
    virtual void drawGnomeQt(QPainter * painter, QRect extents)
        { Q_UNUSED(painter); Q_UNUSED(extents); }
    virtual void drawGnomeGL(QRect extents, VisOptions * _options) { Q_UNUSED(extents); options = _options; }

protected:
    Partition * partition;
    VisOptions * options;
};

#endif // GNOME_H
