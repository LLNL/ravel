#ifndef OTFCONVERTER_H
#define OTFCONVERTER_H

#include <QObject>
#include <QString>
#include <QMap>

class RawTrace;
class OTFImporter;
class OTF2Importer;
class OTFImportOptions;
class Trace;
class Partition;
class CommEvent;
class CounterRecord;

// Uses the raw records read from the OTF:
// - switches point events into durational events
// - builds call tree
// - matches messages to durational events
class OTFConverter : public QObject
{
    Q_OBJECT
public:
    OTFConverter();
    ~OTFConverter();
    Trace * importOTF(QString filename, OTFImportOptions * _options);
    Trace * importOTF2(QString filename, OTFImportOptions * _options);

signals:
    void finishRead();
    void matchingUpdate(int, QString);

private:
    void convert();
    void matchEvents();
    void matchEventsSaved();
    void makeSingletonPartition(CommEvent * evt);
    void mergeForWaitall(QList<QList<Partition * > *> * groups);
    int advanceCounters(CommEvent * evt, QStack<CounterRecord *> * counterstack,
                        QVector<CounterRecord *> * counters, int index,
                        QMap<unsigned int, CounterRecord *> * lastcounters);

    RawTrace * rawtrace;
    Trace * trace;
    OTFImportOptions * options;
    int phaseFunction;

    static const int event_match_portion = 24;
    static const int message_match_portion = 0;
    static const QString collectives_string;

};

#endif // OTFCONVERTER_H
