#include "visoptionsdialog.h"
#include "ui_visoptionsdialog.h"
#include <iostream>

VisOptionsDialog::VisOptionsDialog(QWidget *parent, VisOptions * _options, Trace * _trace) :
    QDialog(parent),
    ui(new Ui::VisOptionsDialog),
    options(_options),
    saved(VisOptions(*_options)),
    trace(_trace)
{
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onOK()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(onCancel()));
    connect(ui->metricColorTraditionalCheckBox, SIGNAL(clicked(bool)), this, SLOT(onMetricColorTraditional(bool)));
    connect(ui->metricComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(onMetric(QString)));
    connect(ui->showAggregateCheckBox, SIGNAL(clicked(bool)), this, SLOT(onShowAggregate(bool)));
    connect(ui->showMessagesCheckBox, SIGNAL(clicked(bool)), this, SLOT(onShowMessages(bool)));

    if (trace)
        for (QList<QString>::Iterator metric = trace->metrics->begin(); metric != trace->metrics->end(); ++metric)
            ui->metricComboBox->addItem(*metric);

    setUIState();
}

VisOptionsDialog::~VisOptionsDialog()
{
    delete ui;
}

void VisOptionsDialog::onOK()
{
    this->close();
}

void VisOptionsDialog::onCancel()
{
    *options = saved;
    setUIState();
}

void VisOptionsDialog::onMetricColorTraditional(bool metricColor)
{
    options->colorTraditionalByMetric = metricColor;
}

void VisOptionsDialog::onMetric(QString metric)
{
    if (trace)
        options->metric = metric;
}

void VisOptionsDialog::onShowAggregate(bool showAggregate)
{
    options->showAggregateSteps = showAggregate;
}

void VisOptionsDialog::onShowMessages(bool showMessages)
{
    options->showMessages = showMessages;
}

void VisOptionsDialog::setUIState()
{
    if (options->colorTraditionalByMetric)
        ui->metricColorTraditionalCheckBox->setChecked(true);
    else
        ui->metricColorTraditionalCheckBox->setChecked(false);

    if (options->showAggregateSteps)
        ui->showAggregateCheckBox->setChecked(true);
    else
        ui->showAggregateCheckBox->setChecked(false);

    if (options->showMessages)
        ui->showMessagesCheckBox->setChecked(true);
    else
        ui->showMessagesCheckBox->setChecked(false);

    if (trace)
    {
        int metric_index = mapMetricToIndex(options->metric);
        ui->metricComboBox->setCurrentIndex(metric_index);
        options->metric = ui->metricComboBox->itemText(metric_index); // In case we're stuck at the defautl
    }
}

int VisOptionsDialog::mapMetricToIndex(QString metric)
{
    int index = 0;
    for (int i = 0; i < trace->metrics->length(); i++)
        if (trace->metrics->at(i) == metric)
            return index;

    return index;
}
