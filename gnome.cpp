#include "gnome.h"

Gnome::Gnome()
    : partition(NULL)
{
}

bool Gnome::detectGnome(Partition * part)
{
    Q_UNUSED(part);
    return false;
}

Gnome * Gnome::create()
{
    return new Gnome();
}
