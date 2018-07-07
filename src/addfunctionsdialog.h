#ifndef ADDFUNCTIONSDIALOG_H
#define ADDFUNCTIONSDIALOG_H

#include <QDialog>

namespace Ui {
class AddFunctionsDialog;
}

class AddFunctionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddFunctionsDialog(QWidget *parent = 0);
    ~AddFunctionsDialog();

public slots:
    void switchVisibility(int);

private:
    Ui::AddFunctionsDialog *ui;
};

#endif // ADDFUNCTIONSDIALOG_H
