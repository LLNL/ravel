#include "taskpropertywindow.h"
#include "ui_taskpropertywindow.h"
#include "event.h"
#include "function.h"
#include "trace.h"
#include "ravelutils.h"

TaskPropertyWindow::TaskPropertyWindow(QWidget *parent, Event *event, Function *func, QString units) :
    QDialog(parent),
    ui(new Ui::TaskPropertyWindow),
    event(event),
    func(func)
{
    ui->setupUi(this);

    ui->name_value->setText(func->name);
    ui->start_value->setText(QString::number(event->enter) + " " + units);
    ui->end_value->setText(QString::number(event->exit) + " " + units);
}

TaskPropertyWindow::~TaskPropertyWindow()
{
    delete ui;
}