#include "taskpropertywindow.h"
#include "ui_taskpropertywindow.h"
#include "event.h"

TaskPropertyWindow::TaskPropertyWindow(QWidget *parent, Event *event) :
    QDialog(parent),
    ui(new Ui::TaskPropertyWindow),
    event(event)
{
    ui->setupUi(this);
    ui->id->setText(QString::number(event->entity));
}

TaskPropertyWindow::~TaskPropertyWindow()
{
    delete ui;
}
