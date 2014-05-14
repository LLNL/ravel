#ifndef OTFIMPORTOPTIONS_H
#define OTFIMPORTOPTIONS_H

#include <QString>

// Container for all the structure extraction options
class OTFImportOptions
{
public:
    OTFImportOptions(bool _waitall = true,
                     bool _leap = false, bool _skip = false,
                     bool _partition = false, QString _fxn = "");
    OTFImportOptions(const OTFImportOptions& copy);

    bool waitallMerge; // use waitall heuristic
    bool leapMerge; // merge to complete leaps
        bool leapSkip; // but skip if you can't gain processes
    bool partitionByFunction; // partitions based on functions
    bool globalMerge; // merge across steps
    bool cluster; // clustering on gnomes should be done
    bool isendCoalescing; // group consecutive isends
    QString partitionFunction;
};

#endif // OTFIMPORTOPTIONS_H
