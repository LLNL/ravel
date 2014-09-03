#ifndef CHARMIMPORTER_H
#define CHARMIMPORTER_H

#include <QString>
#include <QMap>
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

    class Entry {
    public:
        Entry(int _chare, QString _name)
            : chare(_chare), name(_name) {}

        int chare;
        QString name;
    };

    QMap<int, QString> * chares;
    QMap<int, Entry *> * entries;

    QString version;
    int processes;


};

#endif // CHARMIMPORTER_H
