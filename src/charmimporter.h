#ifndef CHARMIMPORTER_H
#define CHARMIMPORTER_H

#include <QString>
#include <QMap>
#include <QSet>
#include <QLinkedList>
#include <iostream>
#include <QStack>

class Trace;
class Task;
class TaskGroup;
class Function;
class OTFImportOptions;
class Message;
class Event;
class P2PEvent;
class CommEvent;
class PrimaryTaskGroup;

class Message;

class CharmImporter
{
public:
    CharmImporter();
    ~CharmImporter();
    void importCharmLog(QString filename, OTFImportOptions *_options);
    // RawTrace * getRawTrace() { return rawtrace; }
    Trace * getTrace() { return trace; }

    class ChareIndex {
    public:
        ChareIndex(int c, int i0, int i1, int i2, int i3, int a = 0)
            : chare(c), array(a)
        {
            index[0] = i0;
            index[1] = i1;
            index[2] = i2;
            index[3] = i3;
        }

        int index[4];
        int chare;
        int array;

        ChareIndex& operator=(const ChareIndex & other)
        {
            if (this != &other)
            {
                chare = other.chare;
                array = other.array;
                for (int i = 0; i < 4; i++)
                    index[i] = other.index[i];
            }
            return *this;
        }

        bool operator<(const ChareIndex & other) const
        {
            if (chare < other.chare)
                return true;
            else if (chare > other.chare)
                return false;

            if (array < other.array)
                return true;
            else if (array > other.array)
                return false;

            for (int i = 3; i >= 0; i--)
            {
                if (index[i] < other.index[i])
                {
                    return true;
                }
                else if (index[i] > other.index[i])
                {
                    return false;
                }
            }
            return false;
        }

        bool operator>(const ChareIndex & other) const
        {

            if (chare > other.chare)
                return true;
            else if (chare < other.chare)
                return false;


            if (array > other.array)
                return true;
            else if (array < other.array)
                return false;

            for (int i = 3; i >= 0; i--)
            {
                if (index[i] > other.index[i])
                {
                    return true;
                }
                else if (index[i] < other.index[i])
                {
                    return false;
                }
            }
            return false;
        }

        bool operator<=(const ChareIndex & other) const
        {
            if (chare < other.chare)
                return true;
            else if (chare != other.chare)
                return false;

            if (array < other.array)
                return true;
            else if (array != other.array)
                return false;

            for (int i = 3; i >= 0; i--)
                if (index[i] < other.index[i])
                    return true;
                else if (index[i] != other.index[i])
                    return false;
            return false;
        }

        bool operator>=(const ChareIndex & other) const
        {
            if (chare > other.chare)
                return true;
            else if (chare != other.chare)
                return false;

            if (array > other.array)
                return true;
            else if (array != other.array)
                return false;

            for (int i = 3; i >= 0; i--)
                if (index[i] > other.index[i])
                    return true;
                else if (index[i] != other.index[i])
                    return false;
            return false;
        }

        bool operator==(const ChareIndex & other) const
        {
            if (chare != other.chare)
                return false;
            if (array != other.array)
                return false;

            for (int i = 0; i < 4; i++)
                if (index[i] != other.index[i])
                    return false;

            return true;
        }

        void setIndex(int i0, int i1, int i2, int i3)
        {
            index[0] = i0;
            index[1] = i1;
            index[2] = i2;
            index[3] = i3;
        }

        QString toString() const
        {
            QString str = "";
            str += QString::number(index[0]);
            for (int i = 1; i <= 3; i++)
                str += "." + QString::number(index[i]);
            return str;
        }

        QString toVerboseString() const
        {
            QString str = "";
            str += QString::number(chare) + ".";
            str += QString::number(array) + ".";
            for (int i = 0; i < 3; i++)
                str += QString::number(index[i]) + ".";
            str += QString::number(index[3]);
            return str;
        }
    };

private:
    class CharmEvt;

    void readSts(QString dataFileName);
    void readLog(QString logFileName, bool gzipped, int pe);
    void parseLine(QString line, int my_pe);
    void processDefinitions();
    int makeTasks();
    void makeTaskEvents();
    int makeTaskEventsPop(QStack<CharmEvt *> * stack, CharmEvt * bgn,
                          long endtime, int phase, int depth);

    void chargeIdleness();

    void makePartition(QList<P2PEvent *> *events);
    void buildPartitions();

    void cleanUp();

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
            : name(_name), indices(new QSet<ChareIndex>()) {}
        ~Chare() { delete indices; }

        QString name;
        QSet<ChareIndex> * indices;
    };

    class ChareArray {
    public:
        ChareArray(int _id, int _chare)
            : id(_id), chare(_chare),
              indices(new QSet<ChareIndex>()) {}

        int id;
        int chare;
        QSet<ChareIndex> * indices;
    };

    class ChareGroup {
    public:
        ChareGroup(int _chare)
            : chare(_chare),
              first_task(-1),
              pes(QSet<int>()) {}

        int chare;
        int first_task;
        QSet<int> pes;
    };

    class CharmMsg {
    public:
        CharmMsg(int _mtype, long _mlen, int _pe, int _entry, int _event, int _mype)
            : sendtime(0), recvtime(0), msg_type(_mtype), msg_len(_mlen),
              send_pe(_pe), entry(_entry), event(_event), arrayid(0), recv_pe(_mype),
              send_task(-1), recv_task(-1), send_evt(NULL), recv_evt(NULL),
              tracemsg(NULL) {}

        unsigned long long sendtime;
        unsigned long long recvtime;
        int msg_type;
        long msg_len;
        int send_pe; // send pe
        int entry;
        int event;
        int arrayid;
        int recv_pe; // recv pe
        int send_task;
        int recv_task;

        CharmEvt * send_evt;
        CharmEvt * recv_evt;
        Message * tracemsg;
    };


    class CharmEvt {
    public:
        CharmEvt(int _entry, unsigned long long _time, int _pe, int _chare,
                 int _array, bool _enter = true)
            : time(_time), pe(_pe), task(-1), enter(_enter),
              chare(_chare), index(ChareIndex(-1, 0,0,0,0)),
              arrayid(_array), entry(_entry),
              charmmsgs(new QList<CharmMsg *>()), children(new QList<Event *>()),
              trace_evt(NULL)
        { }
        ~CharmEvt()
        {
            // Messages deleted in CharmImporter from messages QList
            delete charmmsgs;

            // These events get saved elsewhere.
            delete children;
        }

        // If the time is the same, use the entry... send/recv are very high
        // and thus will sort afterwards.
        bool operator<(const CharmEvt & event)
        {
            if (time == event.time)
                return entry < event.entry;
            return time < event.time;
        }
        bool operator>(const CharmEvt & event)
        {
            if (time == event.time)
                return entry > event.entry;
            return time > event.time;
        }
        bool operator<=(const CharmEvt & event)
        {
            if (time == event.time)
                return entry <= event.entry;
            return time <= event.time;
        }
        bool operator>=(const CharmEvt & event)
        {
            if (time == event.time)
                return entry >= event.entry;
            return time >= event.time;
        }
        bool operator==(const CharmEvt & event)
        {
            return time == event.time && entry == event.entry;
        }

        unsigned long long time;
        int pe;
        int task;
        bool enter;

        int chare;
        ChareIndex index;
        int arrayid;
        int entry;

       QList<CharmMsg *> * charmmsgs;

       QList<Event *> * children;

       P2PEvent * trace_evt;

    };

    bool matchingMessages(CharmMsg * send, CharmMsg * recv);

    QMap<int, Chare *> * chares;
    QMap<int, Entry *> * entries;

    OTFImportOptions * options;

    float version;
    int processes;
    bool hasPAPI;
    int numPAPI;
    int main;
    int forravel;
    int traceChare;
    int reductionChare;
    int ckArrayChare;
    int addContribution;
    int recvMsg;
    int contribute;
    long traceEnd;
    int num_application_tasks;

    Trace * trace;

    QVector<QMap<int, QList<CharmMsg *> *> *> * unmatched_recvs; //[other pe][event]
    QVector<QMap<int, QList<CharmMsg *> *> *> * sends;
    QStack<CharmEvt *> * charm_stack;
    QVector<QVector<CharmEvt *> *> * charm_events;
    QVector<QVector<CharmEvt *> *> * task_events;
    QVector<QVector<Event *> *> * pe_events;
    QVector<QVector<P2PEvent *> *> * charm_p2ps;
    QVector<QVector<P2PEvent *> *> * pe_p2ps;
    QMap<Event *, int> * idle_to_next;
    QVector<CharmMsg *> * messages;
    QMap<int, PrimaryTaskGroup *> * primaries;
    QMap<int, TaskGroup *> * taskgroups;
    QMap<int, QString> * functiongroups;
    QMap<int, Function *> * functions;
    QMap<int, ChareArray *> * arrays;
    QMap<int, ChareGroup *> * groups;
    QMap<ChareIndex, int> * chare_to_task;
    QStack<CharmEvt *> last;

    QList<Event *> idles;

    QSet<QString> seen_chares;
    QSet<int> application_chares;

    static const int IDLE_FXN = 999997;
    static const int SEND_FXN = 999998;
    static const int RECV_FXN = 999999;

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

    static const bool verbose = true;

};

inline uint qHash(const CharmImporter::ChareIndex& key)
{
    uint myhash = qHash(key.chare) ^ qHash(key.array)
                  ^ qHash(key.index[3]) ^ qHash(key.index[2])
                  ^ qHash(key.index[1]) ^ qHash(key.index[0]);
    return myhash;
}

#endif // CHARMIMPORTER_H
