#ifndef TASKPROPERTYWINDOW_H
#define TASKPROPERTYWINDOW_H

#include <QDialog>
#include <QString>
#include <QLabel>
#include "event.h"
#include "function.h"

class Event;
class Function;

namespace Ui {
class TaskPropertyWindow;
}

class TaskPropertyWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TaskPropertyWindow(QWidget *parent = 0,
                                Event *event = NULL, Function *func = NULL, QString units = "");
    ~TaskPropertyWindow();

private:
    Ui::TaskPropertyWindow *ui;

    Event *event;
    Function *func;
};

#endif // TASKPROPERTYWINDOW_H
