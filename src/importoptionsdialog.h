#ifndef IMPORTOPTIONSDIALOG_H
#define IMPORTOPTIONSDIALOG_H

#include <QDialog>
#include "otfimportoptions.h"

// GUI to set import options, must functions are GUI handlers
namespace Ui {
class ImportOptionsDialog;
}

class ImportOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportOptionsDialog(QWidget *parent = 0,
                                 OTFImportOptions * _options = NULL);
    ~ImportOptionsDialog();

public slots:
    void onCancel();
    void onOK();
    void onPartitionByFunction(bool value);
    void onPartitionByHeuristic(bool value);
    void onWaitallMerge(bool merge);
    void onCallerMerge(bool merge);
    void onLeapMerge(bool merge);
    void onLeapSkip(bool skip);
    void onGlobalMerge(bool merge);
    void onIsend(bool coalesce);
    void onMessageSize(bool enforce);
    void onAdvancedStep(bool advanced);
    void onFunctionEdit(const QString& text);
    void onCluster(bool cluster);
    void onSeedEdit(const QString& text);


private:
    Ui::ImportOptionsDialog *ui;

    OTFImportOptions * options;
    OTFImportOptions saved;

    void setUIState();
};

#endif // IMPORTOPTIONSDIALOG_H
