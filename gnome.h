#ifndef GNOME_H
#define GNOME_H

#include "partition.h"

class Gnome
{
public:
    Gnome();

    virtual bool detectGnome(Partition * part);
    virtual Gnome * create();
    void setPartition(Partition * part) { partition = part; }
    virtual void preprocess() { }
    virtual void drawGnomeQt(QPainter * painter, QRect extents) { Q_UNUSED(painter); Q_UNUSED(extents); }
    virtual void drawGnomeGL(QRect extents) { Q_UNUSED(extents); }

private:
    Partition * partition;
};

#endif // GNOME_H
