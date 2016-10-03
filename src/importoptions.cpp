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
#include "importoptions.h"

ImportOptions::ImportOptions(bool _waitall, bool _leap, bool _skip,
                             bool _partition, QString _fxn)
    : waitallMerge(_waitall),
      callerMerge(true),
      leapMerge(_leap),
      leapSkip(_skip),
      partitionByFunction(_partition),
      globalMerge(false),
      cluster(false),
      isendCoalescing(true),
      enforceMessageSizes(false),
      seedClusters(false),
      clusterSeed(0),
      advancedStepping(true),
      reorderReceives(false),
      origin(OF_NONE),
      partitionFunction(_fxn),
      breakFunctions("")
{
}

void ImportOptions::saveSettings(QSettings * settings)
{
    settings->beginGroup("Import");
    settings->setValue("waitallMerge", waitallMerge);
    settings->setValue("callerMerge", callerMerge);
    settings->setValue("leapMerge", leapMerge);
    settings->setValue("leapSkip", leapSkip);
    settings->setValue("partitionbyFunction", partitionByFunction);
    settings->setValue("globalMerge", globalMerge);
    settings->setValue("cluster", cluster);
    settings->setValue("isendCoalescing", isendCoalescing);
    settings->setValue("enforceMessageSizes", enforceMessageSizes);
    settings->setValue("partitionFunction", partitionFunction);
    settings->setValue("seedClusters", seedClusters);
    settings->setValue("clusterSeed", qlonglong(clusterSeed));
    settings->setValue("advancedStepping", advancedStepping);
    settings->setValue("reorderReceives", reorderReceives);
    settings->setValue("isset", true);
    settings->endGroup();
}

void ImportOptions::readSettings(QSettings * settings)
{
    settings->beginGroup("Import");
    bool check = settings->value("isset").toBool();
    if (check)
    {
        waitallMerge = settings->value("waitallMerge").toBool();
        callerMerge = settings->value("callerMerge").toBool();
        leapMerge = settings->value("leapMerge").toBool();
        leapSkip = settings->value("leapSkip").toBool();
        partitionByFunction = settings->value("partitionbyFunction").toBool();
        globalMerge = settings->value("globalMerge").toBool();
        cluster = settings->value("cluster").toBool();
        isendCoalescing = settings->value("isendCoalescing").toBool();
        enforceMessageSizes = settings->value("enforceMessageSizes").toBool();
        partitionFunction = settings->value("partitionFunction").toString();
        seedClusters = settings->value("seedClusters").toBool();
        clusterSeed = settings->value("clusterSeed").toInt();
        advancedStepping = settings->value("advancedStepping").toBool();
        reorderReceives = settings->value("reorderReceives").toBool();
    }
    settings->endGroup();
}

QList<QString> ImportOptions::getOptionNames()
{
    QList<QString> names = QList<QString>();
    names.append("option_waitallMerge");
    names.append("option_callerMerge");
    names.append("option_leapMerge");
    names.append("option_leapSkip");
    names.append("option_partitionByFunction");
    names.append("option_breakFunctions");
    names.append("option_globalMerge");
    names.append("option_cluster");
    names.append("option_isendCoalescing");
    names.append("option_enforceMessageSizes");
    names.append("option_partitionFunction");
    names.append("option_seedClusters");
    names.append("option.clusterSeed");
    names.append("option.advancedStepping");
    names.append("option.reorderReceives");
    return names;
}

QString ImportOptions::getOptionValue(QString option)
{
    if (option == "option_waitallMerge")
        return waitallMerge ? "true" : "";
    else if (option == "option_callerMerge")
        return callerMerge ? "true" : "";
    else if (option == "option_leapMerge")
        return leapMerge ? "true" : "";
    else if (option == "option_leapSkip")
        return leapSkip ? "true" : "";
    else if (option == "option_partitionByFunction")
        return partitionByFunction ? "true" : "";
    else if (option == "option_globalMerge")
        return globalMerge ? "true" : "";
    else if (option == "option_cluster")
        return cluster ? "true" : "";
    else if (option == "option_isendCoalescing")
        return isendCoalescing ? "true" : "";
    else if (option == "option_enforceMessageSizes")
        return enforceMessageSizes ? "true" : "";
    else if (option == "option_partitionFunction")
        return partitionFunction;
    else if (option == "option_breakFunctions")
        return breakFunctions;
    else if (option == "option_seedClusters")
        return seedClusters ? "true" : "";
    else if (option == "option_clusterSeed")
        return QString::number(clusterSeed);
    else if (option == "option.advancedStepping")
        return advancedStepping ? "true" : "";
    else if (option == "option.reorderReceives")
        return reorderReceives ? "true" : "";
    else
        return "";
}

void ImportOptions::setOption(QString option, QString value)
{
    if (option == "option_waitallMerge")
        waitallMerge = value.size();
    else if (option == "option_callerMerge")
        callerMerge = value.size();
    else if (option == "option_leapMerge")
        leapMerge = value.size();
    else if (option == "option_leapSkip")
        leapSkip = value.size();
    else if (option == "option_partitionByFunction")
        partitionByFunction = value.size();
    else if (option == "option_globalMerge")
        globalMerge = value.size();
    else if (option == "option_cluster")
        cluster = value.size();
    else if (option == "option_isendCoalescing")
        isendCoalescing = value.size();
    else if (option == "option_enforceMessageSizes")
        enforceMessageSizes = value.size();
    else if (option == "option_partitionFunction")
        partitionFunction = value;
    else if (option == "option_breakFunctions")
        breakFunctions = value;
    else if (option == "option_seedClusters")
        seedClusters = value.size();
    else if (option == "option_clusterSeed")
        clusterSeed = value.toLong();
    else if (option == "option_advancedStepping")
        advancedStepping = value.size();
    else if (option == "option_reorderReceives")
        reorderReceives = value.size();
}
