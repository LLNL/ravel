#ifndef GNOME_H
#define GNOME_H

#include "partition.h"

class Gnome
{
public:
    Gnome();

    virtual bool detectGnome(Partition * part);
    // virtual void drawGnomeQt(QPainter * painter);
    // virtual void drawGnomeGL();
};

#endif // GNOME_H
