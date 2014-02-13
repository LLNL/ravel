#ifndef GNOME_H
#define GNOME_H

#include "partition.h"

class Gnome
{
public:
    Gnome();

    virtual bool detectGnome(Partition * part);
};

#endif // GNOME_H
