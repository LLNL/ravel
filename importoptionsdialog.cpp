#include "importoptionsdialog.h"
#include "ui_importoptionsdialog.h"

ImportOptionsDialog::ImportOptionsDialog(QWidget *parent, OTFImportOptions * _options) :
    QDialog(parent),
    ui(new Ui::ImportOptionsDialog),
    options(_options),
    saved(OTFImportOptions(*_options))
{
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onOK()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(onCancel()));

    setUIState();
}

ImportOptionsDialog::~ImportOptionsDialog()
{
    delete ui;
}

void ImportOptionsDialog::onOK()
{
    this->close();
}

void ImportOptionsDialog::onCancel()
{
    *options = saved;
    setUIState();
}

void ImportOptionsDialog::setUIState()
{
    if (options->waitallMerge)
        ui->waitallCheckbox->setChecked(true);
    else
        ui->waitallCheckbox->setChecked(false);

    if (options->leapMerge)
        ui->leapCheckbox->setChecked(true);
    else
        ui->leapCheckbox->setChecked(false);

    if (options->leapSkip)
        ui->skipCheckbox->setChecked(true);
    else
        ui->skipCheckbox->setChecked(false);

    ui->functionEdit->setText(options->partitionFunction);

    if (options->partitionByFunction)
    {
        ui->heuristicRadioButton->setChecked(false);
        ui->functionRadioButton->setChecked(true);
        ui->waitallCheckbox->setEnabled(false);
        ui->leapCheckbox->setEnabled(false);
        ui->skipCheckbox->setEnabled(false);
        ui->functionEdit->setEnabled(true);
    }
    else
    {
        ui->heuristicRadioButton->setChecked(true);
        ui->functionRadioButton->setChecked(false);
        ui->waitallCheckbox->setEnabled(true);
        ui->leapCheckbox->setEnabled(true);
        ui->skipCheckbox->setEnabled(true);
        ui->functionEdit->setEnabled(false);
    }
}
