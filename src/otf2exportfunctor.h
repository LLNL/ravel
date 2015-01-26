#ifndef OTF2EXPORTFUNCTOR_H
#define OTF2EXPORTFUNCTOR_H

#include <QObject>
#include <QString>

class Trace;

class OTF2ExportFunctor : public QObject
{
    Q_OBJECT
public:
    OTF2ExportFunctor();

public slots:
    void exportTrace(Trace *, const QString&, const QString&);

signals:
    void done();
};

#endif // OTF2EXPORTFUNCTOR_H
