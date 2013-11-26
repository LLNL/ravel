#include "otfimportoptions.h"

OTFImportOptions::OTFImportOptions(bool _waitall, bool _leap, bool _skip, bool _partition, QString _fxn)
    : waitallMerge(_waitall),
      leapMerge(_leap),
      leapSkip(_skip),
      partitionByFunction(_partition),
      partitionFunction(_fxn)
{
}

OTFImportOptions::OTFImportOptions(const OTFImportOptions& copy)
{
    waitallMerge = copy.waitallMerge;
    leapMerge = copy.leapMerge;
    leapSkip = copy.leapSkip;
    partitionByFunction = copy.partitionByFunction;
    partitionFunction = copy.partitionFunction;
}
