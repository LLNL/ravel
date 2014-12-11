//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Ravel.
// Written by Kate Isaacs, kisaacs@acm.org, All rights reserved.
// LLNL-CODE-663885
//
// For details, see https://scalability-llnl.github.io/ravel
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
#ifndef VISOPTIONS_H
#define VISOPTIONS_H

#include <QString>

class ColorMap;

class VisOptions
{
public:
    VisOptions(bool _showAgg = true,
               bool _metricTraditional = true,
               QString _metric = "Lateness");
    VisOptions(const VisOptions& copy);
    void setRange(double low, double high);

    enum ColorMapType { COLOR_SEQUENTIAL, COLOR_DIVERGING, COLOR_CATEGORICAL };
    enum MessageType { MSG_NONE, MSG_TRUE, MSG_SINGLE };

    bool absoluteTime;
    bool showAggregateSteps;
    bool colorTraditionalByMetric; // color physical timeline by metric
    MessageType showMessages; // draw message lines
    bool topByCentroid; // focus processes are centroid of cluster
    bool showInactiveSteps; // default: no color for inactive proportion
    QString metric;
    ColorMapType maptype;
    ColorMap * divergentmap;
    ColorMap * rampmap; // sequential colormap
    ColorMap * catcolormap; // categorical colormap
    ColorMap * colormap; // active colormap
};

#endif // VISOPTIONS_H
