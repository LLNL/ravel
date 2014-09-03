#include "charmimporter.h"
#include <iostream>
#include <QStringList>
#include <QDir>
#include <QFileInfo>

CharmImporter::CharmImporter()
    : version(""),
      processes(0),
      chares(new QMap<int, QString>()),
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
                            new Entry(lineList.at(4).toInt(), lineList.at(3)));
        }
        else if (lineList.at(0) == "CHARE")
        {
            chares->insert(lineList.at(1).toInt(), lineList.at(2));
        }
        else if (lineList.at(0) == "VERSION")
        {
            version = lineList.at(1);
        }
        else if (lineList.at(0) == "PROCESSORS")
        {
            processes = lineList.at(1).toInt();
        }
    }
}
