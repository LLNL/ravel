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
#include "otfimportoptions.h"

OTFImportOptions::OTFImportOptions(bool _waitall, bool _leap, bool _skip,
                                   bool _partition, QString _fxn)
    : waitallMerge(_waitall),
      leapMerge(_leap),
      leapSkip(_skip),
      partitionByFunction(_partition),
      globalMerge(true),
      cluster(true),
      isendCoalescing(false),
      enforceMessageSizes(false),
      seedClusters(false),
      clusterSeed(0),
      advancedStepping(true),
      partitionFunction(_fxn),
      origin(OF_NONE)
{
}

OTFImportOptions::OTFImportOptions(const OTFImportOptions& copy)
{
    waitallMerge = copy.waitallMerge;
    leapMerge = copy.leapMerge;
    leapSkip = copy.leapSkip;
    partitionByFunction = copy.partitionByFunction;
    globalMerge = copy.globalMerge;
    cluster = copy.cluster;
    isendCoalescing = copy.isendCoalescing;
    seedClusters = copy.seedClusters;
    clusterSeed = copy.clusterSeed;
    advancedStepping = copy.advancedStepping;
    enforceMessageSizes = copy.enforceMessageSizes;
    partitionFunction = copy.partitionFunction;
}
