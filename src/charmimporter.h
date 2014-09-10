#ifndef CHARMIMPORTER_H
#define CHARMIMPORTER_H

#include <QString>
#include <QMap>
#include <QSet>
#include <zlib.h>
#include <sstream>
#include <fstream>
#include "otfimportoptions.h"
#include "function.h"

class CharmImporter
{
public:
    CharmImporter();
    ~CharmImporter();
    void importCharmLog(QString filename, OTFImportOptions *_options);

private:
    void readSts(QString dataFileName);
    void readLog(QString logFileName, bool gzipped);
    void parseLine(QString line);

    class Entry {
    public:
        Entry(int _chare, QString _name)
            : chare(_chare), name(_name) {}

        int chare;
        QString name;
    };

    class Chare {
    public:
        Chare(QString _name)
            : name(_name), indices(new QSet<QString>()) {}
        ~Chare() { delete indices; }

        QString name;
        QSet<QString> * indices;
    };

    class CharmEvt {
    public:
        CharmEvt(int _type, unsigned long long _start, int _pe)
            : evt_type(_type), enter(_start), pe(_pe),
              chare(-1), chareIndex({0, 0, 0, 0}), entry(-1) {}

        int evt_type;
        unsigned long long enter;
        unsigned long long exit;
        int pe;
        int chare;
        int chareIndex[4];
        int entry;

        QString indexToString()
        {
            QString str = QString::number(chareIndex[0]);
            for (int i = 1; i < 4; i++)
                str += "-" + QString::number(chareIndex[i]);
            return str;
        }
    };

    QMap<int, Chare *> * chares;
    QMap<int, Entry *> * entries;

    float version;
    int processes;

    static const int CREATION = 1;
    static const int BEGIN_PROCESSING = 2;
    static const int END_PROCESSING = 3;
    static const int ENQUEUE = 4;
    static const int DEQUEUE = 5;
    static const int BEGIN_COMPUTATIION = 6;
    static const int END_COMPUTATION = 7;
    static const int BEGIN_INTERRUPT = 8;
    static const int END_INTERRUPT = 9;
    static const int MESSAGE_RECV = 10;
    static const int BEGIN_TRACE = 12;
    static const int END_TRACE = 13;
    static const int USER_EVENT = 13;
    static const int BEGIN_IDLE = 14;
    static const int END_IDLE = 15;
    static const int BEGIN_PACK = 16;
    static const int END_PACK = 17;
    static const int BEGIN_UNPACK = 18;
    static const int END_UNPACK = 19;
    static const int CREATION_BCAST = 20;
    static const int CREATION_MULTICAST = 21;
    static const int BEGIN_FUNC = 22;
    static const int END_FUNC = 23;
    static const int USER_SUPPLIED = 26;
    static const int MEMORY_USAGE = 27;
    static const int USER_SUPPLIED_NOTE = 28;
    static const int USER_SUPPLIED_BRACKETED_NOTE = 29;
    static const int USER_EVENT_PAIR = 100;

    static const int NEW_CHARE_MSG = 0;
    static const int FOR_CHARE_MSG = 2;
    static const int BOC_INIT_MSG = 3;

    static const int LDB_MSG = 12;
    static const int QD_BOC_MSG = 14;
    static const int QD_BROACAST_BOC_MSG = 15;

};

#endif // CHARMIMPORTER_H
