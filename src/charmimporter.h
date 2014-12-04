#ifndef CHARMIMPORTER_H
#define CHARMIMPORTER_H

#include <QString>
#include <QMap>
#include <QSet>
#include <QLinkedList>
#include <zlib.h>
#include <sstream>
#include <fstream>
#include "otfimportoptions.h"
#include "function.h"

#include "rawtrace.h"

class CharmImporter
{
public:
    CharmImporter();
    ~CharmImporter();
    void importCharmLog(QString filename, OTFImportOptions *_options);
    RawTrace * getRawTrace() { return rawtrace; }

private:
    void readSts(QString dataFileName);
    void readLog(QString logFileName, bool gzipped, int pe);
    void parseLine(QString line, int my_pe);
    void processDefinitions();
    void processMessages();

    class Entry {
    public:
        Entry(int _chare, QString _name, int _msg)
            : chare(_chare), name(_name), msgid(_msg) {}

        int chare;
        QString name;
        int msgid;
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
        CharmEvt(int _type, unsigned long long _start, int _pe,
                 bool _leave = false)
            : evt_type(_type), enter(_start), pe(_pe), leave(_leave),
              chare(-1), entry(-1)
        {
            chareIndex[0] = 0;
            chareIndex[1] = 0;
            chareIndex[2] = 0;
            chareIndex[3] = 0;
        }

        int evt_type;
        unsigned long long enter;
        unsigned long long exit;
        int pe;
        bool leave;

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

    class CharmMsg {
    public:
        CharmMsg(int _mtype, long _mlen, int _pe, int _entry, int _event, int _mype)
            : sendtime(0), recvtime(0), msg_type(_mtype), msg_len(_mlen),
              pe(_pe), entry(_entry), event(_event), my_pe(_mype) {}

        unsigned long long sendtime;
        unsigned long long recvtime;
        int msg_type;
        long msg_len;
        int pe;
        int entry;
        int event;
        int my_pe;
    };

    bool matchingMessages(CharmMsg * send, CharmMsg * recv);

    QMap<int, Chare *> * chares;
    QMap<int, Entry *> * entries;

    float version;
    int processes;

    RawTrace * rawtrace;

    QVector<QLinkedList<CharmMsg *> *> * unmatched_recvs; // map event to recv
    QVector<QLinkedList<CharmMsg *> *> * unmatched_sends;

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
