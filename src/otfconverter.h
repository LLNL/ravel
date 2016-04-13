#ifndef OTFCONVERTER_H
#define OTFCONVERTER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QStack>

class RawTrace;
class OTFImporter;
class OTF2Importer;
class ImportOptions;
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

    Trace * importOTF(QString filename, ImportOptions * _options);
    Trace * importOTF2(QString filename, ImportOptions * _options);
    Trace * importCharm(RawTrace *, ImportOptions * _options);

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
    ImportOptions * options;
    int phaseFunction;

    static const int event_match_portion = 24;
    static const int message_match_portion = 0;
    static const QString collectives_string;

};

#endif // OTFCONVERTER_H
