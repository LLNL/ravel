#ifndef VISOPTIONSDIALOG_H
#define VISOPTIONSDIALOG_H

#include "visoptions.h"
#include <QDialog>

namespace Ui {
class VisOptionsDialog;
}

class VisOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VisOptionsDialog(QWidget *parent = 0, VisOptions * _options = NULL);
    ~VisOptionsDialog();

public slots:
    void onCancel();
    void onOK();

private:
    Ui::VisOptionsDialog *ui;

    VisOptions * options;
    VisOptions saved;

    void setUIState();
};

#endif // VISOPTIONSDIALOG_H
