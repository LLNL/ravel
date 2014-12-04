#ifndef OTFIMPORTFUNCTOR_H
#define OTFIMPORTFUNCTOR_H

#include "trace.h"
#include "otfconverter.h"
#include "otfimportoptions.h"
#include "otf2importer.h"
#include <QObject>

// Handle signaling for progress bar
class OTFImportFunctor : public QObject
{
    Q_OBJECT
public:
    OTFImportFunctor(OTFImportOptions * _options);
    Trace * getTrace() { return trace; }

public slots:
    void doImportOTF(QString dataFileName);
    void doImportOTF2(QString dataFileName);
    void finishInitialRead();
    void updateMatching(int portion, QString msg);
    void updatePreprocess(int portion, QString msg);
    void updateClustering(int portion);
    void switchProgress();

signals:
    void switching();
    void done(Trace *);
    void reportProgress(int, QString);
    void reportClusterProgress(int, QString);

private:
    OTFImportOptions * options;
    Trace * trace;
};

#endif // OTFIMPORTFUNCTOR_H
