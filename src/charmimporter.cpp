#include "charmimporter.h"
#include "eventrecord.h"
#include "commrecord.h"
#include <iostream>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QStack>

#include "general_util.h"

CharmImporter::CharmImporter()
    : version(0),
      processes(0),
      rawtrace(NULL),
      unmatched_recvs(new QVector<QLinkedList<CharmMsg *> *>()),
      unmatched_sends(new QVector<QLinkedList<CharmMsg *> *>()),
      chares(new QMap<int, Chare*>()),
      entries(new QMap<int, Entry *>())
{
}

CharmImporter::~CharmImporter()
{
    delete chares;
    delete entries; // May need to delete each Entry
}

void CharmImporter::importCharmLog(QString dataFileName, OTFImportOptions * _options)
{
    readSts(dataFileName);

    rawtrace = new RawTrace(processes);

    // Most of this we won't use
    rawtrace = new RawTrace(processes);
    rawtrace->second_magnitude = 9;
    rawtrace->functions = new QMap<int, Function *>();
    rawtrace->functionGroups = new QMap<int, QString>();
    rawtrace->collective_definitions = new QMap<int, OTFCollective *>();
    rawtrace->collectives = new QMap<unsigned long long, CollectiveRecord *>();
    rawtrace->counters = new QMap<unsigned int, Counter *>();
    rawtrace->events = new QVector<QVector<EventRecord *> *>(processes);
    rawtrace->messages = new QVector<QVector<CommRecord *> *>(processes);
    rawtrace->messages_r = new QVector<QVector<CommRecord *> *>(processes);
    rawtrace->counter_records = new QVector<QVector<CounterRecord *> *>(processes);
    rawtrace->collectiveMap = new QVector<QMap<unsigned long long, CollectiveRecord *> *>(processes);

    delete unmatched_recvs;
    unmatched_recvs = new QVector<QLinkedList<CharmMsg *> *>(processes);
    delete unmatched_sends;
    unmatched_sends = new QVector<QLinkedList<CharmMsg *> *>(processes);
    for (int i = 0; i < processes; i++) {
        (*unmatched_recvs)[i] = new QLinkedList<CharmMsg *>();
        (*unmatched_sends)[i] = new QLinkedList<CharmMsg *>();
        (*(rawtrace->collectiveMap))[i] = new QMap<unsigned long long, CollectiveRecord *>();
        (*(rawtrace->events))[i] = new QVector<EventRecord *>();
        (*(rawtrace->messages))[i] = new QVector<CommRecord *>();
        (*(rawtrace->messages_r))[i] = new QVector<CommRecord *>();
        (*(rawtrace->counter_records))[i] = new QVector<CounterRecord *>();
    }

    processDefinitions();

    // Read individual log files
    QFileInfo file_info = QFileInfo(dataFileName);
    QDir directory = file_info.dir();
    QString basename = file_info.baseName();
    QString suffix = ".log";
    QString path = file_info.absolutePath();
    bool gzflag = false;
    if (directory.exists(basename + ".0.log.gz"))
    {
        gzflag = true;
        suffix += ".gz";
    }
    for (int i = 0; i < processes; i++)
        readLog(path + "/" + basename + "." + QString::number(i) + suffix,
                gzflag, i);

    // Message  mapping
    processMessages();

}

void CharmImporter::readLog(QString logFileName, bool gzipped, int pe)
{
    if (gzipped)
    {
        gzFile logfile = gzopen(logFileName.toStdString().c_str(), "r");
        char * buffer = new char[1024];
        gzgets(logfile, buffer, 1024); // Skip first line
        while (gzgets(logfile, buffer, 1024))
            parseLine(QString::fromUtf8(buffer).simplified(), pe);

        gzclose(logfile);
    }
    else
    {
        std::ifstream logfile(logFileName.toStdString().c_str());
        std::string line;
        std::getline(logfile, line); // Skip first line
        while(std::getline(logfile, line))
            parseLine(QString::fromStdString(line), pe);

        logfile.close();
    }
}

void CharmImporter::parseLine(QString line, int my_pe)
{
    int index, mtype, entry, event, pe, numpes, id[4];
    long time, msglen, sendTime, recvTime, cpuStart, cpuEnd;
    QStack<CharmEvt *> * events_stack = new QStack<CharmEvt *>();
    QVector<CharmEvt *> * comm_events = new QVector<CharmEvt *>();
    QVector<CharmEvt *> * events = new QVector<CharmEvt *>();
    QLinkedList<CharmMsg *> * sends = new QLinkedList<CharmMsg *>();
    QLinkedList<CharmMsg *> * receives = new QLinkedList<CharmMsg *>();

    QStringList lineList = line.split(" ");
    int rectype = lineList.at(0).toInt();
    if (rectype == CREATION || rectype == CREATION_BCAST)
    {
        // Some type of (multi-send)
        mtype = lineList.at(1).toInt();
        entry = lineList.at(2).toInt();
        time = lineList.at(3).toLong();
        event = lineList.at(4).toInt();
        pe = lineList.at(5).toInt();
        msglen = -1;
        index = 6;
        if (version >= 2.0)
        {
            msglen = lineList.at(5).toLong();
            index++;
        }
        if (version >= 5.0)
        {
            sendTime = lineList.at(index).toLong();
            index++;
        }
        if (rectype == CREATION_BCAST)
        {
            numpes = lineList.at(index).toInt();
        }

        CharmEvt * evt = new CharmEvt(CREATION, time, pe, false);
        evt->chare = entries->value(entry)->chare;
        evt->entry = entry;
        events_stack->push(evt);

        evt = new CharmEvt(CREATION, time, pe, true);
        evt->chare = entries->value(entry)->chare;
        evt->entry = entry;
        events_stack->push(evt);

        if (rectype == CREATION)
        {
            CharmMsg * msg = new CharmMsg(mtype, msglen, pe, entry, event, my_pe);
            msg->sendtime = time;
            sends->append(msg);
            unmatched_sends->at(pe)->append(msg);
            std::cout << "Send on " << pe << " of " << entries->value(entry)->name.toStdString().c_str() << std::endl;
            std::cout << "     Event: " << event << ", msg_type: " << mtype << std::endl;
        }
        else
        {
            /*for (int i = 0; i < numpes; i++)
            {
                CharmMsg * msg = new CharmMsg(mtype, msglen, pe, entry, event, my_pe);
                msg->sendtime = time;
                sends->append(msg);
                unmatched_sends->at(pe)->append(msg);
            }*/
            std::cout << "Bcast on " << pe << " of " << entries->value(entry)->name.toStdString().c_str() << std::endl;
            std::cout << "     Event: " << event << ", msg_type: " << mtype << std::endl;

        }

        rawtrace->events->at(my_pe)->append(new EventRecord(my_pe,
                                                            time,
                                                            999998,
                                                            true));

        rawtrace->events->at(my_pe)->append(new EventRecord(my_pe,
                                                            time,
                                                            999998,
                                                            false));
    }
    else if (rectype == BEGIN_PROCESSING)
    {
        // A receive immediately followed by a function
        mtype = lineList.at(1).toInt();
        entry = lineList.at(2).toInt();
        time = lineList.at(3).toLong();
        event = lineList.at(4).toInt();
        pe = lineList.at(5).toInt();
        index = 6;
        msglen = -1;
        if (version >= 2.0)
        {
            msglen = lineList.at(index).toLong();
            index++;
        }
        if (version >= 4.0)
        {
            recvTime = lineList.at(index).toLong();
            index++;
            for (int i = 0; i < 3; i++)
            {
                id[i] = lineList.at(index).toInt();
                index++;
            }
        }
        if (version >= 7.0)
        {
            id[3] = lineList.at(index).toInt();
            index++;
        }
        if (version >= 6.5)
        {
            cpuStart = lineList.at(index).toLong();
            index++;
        }
        if (version >= 6.6)
        {
            // PerfCount stuff... skip for now.
        }

        CharmEvt * evt = new CharmEvt(BEGIN_PROCESSING, time, pe);
        evt->chare = entries->value(entry)->chare;
        evt->entry = entry;
        for (int i = 0; i < 4; i++)
            evt->chareIndex[i] = id[i];
        events_stack->push(evt);
        chares->value(evt->chare)->indices->insert(evt->indexToString());


        // Real Event
        rawtrace->events->at(my_pe)->append(new EventRecord(my_pe,
                                                            time,
                                                            entry,
                                                            true));

        // RECV
        rawtrace->events->at(my_pe)->append(new EventRecord(my_pe,
                                                            time,
                                                            999999,
                                                            true));
        rawtrace->events->at(my_pe)->append(new EventRecord(my_pe,
                                                            time,
                                                            999999,
                                                            false));

        CharmMsg * msg = new CharmMsg(mtype, msglen, pe, entry, event, my_pe);
        msg->recvtime = time;
        receives->append(msg);
        unmatched_recvs->at(my_pe)->append(msg);
    }
    else if (rectype == END_PROCESSING)
    {
        // End of function
        mtype = lineList.at(1).toInt();
        entry = lineList.at(2).toInt();
        time = lineList.at(3).toLong();
        event = lineList.at(4).toInt();
        pe = lineList.at(5).toInt();
        index = 6;
        msglen = -1;
        if (version >= 2.0)
        {
            msglen = lineList.at(index).toLong();
            index++;
        }
        if (version >= 6.5)
        {
            cpuEnd = lineList.at(index).toLong();
            index++;
        }
        if (version >= 6.6)
        {
            // PerfCount stuff... skip for now.
        }

        CharmEvt * evt = new CharmEvt(BEGIN_PROCESSING, time, pe, true);
        evt->chare = entries->value(entry)->chare;
        evt->entry = entry;
        events_stack->push(evt);

        rawtrace->events->at(my_pe)->append(new EventRecord(my_pe,
                                                            time,
                                                            entry,
                                                            false));
    }
    else if (rectype == MESSAGE_RECV) // just in case we ever find one
    {
        std::cout << "Message Recv!" << std::endl;
    }


}


// Leak a ton of memory for now
void CharmImporter::processMessages()
{
    CharmMsg * match = NULL;
    QLinkedList<CharmMsg *> * sends = NULL;
    QLinkedList<CharmMsg *> * recvs = NULL;

    // Receives
    for (int i = 0; i < processes; i++)
    {
        recvs = unmatched_recvs->at(i);

        for (QLinkedList<CharmMsg *>::Iterator recv = recvs->begin();
             recv != recvs->end(); ++recv)
        {
            match = NULL;
            sends = unmatched_sends->at((*recv)->pe);
            for (QLinkedList<CharmMsg *>::Iterator send = sends->begin();
                 send != sends->end(); ++send)
            {
                if (matchingMessages(*send, *recv))
                {
                    match = *send;
                    break;
                }
            }

            if (match && match->my_pe != (*recv)->my_pe) // Skip on pe for now -- Ravel bug for single process partition/message
            {
                CommRecord * cr = new CommRecord(match->pe,
                                                 match->sendtime,
                                                 (*recv)->my_pe,
                                                 (*recv)->recvtime,
                                                 match->msg_len,
                                                 match->event);
                rawtrace->messages_r->at(i)->append(cr);
                rawtrace->messages->at(match->my_pe)->append(cr);// Sort later

            }
            else
            {
                std::cout << "No match for receive on " << i << " from " << (*recv)->pe << " with entry ";
                std::cout << entries->value((*recv)->entry)->name.toStdString().c_str() << std::endl;
                std::cout << "     Event: " << (*recv)->event << ", msg_type: " << (*recv)->msg_type << std::endl;
            }
        }
    }

    for (int i = 0; i < processes; i++)
    {
        qSort(rawtrace->messages->at(i)->begin(),
              rawtrace->messages->at(i)->end(),
              dereferencedLessThan<CommRecord>);
    }

}

bool CharmImporter::matchingMessages(CharmMsg * send, CharmMsg * recv)
{
    if (send->entry == recv->entry && send->event == recv->event
            && send->pe == recv->pe) //&& send->msg_len == recv->msg_len
            //&& send->msg_type == recv->msg_type)
        return true;
    return false;
}

// STS is never gzipped
void CharmImporter::readSts(QString dataFileName)
{
    std::ifstream stsfile(dataFileName.toStdString().c_str());
    std::string line;

    while (std::getline(stsfile, line))
    {
        QStringList lineList = QString::fromStdString(line).split(" ");
        if (lineList.at(0) == "ENTRY" && lineList.at(1) == "CHARE")
        {
            int len = lineList.length();
            QString name = lineList.at(3);
            for (int i = 4; i < len - 2; i++)
                name += " " + lineList.at(i);
            entries->insert(lineList.at(2).toInt(),
                            new Entry(lineList.at(len-2).toInt(), name,
                                      lineList.at(len-1).toInt()));
        }
        else if (lineList.at(0) == "CHARE")
        {
            chares->insert(lineList.at(1).toInt(),
                           new Chare(lineList.at(2)));
        }
        else if (lineList.at(0) == "VERSION")
        {
            version = lineList.at(1).toFloat();
        }
        else if (lineList.at(0) == "PROCESSORS")
        {
            processes = lineList.at(1).toInt();
        }
    }

    stsfile.close();
}

void CharmImporter::processDefinitions()
{
    rawtrace->functionGroups->insert(0, "MPI");
    rawtrace->functionGroups->insert(1, "Entry");

    rawtrace->functions->insert(999998, new Function("Send", 0));
    rawtrace->functions->insert(999999, new Function("Recv", 0));
    for (QMap<int, Entry *>::Iterator entry = entries->begin();
         entry != entries->end(); ++entry)
    {
        int id = entry.key();
        Entry * e = entry.value();

        rawtrace->functions->insert(id, new Function(e->name, 1));
    }
}
