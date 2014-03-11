#include "visoptionsdialog.h"
#include "ui_visoptionsdialog.h"
#include <iostream>

VisOptionsDialog::VisOptionsDialog(QWidget *parent, VisOptions * _options, Trace * _trace) :
    QDialog(parent),
    ui(new Ui::VisOptionsDialog),
    isSet(false),
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
    connect(ui->colorComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(onColorCombo(QString)));
    connect(ui->gnomeCheckBox, SIGNAL(clicked(bool)), this, SLOT(onDrawGnomes(bool)));

    if (trace)
    {
        for (QList<QString>::Iterator metric = trace->metrics->begin(); metric != trace->metrics->end(); ++metric)
            ui->metricComboBox->addItem(*metric);
        ui->colorComboBox->addItem("Sequential");
        ui->colorComboBox->addItem("Diverging");
        ui->colorComboBox->addItem("Categorical");
    }

    setUIState();
    isSet = true;
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
    if (!isSet)
        return;
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

void VisOptionsDialog::onColorCombo(QString type)
{
    if (!isSet)
        return;

    options->categoricalColors = false;
    if (type == "Sequential") {
        options->maptype = VisOptions::SEQUENTIAL;
        options->colormap = options->rampmap;
    } else if (type == "Categorical") {
        options->maptype = VisOptions::CATEGORICAL;
        options->colormap = options->catcolormap;
        options->categoricalColors = true;
    } else {
        options->maptype = VisOptions::DIVERGING;
        options->colormap = options->divergentmap;
    }
}

void VisOptionsDialog::onDrawGnomes(bool drawGnomes)
{
    options->drawGnomes = drawGnomes;
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

    if (options->drawGnomes)
        ui->gnomeCheckBox->setChecked(true);
    else
        ui->gnomeCheckBox->setChecked(false);

    if (trace)
    {
        int metric_index = mapMetricToIndex(options->metric);
        ui->metricComboBox->setCurrentIndex(metric_index);
        options->metric = ui->metricComboBox->itemText(metric_index); // In case we're stuck at the default
        if (options->maptype == VisOptions::SEQUENTIAL)
            ui->colorComboBox->setCurrentIndex(0);
        else if (options->maptype == VisOptions::CATEGORICAL)
            ui->colorComboBox->setCurrentIndex(2);
        else
            ui->colorComboBox->setCurrentIndex(1);
    }
}

int VisOptionsDialog::mapMetricToIndex(QString metric)
{
    for (int i = 0; i < trace->metrics->length(); i++)
    {
        if (trace->metrics->at(i).compare(metric) == 0)
            return i;
    }

    return 0;
}
