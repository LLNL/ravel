#include "filterdialog.h"
#include "ui_filterdialog.h"
#include "addfunctionsdialog.h"
#include <QDebug>

FilterDialog::FilterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilterDialog),
    addFuncDialog(NULL)
{
    ui->setupUi(this);
    connect(ui->addFunctions, SIGNAL(clicked()), this, SLOT(openAddFunctionsDialog()));
    connect(ui->removeFunctions, SIGNAL(clicked()), this, SLOT(openRemoveFunctionsDialog()));
    connect(ui->importFunctions, SIGNAL(clicked()), this, SLOT(openImportFunctionsDialog()));
}

FilterDialog::~FilterDialog()
{
    delete ui;
}

void FilterDialog::openAddFunctionsDialog()
{
    delete addFuncDialog;
    qDebug(ui->addFunctions->text().toLatin1());
    addFuncDialog = new AddFunctionsDialog(this);
    addFuncDialog->show();
}

void FilterDialog::openRemoveFunctionsDialog()
{
    qDebug(ui->removeFunctions->text().toLatin1());
}

void FilterDialog::openImportFunctionsDialog()
{
    qDebug(ui->importFunctions->text().toLatin1());
}
