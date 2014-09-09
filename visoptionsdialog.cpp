#include "visoptionsdialog.h"
#include "ui_visoptionsdialog.h"
#include <iostream>

VisOptionsDialog::VisOptionsDialog(QWidget *parent, VisOptions * _options,
                                   Trace * _trace)
    : QDialog(parent),
    ui(new Ui::VisOptionsDialog),
    isSet(false),
    options(_options),
    saved(VisOptions(*_options)),
    trace(_trace)
{
    ui->setupUi(this);

    ui->colorComboBox->addItem("Sequential");
    ui->colorComboBox->addItem("Diverging");
    ui->colorComboBox->addItem("Categorical");
    ui->messageComboBox->addItem("No Messages");
    ui->messageComboBox->addItem("Across Steps");
    ui->messageComboBox->addItem("Within Step");

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onOK()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(onCancel()));
    connect(ui->metricColorTraditionalCheckBox, SIGNAL(clicked(bool)), this,
            SLOT(onMetricColorTraditional(bool)));
    connect(ui->absoluteTimeCheckBox, SIGNAL(clicked(bool)), this,
            SLOT(onAbsoluteTime(bool)));
    connect(ui->metricComboBox, SIGNAL(currentIndexChanged(QString)), this,
            SLOT(onMetric(QString)));
    connect(ui->showAggregateCheckBox, SIGNAL(clicked(bool)), this,
            SLOT(onShowAggregate(bool)));
    connect(ui->messageComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onShowMessages(int)));
    connect(ui->inactiveCheckBox, SIGNAL(clicked(bool)), this,
            SLOT(onShowInactive(bool)));
    connect(ui->colorComboBox, SIGNAL(currentIndexChanged(QString)), this,
            SLOT(onColorCombo(QString)));

    // We only have metrics if we have an active trace.
    if (trace)
    {
        for (QList<QString>::Iterator metric = trace->metrics->begin();
             metric != trace->metrics->end(); ++metric)
        {
            ui->metricComboBox->addItem(*metric);
        }
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

void VisOptionsDialog::onAbsoluteTime(bool absolute)
{
    options->absoluteTime = absolute;
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

void VisOptionsDialog::onShowMessages(int showMessages)
{
    if (showMessages == 0)
        options->showMessages = VisOptions::MSG_NONE;
    else if (showMessages == 1)
        options->showMessages = VisOptions::MSG_TRUE;
    else if (showMessages == 2)
        options->showMessages = VisOptions::MSG_SINGLE;
}

void VisOptionsDialog::onShowInactive(bool showInactive)
{
    options->showInactiveSteps = showInactive;
}

void VisOptionsDialog::onColorCombo(QString type)
{
    if (!isSet)
        return;

    if (type == "Sequential") {
        options->maptype = VisOptions::COLOR_SEQUENTIAL;
        options->colormap = options->rampmap;
    } else if (type == "Categorical") {
        options->maptype = VisOptions::COLOR_CATEGORICAL;
        options->colormap = options->catcolormap;
    } else {
        options->maptype = VisOptions::COLOR_DIVERGING;
        options->colormap = options->divergentmap;
    }
}

void VisOptionsDialog::setUIState()
{
    if (options->absoluteTime)
        ui->absoluteTimeCheckBox->setChecked(true);
    else
        ui->absoluteTimeCheckBox->setChecked(true);

    if (options->colorTraditionalByMetric)
        ui->metricColorTraditionalCheckBox->setChecked(true);
    else
        ui->metricColorTraditionalCheckBox->setChecked(false);

    if (options->showAggregateSteps)
        ui->showAggregateCheckBox->setChecked(true);
    else
        ui->showAggregateCheckBox->setChecked(false);

    if (options->showInactiveSteps)
        ui->inactiveCheckBox->setChecked(true);
    else
        ui->inactiveCheckBox->setChecked(false);

    if (options->maptype == VisOptions::COLOR_SEQUENTIAL)
        ui->colorComboBox->setCurrentIndex(0);
    else if (options->maptype == VisOptions::COLOR_CATEGORICAL)
        ui->colorComboBox->setCurrentIndex(2);
    else
        ui->colorComboBox->setCurrentIndex(1);

    if (options->showMessages == VisOptions::MSG_NONE)
        ui->messageComboBox->setCurrentIndex(0);
    else if (options->showMessages == VisOptions::MSG_TRUE)
        ui->messageComboBox->setCurrentIndex(1);
    else
        ui->messageComboBox->setCurrentIndex(2);


    if (trace)
    {
        int metric_index = mapMetricToIndex(options->metric);
        ui->metricComboBox->setCurrentIndex(metric_index);

        // In case we're stuck at the default
        options->metric = ui->metricComboBox->itemText(metric_index);
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
