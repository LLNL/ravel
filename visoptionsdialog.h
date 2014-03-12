#ifndef VISOPTIONSDIALOG_H
#define VISOPTIONSDIALOG_H

#include "visoptions.h"
#include "trace.h"
#include <QDialog>

namespace Ui {
class VisOptionsDialog;
}

class VisOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VisOptionsDialog(QWidget *parent = 0, VisOptions * _options = NULL, Trace * _trace = NULL);
    ~VisOptionsDialog();

public slots:
    void onCancel();
    void onOK();
    void onMetricColorTraditional(bool metricColor);
    void onMetric(QString metric);
    void onShowAggregate(bool showAggregate);
    void onShowMessages(bool showMessages);
    void onColorCombo(QString type);
    void onDrawGnomes(bool drawGnomes);
    void onDrawTop(bool drwaTop);

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
