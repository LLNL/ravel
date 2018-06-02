/********************************************************************************
** Form generated from reading UI file 'metricrangedialog.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_METRICRANGEDIALOG_H
#define UI_METRICRANGEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_MetricRangeDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QLineEdit *metricEdit;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *MetricRangeDialog)
    {
        if (MetricRangeDialog->objectName().isEmpty())
            MetricRangeDialog->setObjectName(QStringLiteral("MetricRangeDialog"));
        MetricRangeDialog->resize(285, 101);
        verticalLayout = new QVBoxLayout(MetricRangeDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        label = new QLabel(MetricRangeDialog);
        label->setObjectName(QStringLiteral("label"));

        verticalLayout->addWidget(label);

        metricEdit = new QLineEdit(MetricRangeDialog);
        metricEdit->setObjectName(QStringLiteral("metricEdit"));

        verticalLayout->addWidget(metricEdit);

        buttonBox = new QDialogButtonBox(MetricRangeDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok|QDialogButtonBox::Reset);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(MetricRangeDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), MetricRangeDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), MetricRangeDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(MetricRangeDialog);
    } // setupUi

    void retranslateUi(QDialog *MetricRangeDialog)
    {
        MetricRangeDialog->setWindowTitle(QApplication::translate("MetricRangeDialog", "Dialog", 0));
        label->setText(QApplication::translate("MetricRangeDialog", "Maximum Value:", 0));
    } // retranslateUi

};

namespace Ui {
    class MetricRangeDialog: public Ui_MetricRangeDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_METRICRANGEDIALOG_H
