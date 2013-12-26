#ifndef OTFIMPORTFUNCTOR_H
#define OTFIMPORTFUNCTOR_H

#include "trace.h"
#include "otfconverter.h"
#include "otfimportoptions.h"
#include <QObject>

class OTFImportFunctor : public QObject
{
    Q_OBJECT
public:
    OTFImportFunctor(OTFImportOptions * _options);
    Trace * getTrace() { return trace; }

public slots:
    void doImport(QString dataFileName);
    void finishInitialRead();
    void updateMatching(int portion, QString msg);
    void updatePreprocess(int portion, QString msg);

signals:
    void done(Trace *);
    void reportProgress(int, QString);

private:
    OTFImportOptions * options;
    Trace * trace;
};

#endif // OTFIMPORTFUNCTOR_H
