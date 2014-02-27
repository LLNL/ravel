#ifndef EXCHANGEGNOME_H
#define EXCHANGEGNOME_H

#include "gnome.h"

class ExchangeGnome : public Gnome
{
public:
    ExchangeGnome();

    bool detectGnome(Partition * part);
    Gnome * create();
    void preprocess();
    void drawGnomeQt(QPainter * painter, QRect extents);
    void drawGnomeGL(QRect extents);
};

#endif // EXCHANGEGNOME_H
