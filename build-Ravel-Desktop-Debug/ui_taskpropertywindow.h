/********************************************************************************
** Form generated from reading UI file 'taskpropertywindow.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TASKPROPERTYWINDOW_H
#define UI_TASKPROPERTYWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>

QT_BEGIN_NAMESPACE

class Ui_TaskPropertyWindow
{
public:
    QDialogButtonBox *buttonBox;
    QLabel *heading;
    QLabel *id;

    void setupUi(QDialog *TaskPropertyWindow)
    {
        if (TaskPropertyWindow->objectName().isEmpty())
            TaskPropertyWindow->setObjectName(QStringLiteral("TaskPropertyWindow"));
        TaskPropertyWindow->resize(400, 300);
        buttonBox = new QDialogButtonBox(TaskPropertyWindow);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setGeometry(QRect(30, 240, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        heading = new QLabel(TaskPropertyWindow);
        heading->setObjectName(QStringLiteral("heading"));
        heading->setGeometry(QRect(30, 10, 211, 31));
        id = new QLabel(TaskPropertyWindow);
        id->setObjectName(QStringLiteral("id"));
        id->setGeometry(QRect(30, 60, 67, 17));

        retranslateUi(TaskPropertyWindow);
        QObject::connect(buttonBox, SIGNAL(accepted()), TaskPropertyWindow, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), TaskPropertyWindow, SLOT(reject()));

        QMetaObject::connectSlotsByName(TaskPropertyWindow);
    } // setupUi

    void retranslateUi(QDialog *TaskPropertyWindow)
    {
        TaskPropertyWindow->setWindowTitle(QApplication::translate("TaskPropertyWindow", "Dialog", 0));
        heading->setText(QApplication::translate("TaskPropertyWindow", "Task Information", 0));
        id->setText(QApplication::translate("TaskPropertyWindow", "TextLabel", 0));
    } // retranslateUi

};

namespace Ui {
    class TaskPropertyWindow: public Ui_TaskPropertyWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TASKPROPERTYWINDOW_H
