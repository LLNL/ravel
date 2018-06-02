/********************************************************************************
** Form generated from reading UI file 'visoptionsdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VISOPTIONSDIALOG_H
#define UI_VISOPTIONSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_VisOptionsDialog
{
public:
    QVBoxLayout *verticalLayout;
    QCheckBox *absoluteTimeCheckBox;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QComboBox *messageComboBox;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QComboBox *metricComboBox;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QComboBox *colorComboBox;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *VisOptionsDialog)
    {
        if (VisOptionsDialog->objectName().isEmpty())
            VisOptionsDialog->setObjectName(QStringLiteral("VisOptionsDialog"));
        VisOptionsDialog->resize(357, 150);
        verticalLayout = new QVBoxLayout(VisOptionsDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        absoluteTimeCheckBox = new QCheckBox(VisOptionsDialog);
        absoluteTimeCheckBox->setObjectName(QStringLiteral("absoluteTimeCheckBox"));

        verticalLayout->addWidget(absoluteTimeCheckBox);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(-1, 0, 0, -1);
        label_3 = new QLabel(VisOptionsDialog);
        label_3->setObjectName(QStringLiteral("label_3"));

        horizontalLayout_3->addWidget(label_3);

        messageComboBox = new QComboBox(VisOptionsDialog);
        messageComboBox->setObjectName(QStringLiteral("messageComboBox"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(messageComboBox->sizePolicy().hasHeightForWidth());
        messageComboBox->setSizePolicy(sizePolicy);

        horizontalLayout_3->addWidget(messageComboBox);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label = new QLabel(VisOptionsDialog);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout->addWidget(label);

        metricComboBox = new QComboBox(VisOptionsDialog);
        metricComboBox->setObjectName(QStringLiteral("metricComboBox"));
        sizePolicy.setHeightForWidth(metricComboBox->sizePolicy().hasHeightForWidth());
        metricComboBox->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(metricComboBox);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label_2 = new QLabel(VisOptionsDialog);
        label_2->setObjectName(QStringLiteral("label_2"));

        horizontalLayout_2->addWidget(label_2);

        colorComboBox = new QComboBox(VisOptionsDialog);
        colorComboBox->setObjectName(QStringLiteral("colorComboBox"));
        sizePolicy.setHeightForWidth(colorComboBox->sizePolicy().hasHeightForWidth());
        colorComboBox->setSizePolicy(sizePolicy);

        horizontalLayout_2->addWidget(colorComboBox);


        verticalLayout->addLayout(horizontalLayout_2);

        buttonBox = new QDialogButtonBox(VisOptionsDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(VisOptionsDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), VisOptionsDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), VisOptionsDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(VisOptionsDialog);
    } // setupUi

    void retranslateUi(QDialog *VisOptionsDialog)
    {
        VisOptionsDialog->setWindowTitle(QApplication::translate("VisOptionsDialog", "Visualization Options", 0));
        absoluteTimeCheckBox->setText(QApplication::translate("VisOptionsDialog", "use absolute time", 0));
#ifndef QT_NO_TOOLTIP
        label_3->setToolTip(QApplication::translate("VisOptionsDialog", "Message lines can be drawn from sender to receiver, within the sender event, or not at all.", 0));
#endif // QT_NO_TOOLTIP
        label_3->setText(QApplication::translate("VisOptionsDialog", "message style:", 0));
        label->setText(QApplication::translate("VisOptionsDialog", "structure metric:", 0));
        label_2->setText(QApplication::translate("VisOptionsDialog", "color map:", 0));
    } // retranslateUi

};

namespace Ui {
    class VisOptionsDialog: public Ui_VisOptionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VISOPTIONSDIALOG_H
