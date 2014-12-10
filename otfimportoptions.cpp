#include "otfimportoptions.h"

OTFImportOptions::OTFImportOptions(bool _waitall, bool _leap, bool _skip,
                                   bool _partition, QString _fxn)
    : waitallMerge(_waitall),
      callerMerge(true),
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
    callerMerge = copy.callerMerge;
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
