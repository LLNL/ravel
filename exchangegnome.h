#ifndef EXCHANGEGNOME_H
#define EXCHANGEGNOME_H

#include "gnome.h"

class ExchangeGnome : public Gnome
{
public:
    ExchangeGnome();

    bool detectGnome(Partition * part);
};

#endif // EXCHANGEGNOME_H
