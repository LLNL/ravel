#ifndef OTFCONVERTER_H
#define OTFCONVERTER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QStack>

class RawTrace;
class OTFImporter;
class OTF2Importer;
class OTFImportOptions;
class Trace;
class Partition;
class CommEvent;
class CounterRecord;
class EventRecord;

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
#ifdef OTF1LIB
    Trace * importOTF(QString filename, OTFImportOptions * _options);
#endif
    Trace * importOTF2(QString filename, OTFImportOptions * _options);

signals:
    void finishRead();
    void matchingUpdate(int, QString);

private:
    void convert();
    void matchEvents();
    void matchEventsSaved();
    void makeSingletonPartition(CommEvent * evt);
    void addToSavedPartition(CommEvent * evt, int partition);
    void handleSavedAttributes(CommEvent * evt, EventRecord *er);
    void mergeContiguous(QList<QList<Partition * > *> * groups);
    void mergeByMultiCaller();
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
