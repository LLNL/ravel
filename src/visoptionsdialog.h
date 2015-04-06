#ifndef VISOPTIONSDIALOG_H
#define VISOPTIONSDIALOG_H

#include <QDialog>
#include <QString>
#include "visoptions.h"

class Trace;

// GUI element for handling Vis options
namespace Ui {
class VisOptionsDialog;
}

class VisOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VisOptionsDialog(QWidget *parent = 0,
                              VisOptions * _options = NULL,
                              Trace * _trace = NULL);
    ~VisOptionsDialog();

public slots:
    void onCancel();
    void onOK();
    void onAbsoluteTime(bool absolute);
    void onMetricColorTraditional(bool metricColor);
    void onMetric(QString metric);
    void onShowAggregate(bool showAggregate);
    void onTraceBack(bool traceBack);
    void onShowMessages(int showMessages);
    void onColorCombo(QString type);
    void onShowInactive(bool showInactive);

private:
    int mapMetricToIndex(QString metric);

    Ui::VisOptionsDialog *ui;
    bool isSet;

    VisOptions * options;
    VisOptions saved;

    Trace * trace;

    void setUIState();
};

#endif // VISOPTIONSDIALOG_H
