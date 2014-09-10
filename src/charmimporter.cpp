#include "charmimporter.h"
#include <iostream>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QStack>

CharmImporter::CharmImporter()
    : version(0),
      processes(0),
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
                gzflag);
}

void CharmImporter::readLog(QString logFileName, bool gzipped)
{
    std::cout << "Reading " << logFileName.toStdString().c_str() << std::endl;
    if (gzipped)
    {
        gzFile logfile = gzopen(logFileName.toStdString().c_str(), "r");
        char * buffer = new char[1024];
        gzgets(logfile, buffer, 1024); // Skip first line
        while (gzgets(logfile, buffer, 1024))
            parseLine(QString::fromUtf8(buffer).simplified());

        gzclose(logfile);
    }
    else
    {
        std::ifstream logfile(logFileName.toStdString().c_str());
        std::string line;
        std::getline(logfile, line); // Skip first line
        while(std::getline(logfile, line))
            parseLine(QString::fromStdString(line));

        logfile.close();
    }
}

void CharmImporter::parseLine(QString line)
{
    int index, mtype, entry, event, pe, numpes, id[4];
    long time, msglen, sendTime, recvTime, cpuStart, cpuEnd;
    QStack<CharmEvt *> * events_stack = new QStack<CharmEvt *>();
    QVector<CharmEvt *> * processing_events = new QVector<CharmEvt *>();
    QVector<CharmEvt *> * events = new QVector<CharmEvt *>();

    std::cout << line.toStdString().c_str() << std::endl;
    QStringList lineList = line.split(" ");
    int rectype = lineList.at(0).toInt();
    if (rectype == CREATION || rectype == CREATION_BCAST)
    {
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
    }
    else if (rectype == BEGIN_PROCESSING)
    {
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
    }
    else if (rectype == END_PROCESSING)
    {
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
    }


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
            entries->insert(lineList.at(2).toInt(),
                            new Entry(lineList.at(4).toInt(), lineList.at(3),
                                      lineList.at(5).toInt()));
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
