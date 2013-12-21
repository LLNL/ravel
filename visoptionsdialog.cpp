#include "visoptionsdialog.h"
#include "ui_visoptionsdialog.h"

VisOptionsDialog::VisOptionsDialog(QWidget *parent, VisOptions * _options) :
    QDialog(parent),
    ui(new Ui::VisOptionsDialog),
    options(_options),
    saved(VisOptions(*_options))
{
    ui->setupUi(this);
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

void VisOptionsDialog::setUIState()
{

}
