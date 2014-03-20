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
    connect(ui->functionRadioButton, SIGNAL(clicked(bool)), this, SLOT(onPartitionByFunction(bool)));
    connect(ui->heuristicRadioButton, SIGNAL(clicked(bool)), this, SLOT(onPartitionByHeuristic(bool)));
    connect(ui->waitallCheckbox, SIGNAL(clicked(bool)), this, SLOT(onWaitallMerge(bool)));
    connect(ui->leapCheckbox, SIGNAL(clicked(bool)), this, SLOT(onLeapMerge(bool)));
    connect(ui->leapCollectiveCheckbox, SIGNAL(clicked(bool)), this, SLOT(onLeapCollective(bool)));
    connect(ui->skipCheckbox, SIGNAL(clicked(bool)), this, SLOT(onLeapSkip(bool)));
    connect(ui->globalMergeBox, SIGNAL(clicked(bool)), this, SLOT(onGlobalMerge(bool)));
    connect(ui->functionEdit, SIGNAL(textChanged(QString)), this, SLOT(onFunctionEdit(QString)));
    connect(ui->clusterCheckbox, SIGNAL(clicked(bool)), this, SLOT(onCluster(bool)));

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

void ImportOptionsDialog::onPartitionByFunction(bool value)
{
    options->partitionByFunction = value;
    setUIState();
}

void ImportOptionsDialog::onPartitionByHeuristic(bool value)
{
    options->partitionByFunction = !value;
    setUIState();
}

void ImportOptionsDialog::onWaitallMerge(bool merge)
{
    options->waitallMerge = merge;
}

void ImportOptionsDialog::onLeapMerge(bool merge)
{
    options->leapMerge = merge;
    setUIState();
}

void ImportOptionsDialog::onLeapCollective(bool respect)
{
    options->leapCollective = respect;
}

void ImportOptionsDialog::onLeapSkip(bool skip)
{
    options->leapSkip = skip;
}

void ImportOptionsDialog::onGlobalMerge(bool merge)
{
    options->globalMerge = merge;
}

void ImportOptionsDialog::onCluster(bool cluster)
{
    options->cluster = cluster;
}


void ImportOptionsDialog::onFunctionEdit(const QString& text)
{
    options->partitionFunction = text;
}


void ImportOptionsDialog::setUIState()
{
    if (options->waitallMerge)
        ui->waitallCheckbox->setChecked(true);
    else
        ui->waitallCheckbox->setChecked(false);

    if (options->leapSkip)
        ui->skipCheckbox->setChecked(true);
    else
        ui->skipCheckbox->setChecked(false);

    if (options->leapCollective)
        ui->leapCollectiveCheckbox->setChecked(true);
    else
        ui->leapCollectiveCheckbox->setChecked(false);

    if (options->leapMerge)
    {
        ui->leapCheckbox->setChecked(true);
        ui->skipCheckbox->setEnabled(true);
        ui->leapCollectiveCheckbox->setEnabled(true);
    }
    else
    {
        ui->leapCheckbox->setChecked(false);
        ui->skipCheckbox->setEnabled(false);
        ui->leapCollectiveCheckbox->setEnabled(false);
    }

    if (options->globalMerge)
        ui->globalMergeBox->setChecked(true);
    else
        ui->globalMergeBox->setChecked(false);

    if (options->cluster)
        ui->clusterCheckbox->setChecked(true);
    else
        ui->clusterCheckbox->setChecked(false);



    ui->functionEdit->setText(options->partitionFunction);

    if (options->partitionByFunction)
    {
        ui->heuristicRadioButton->setChecked(false);
        ui->functionRadioButton->setChecked(true);
        ui->waitallCheckbox->setEnabled(false);
        ui->leapCheckbox->setEnabled(false);
        ui->leapCollectiveCheckbox->setEnabled(false);
        ui->skipCheckbox->setEnabled(false);
        ui->functionEdit->setEnabled(true);
        ui->globalMergeBox->setEnabled(false);
    }
    else
    {
        ui->heuristicRadioButton->setChecked(true);
        ui->functionRadioButton->setChecked(false);
        ui->waitallCheckbox->setEnabled(true);
        ui->leapCheckbox->setEnabled(true);
        ui->functionEdit->setEnabled(false);
        ui->globalMergeBox->setEnabled(true);
    }
}
