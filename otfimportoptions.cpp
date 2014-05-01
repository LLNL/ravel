#include "otfimportoptions.h"

OTFImportOptions::OTFImportOptions(bool _waitall, bool _leap, bool _skip,
                                   bool _partition, QString _fxn)
    : waitallMerge(_waitall),
      leapMerge(_leap),
      leapSkip(_skip),
      partitionByFunction(_partition),
      globalMerge(true),
      cluster(true),
      partitionFunction(_fxn)
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
    partitionFunction = copy.partitionFunction;
}
