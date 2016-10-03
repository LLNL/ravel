#ifndef IMPORTOPTIONS_H
#define IMPORTOPTIONS_H

#include <QString>
#include <QList>
#include <QSettings>

// Container for all the structure extraction options
class ImportOptions
{
public:
    ImportOptions(bool _waitall = true,
                     bool _leap = false, bool _skip = false,
                     bool _partition = false, QString _fxn = "");

    // Annoying stuff for cramming into OTF2 format
    QList<QString> getOptionNames();
    QString getOptionValue(QString option);
    void setOption(QString option, QString value);
    void saveSettings(QSettings * settings);
    void readSettings(QSettings * settings);

    enum OriginFormat { OF_NONE, OF_SAVE_OTF2, OF_OTF2, OF_OTF, OF_CHARM };

    bool waitallMerge; // use waitall heuristic
    bool callerMerge; // merge for common callers
    bool leapMerge; // merge to complete leaps
        bool leapSkip; // but skip if you can't gain processes
    bool partitionByFunction; // partitions based on functions
    bool globalMerge; // merge across steps
    bool cluster; // clustering on gnomes should be done
    bool isendCoalescing; // group consecutive isends
    bool enforceMessageSizes; // send/recv size must match

    bool seedClusters; // seed has been set
    long clusterSeed; // random seed for clustering

    bool advancedStepping; // send structure over receives
    bool reorderReceives; // idealized receive order;

    OriginFormat origin;
    QString partitionFunction;
    QString breakFunctions;

};

#endif // OTFIMPORTOPTIONS_H
