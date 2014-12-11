#ifndef OTF2EXPORTER_H
#define OTF2EXPORTER_H

#include <otf2/otf2.h>
#include <QString>
#include <QMap>

class Trace;

class OTF2Exporter
{
public:
    OTF2Exporter(Trace * _t);

    void exportTrace(QString path, QString filename);

private:
    Trace * trace;
    int ravel_string;
    int ravel_version_string;

    OTF2_Archive * archive;
    OTF2_GlobalDefWriter * global_def_writer;

    void exportDefinitions();
    void exportStrings();
    int addString(QString str, int counter);
    void exportAttributes();
    void exportFunctions();
    void exportTasks();
    void exportTaskGroups();
    void exportEvents();
    void exportTaskEvents(int taskid);

    QMap<QString, int> inverseStringMap;
};

#endif // OTF2EXPORTER_H
