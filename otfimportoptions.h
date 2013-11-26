#ifndef OTFIMPORTOPTIONS_H
#define OTFIMPORTOPTIONS_H

#include <QString>

class OTFImportOptions
{
public:
    OTFImportOptions(bool _waitall = true,
                     bool _leap = false, bool _skip = false,
                     bool _partition = false, QString _fxn = "");
    OTFImportOptions(const OTFImportOptions& copy);

    bool waitallMerge;
    bool leapMerge;
    bool leapSkip;
    bool partitionByFunction;
    QString partitionFunction;
};

#endif // OTFIMPORTOPTIONS_H
