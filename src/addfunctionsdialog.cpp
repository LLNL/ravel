#include "addfunctionsdialog.h"
#include "ui_addfunctionsdialog.h"

AddFunctionsDialog::AddFunctionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddFunctionsDialog)
{
    ui->setupUi(this);
    ui->filterStartLabel->setVisible(false);
    ui->filterStart->setVisible(false);
    ui->filterEndLabel->setVisible(false);
    ui->filterEnd->setVisible(false);

    connect(ui->filterOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(switchVisibility(int)));
}

AddFunctionsDialog::~AddFunctionsDialog()
{
    delete ui;
}

void AddFunctionsDialog::switchVisibility(int option)
{
    switch(option)
    {
        case 0:
        case 1: ui->filterStringLabel->setVisible(true);
                ui->filterString->setVisible(true);
                ui->filterStartLabel->setVisible(false);
                ui->filterStart->setVisible(false);
                ui->filterEndLabel->setVisible(false);
                ui->filterEnd->setVisible(false);
                break;
        case 2: ui->filterStringLabel->setVisible(false);
                ui->filterString->setVisible(false);
                ui->filterStartLabel->setVisible(true);
                ui->filterStart->setVisible(true);
                ui->filterEndLabel->setVisible(true);
                ui->filterEnd->setVisible(true);
                break;
    }
}
