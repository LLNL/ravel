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
    virtual void preprocess(VisOptions * _options) { options = _options; }
    virtual void drawGnomeQt(QPainter * painter, QRect extents)
        { Q_UNUSED(painter); Q_UNUSED(extents); }
    virtual void drawGnomeGL(QRect extents) { Q_UNUSED(extents); }

protected:
    Partition * partition;
    VisOptions * options;
};

#endif // GNOME_H
