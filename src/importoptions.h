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
#ifndef IMPORTOPTIONS_H
#define IMPORTOPTIONS_H

#include <QString>
#include <QList>
#include <QSettings>

// Container for all the structure extraction options
class ImportOptions
{
public:
    ImportOptions(bool _waitall = true,
                     bool _leap = false, bool _skip = false,
                     bool _partition = false, QString _fxn = "");

    // Annoying stuff for cramming into OTF2 format
    QList<QString> getOptionNames();
    QString getOptionValue(QString option);
    void setOption(QString option, QString value);
    void saveSettings(QSettings * settings);
    void readSettings(QSettings * settings);

    enum OriginFormat { OF_NONE, OF_SAVE_OTF2, OF_OTF2, OF_OTF, OF_CHARM };

    bool waitallMerge; // use waitall heuristic
    bool callerMerge; // merge for common callers
    bool leapMerge; // merge to complete leaps
        bool leapSkip; // but skip if you can't gain processes
    bool partitionByFunction; // partitions based on functions
    bool globalMerge; // merge across steps
    bool cluster; // clustering on gnomes should be done
    bool isendCoalescing; // group consecutive isends
    bool enforceMessageSizes; // send/recv size must match

    bool seedClusters; // seed has been set
    long clusterSeed; // random seed for clustering

    bool advancedStepping; // send structure over receives
    bool reorderReceives; // idealized receive order;

    OriginFormat origin;
    QString partitionFunction;
    QString breakFunctions;

};

#endif // OTFIMPORTOPTIONS_H
