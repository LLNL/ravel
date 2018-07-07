#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include <QDialog>

class AddFunctionsDialog;

namespace Ui {
class FilterDialog;
}

class FilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDialog(QWidget *parent = 0);
    ~FilterDialog();

public slots:
    void openAddFunctionsDialog();
    void openRemoveFunctionsDialog();
    void openImportFunctionsDialog();

private:
    Ui::FilterDialog *ui;

    // Add functions
    AddFunctionsDialog *addFuncDialog;

    // Remove functions

    // Import Functions
};

#endif // FILTERDIALOG_H
