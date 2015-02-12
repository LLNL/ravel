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
      isendCoalescing(true),
      enforceMessageSizes(false),
      seedClusters(false),
      clusterSeed(0),
      advancedStepping(true),
      partitionFunction(_fxn),
      origin(OF_NONE)
{
}

QList<QString> OTFImportOptions::getOptionNames()
{
    QList<QString> names = QList<QString>();
    names.append("option_waitallMerge");
    names.append("option_callerMerge");
    names.append("option_leapMerge");
    names.append("option_leapSkip");
    names.append("option_partitionByFunction");
    names.append("option_globalMerge");
    names.append("option_cluster");
    names.append("option_isendCoalescing");
    names.append("option_enforceMessageSizes");
    names.append("option_partitionFunction");
    names.append("option_seedClusters");
    names.append("option.clusterSeed");
    names.append("option.advancedStepping");
    return names;
}

QString OTFImportOptions::getOptionValue(QString option)
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
    else if (option == "option_seedClusters")
        return seedClusters ? "true" : "";
    else if (option == "option_clusterSeed")
        return QString::number(clusterSeed);
    else if (option == "option.advancedStepping")
        return advancedStepping ? "true" : "";
    else
        return "";
}

void OTFImportOptions::setOption(QString option, QString value)
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
    else if (option == "option_seedClusters")
        seedClusters = value.size();
    else if (option == "option_clusterSeed")
        clusterSeed = value.toLong();
    else if (option == "option_advancedStepping")
        advancedStepping = value.size();
}
