#include "otfimportoptions.h"

OTFImportOptions::OTFImportOptions(bool _waitall, bool _leap, bool _skip, bool _partition, QString _fxn)
    : waitallMerge(_waitall),
      leapMerge(_leap),
      leapSkip(_skip),
      leapCollective(true),
      partitionByFunction(_partition),
      globalMerge(true),
      partitionFunction(_fxn)
{
}

OTFImportOptions::OTFImportOptions(const OTFImportOptions& copy)
{
    waitallMerge = copy.waitallMerge;
    leapMerge = copy.leapMerge;
    leapSkip = copy.leapSkip;
    leapCollective = copy.leapCollective;
    partitionByFunction = copy.partitionByFunction;
    globalMerge = copy.globalMerge;
    partitionFunction = copy.partitionFunction;
}
