#ifndef TASKPROPERTYWINDOW_H
#define TASKPROPERTYWINDOW_H

#include <QDialog>
#include <QString>
#include <QLabel>
#include "event.h"

class Event;

namespace Ui {
class TaskPropertyWindow;
}

class TaskPropertyWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TaskPropertyWindow(QWidget *parent = 0,
                                Event *event = NULL);
    ~TaskPropertyWindow();

private:
    Ui::TaskPropertyWindow *ui;

    Event *event;
};

#endif // TASKPROPERTYWINDOW_H
