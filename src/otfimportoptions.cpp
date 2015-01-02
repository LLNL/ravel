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
    enforceMessageSizes = copy.enforceMessageSizes;
    partitionFunction = copy.partitionFunction;
}

QList<QString> OTFImportOptions::getOptionNames()
{
    QList<QString> names = QList<QString>();
    names.append("waitallMerge");
    names.append("leapMerge");
    names.append("leapSkip");
    names.append("partitionByFunction");
    names.append("globalMerge");
    names.append("cluster");
    names.append("isendCoalescing");
    names.append("enforceMessageSizes");
    names.append("partitionFunction");
    return names;
}

QString OTFImportOptions::getOptionValue(QString option)
{
    if (option == "waitallMerge")
        return waitallMerge ? "true" : "";
    else if (option == "leapMerge")
        return leapMerge ? "true" : "";
    else if (option == "leapSkip")
        return leapSkip ? "true" : "";
    else if (option == "partitionByFunction")
        return partitionByFunction ? "true" : "";
    else if (option == "globalMerge")
        return globalMerge ? "true" : "";
    else if (option == "cluster")
        return cluster ? "true" : "";
    else if (option == "isendCoalescing")
        return isendCoalescing ? "true" : "";
    else if (option == "enforceMessageSizes")
        return enforceMessageSizes ? "true" : "";
    else if (option == partitionFunction)
        return partitionFunction;
    else
        return "";
}

void OTFImportOptions::setOption(QString option, QString value)
{
    if (option == "waitallMerge")
        waitallMerge = value.size();
    else if (option == "leapMerge")
        leapMerge = value.size();
    else if (option == "leapSkip")
        leapSkip = value.size();
    else if (option == "partitionByFunction")
        partitionByFunction = value.size();
    else if (option == "globalMerge")
        globalMerge = value.size();
    else if (option == "cluster")
        cluster = value.size();
    else if (option == "isendCoalescing")
        isendCoalescing = value.size();
    else if (option == "enforceMessageSizes")
        enforceMessageSizes = value.size();
    else if (option == partitionFunction)
        partitionFunction = value;
}
