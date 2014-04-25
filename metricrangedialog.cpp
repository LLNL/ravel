#include "metricrangedialog.h"
#include "ui_metricrangedialog.h"

MetricRangeDialog::MetricRangeDialog(QWidget *parent, long long int current,
                                     long long int original)
    : QDialog(parent),
    ui(new Ui::MetricRangeDialog),
    start_value(current),
    original_value(original)
{
    ui->setupUi(this);

    ui->metricEdit->setText(QString::number(current));
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this,
            SLOT(onButtonClick(QAbstractButton*)));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onOK()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(onCancel()));
}

MetricRangeDialog::~MetricRangeDialog()
{
    delete ui;
}

void MetricRangeDialog::onButtonClick(QAbstractButton* qab)
{
    if (ui->buttonBox->buttonRole(qab) == QDialogButtonBox::ResetRole)
    {
        if (ui->metricEdit->text().toLongLong() != original_value)
        {
            start_value = original_value;
            ui->metricEdit->setText(QString::number(original_value));
            emit(valueChanged(original_value));
        }
        this->hide();
    }
}

void MetricRangeDialog::onOK()
{
    if (ui->metricEdit->text().toLongLong() != start_value)
    {
        start_value = ui->metricEdit->text().toLongLong();
        ui->metricEdit->setText(QString::number(start_value));
        emit(valueChanged(start_value));
    }
    this->hide();
}

void MetricRangeDialog::onCancel()
{
    if (ui->metricEdit->text().toLongLong() != start_value)
    {
        ui->metricEdit->setText(QString::number(start_value));
    }
    this->hide();
}

void MetricRangeDialog::setCurrent(long long int current)
{
    start_value = current;
}
