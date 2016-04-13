#ifndef IMPORTFUNCTOR_H
#define IMPORTFUNCTOR_H

#include <QObject>
#include <QString>

class Trace;
class ImportOptions;

// Handle signaling for progress bar
class ImportFunctor : public QObject
{
    Q_OBJECT
public:
    ImportFunctor(ImportOptions * _options);
    Trace * getTrace() { return trace; }

public slots:
    void doImportOTF(QString dataFileName);
    void doImportOTF2(QString dataFileName);
    void doImportCharm(QString dataFileName);
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
    ImportOptions * options;
    Trace * trace;
};

#endif // IMPORTFUNCTOR_H
