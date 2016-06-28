//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Ravel.
// Written by Kate Isaacs, kisaacs@acm.org, All rights reserved.
// LLNL-CODE-663885
//
// For details, see https://github.com/scalability-llnl/ravel
// Please also see the LICENSE file for our notice and the LGPL.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//////////////////////////////////////////////////////////////////////////////
#include "visoptions.h"
#include "colormap.h"

VisOptions::VisOptions(QString _metric)
    : absoluteTime(true),
      showMessages(MSG_TRUE),
      metric(_metric),
      maptype(COLOR_DIVERGING),
      divergentmap(new ColorMap(QColor(173, 216, 230), 0)),
      rampmap(new ColorMap(QColor(255, 247, 222), 0)),
      catcolormap(new ColorMap(QColor(158, 218, 229), 0, true)), // divergent blue
      colormap(divergentmap)
{

    // rampmap from colorbrewer
    rampmap->addColor(QColor(252, 141, 89), 0.5);
    rampmap->addColor(QColor(127, 0, 0), 1);

    divergentmap->addColor(QColor(240, 230, 140), 0.5);
    divergentmap->addColor(QColor(178, 34, 34), 1);

    // Cat colors from d3's category20, reordered to put less saturated first
    catcolormap->addColor(QColor(219, 219, 141), 0.05); // light yellow-green
    catcolormap->addColor(QColor(199, 199, 199), 0.1); // light gray
    catcolormap->addColor(QColor(255, 187, 120), 0.15); // peach
    catcolormap->addColor(QColor(152, 223, 138), 0.2); // light green
    catcolormap->addColor(QColor(196, 156, 148), 0.25); // coffee
    catcolormap->addColor(QColor(247, 182, 210), 0.3); // light pink
    catcolormap->addColor(QColor(148, 192, 189), 0.35); // light blue
    catcolormap->addColor(QColor(197, 176, 213), 0.4); // lavender
    catcolormap->addColor(QColor(140, 86, 75), 0.45); // brown
    catcolormap->addColor(QColor(44, 160, 44), 0.5); // quite green
    catcolormap->addColor(QColor(214, 39, 40), 0.55); // red
    catcolormap->addColor(QColor(227, 119, 194), 0.6); // magenta
    catcolormap->addColor(QColor(255, 152, 150), 0.65); // pink
    catcolormap->addColor(QColor(127, 127, 127), 0.7); // gray
    catcolormap->addColor(QColor(174, 119, 232), 0.75); // purple
    catcolormap->addColor(QColor(188, 189, 34), 0.8);  // chartreuse
    catcolormap->addColor(QColor(255, 127, 14), 0.85); // bright orange
    catcolormap->addColor(QColor(23, 190, 207), 0.9); // electric blue
    catcolormap->addColor(QColor(31, 119, 180), 0.95); // royal blue

}

VisOptions::VisOptions(const VisOptions& copy)
{
    absoluteTime = copy.absoluteTime;
    showMessages = copy.showMessages;
    metric = copy.metric;
    maptype = copy.maptype;
    divergentmap = new ColorMap(*(copy.divergentmap));
    catcolormap = new ColorMap(*(copy.catcolormap));
    rampmap = new ColorMap(*(copy.rampmap));
    if (maptype == COLOR_SEQUENTIAL)
        colormap = rampmap;
    else if (maptype == COLOR_CATEGORICAL)
        colormap = catcolormap;
    else
        colormap = divergentmap;
}


void VisOptions::setRange(double low, double high)
{
    rampmap->setRange(low, high);
    divergentmap->setRange(low, high);
    catcolormap->setRange(low, high);
}
