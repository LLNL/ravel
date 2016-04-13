#ifndef OTF2EXPORTER_H
#define OTF2EXPORTER_H

#include <otf2/otf2.h>
#include <QString>
#include <QMap>
#include <QList>

class Trace;
class Entity;

class OTF2Exporter
{
public:
    OTF2Exporter(Trace * _t);
    ~OTF2Exporter();

    void exportTrace(QString path, QString filename);

    static OTF2_FlushType
    pre_flush( void*            userData,
               OTF2_FileType    fileType,
               OTF2_LocationRef location,
               void*            callerData,
               bool             final )
    {
        return OTF2_FLUSH;
    }

    static OTF2_TimeStamp
    post_flush( void*            userData,
                OTF2_FileType    fileType,
                OTF2_LocationRef location )
    {
        return 0;
    }

    OTF2_FlushCallbacks flush_callbacks;

private:
    Trace * trace;
    QList<Entity *> * entities;
    int ravel_string;
    int ravel_version_string;

    OTF2_Archive * archive;
    OTF2_GlobalDefWriter * global_def_writer;

    void exportDefinitions();
    void exportStrings();
    int addString(QString str, int counter);
    void exportAttributes();
    void exportFunctions();
    void exportEntities();
    void exportEntityGroups();
    void exportEvents();
    void exportEntityEvents(int entityid);

    QMap<QString, int> inverseStringMap;
    QMap<QString, int> * attributeMap;
};

#endif // OTF2EXPORTER_H
