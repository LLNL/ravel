#ifndef GNOME2_H
#define GNOME2_H

#include "partition.h"

// Gnomes are smaller than Dwarves
class Gnome2
{
public:
    Gnome2();

    virtual bool detectGnome(Partition * part);
};

#endif // GNOME2_H
