/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionOpen_JSON;
    QAction *actionOpen_Trace;
    QAction *actionTrace_Importing;
    QAction *actionVisualization;
    QAction *actionLogical_Steps;
    QAction *actionClustered_Logical_Steps;
    QAction *actionPhysical_Time;
    QAction *actionMetric_Overview;
    QAction *actionQuit;
    QAction *actionClose;
    QAction *actionSave;
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout_4;
    QSplitter *splitter_3;
    QWidget *sideSplitWidget;
    QVBoxLayout *verticalLayout_2;
    QSplitter *sideSplitter;
    QWidget *traditionalSide;
    QVBoxLayout *verticalLayout_6;
    QWidget *traditionalLabelWidget;
    QVBoxLayout *verticalLayout_8;
    QWidget *overviewSide;
    QVBoxLayout *verticalLayout_4;
    QWidget *overviewLabelWidget;
    QWidget *splitWidget;
    QVBoxLayout *verticalLayout;
    QSplitter *splitter;
    QWidget *traditionalContainer;
    QHBoxLayout *horizontalLayout_3;
    QWidget *overviewContainer;
    QHBoxLayout *horizontalLayout_5;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuOptions;
    QMenu *menuViews;
    QMenu *menuTraces;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(1600, 611);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        actionOpen_JSON = new QAction(MainWindow);
        actionOpen_JSON->setObjectName(QStringLiteral("actionOpen_JSON"));
        actionOpen_Trace = new QAction(MainWindow);
        actionOpen_Trace->setObjectName(QStringLiteral("actionOpen_Trace"));
        actionTrace_Importing = new QAction(MainWindow);
        actionTrace_Importing->setObjectName(QStringLiteral("actionTrace_Importing"));
        actionVisualization = new QAction(MainWindow);
        actionVisualization->setObjectName(QStringLiteral("actionVisualization"));
        actionLogical_Steps = new QAction(MainWindow);
        actionLogical_Steps->setObjectName(QStringLiteral("actionLogical_Steps"));
        actionLogical_Steps->setCheckable(true);
        actionLogical_Steps->setChecked(true);
        actionClustered_Logical_Steps = new QAction(MainWindow);
        actionClustered_Logical_Steps->setObjectName(QStringLiteral("actionClustered_Logical_Steps"));
        actionClustered_Logical_Steps->setCheckable(true);
        actionClustered_Logical_Steps->setChecked(true);
        actionClustered_Logical_Steps->setShortcutContext(Qt::ApplicationShortcut);
        actionPhysical_Time = new QAction(MainWindow);
        actionPhysical_Time->setObjectName(QStringLiteral("actionPhysical_Time"));
        actionPhysical_Time->setCheckable(true);
        actionMetric_Overview = new QAction(MainWindow);
        actionMetric_Overview->setObjectName(QStringLiteral("actionMetric_Overview"));
        actionMetric_Overview->setCheckable(true);
        actionMetric_Overview->setChecked(true);
        actionQuit = new QAction(MainWindow);
        actionQuit->setObjectName(QStringLiteral("actionQuit"));
        actionQuit->setMenuRole(QAction::QuitRole);
        actionClose = new QAction(MainWindow);
        actionClose->setObjectName(QStringLiteral("actionClose"));
        actionSave = new QAction(MainWindow);
        actionSave->setObjectName(QStringLiteral("actionSave"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(1);
        sizePolicy1.setHeightForWidth(centralwidget->sizePolicy().hasHeightForWidth());
        centralwidget->setSizePolicy(sizePolicy1);
        horizontalLayout_4 = new QHBoxLayout(centralwidget);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 6, 0, 0);
        splitter_3 = new QSplitter(centralwidget);
        splitter_3->setObjectName(QStringLiteral("splitter_3"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(splitter_3->sizePolicy().hasHeightForWidth());
        splitter_3->setSizePolicy(sizePolicy2);
        splitter_3->setOrientation(Qt::Horizontal);
        sideSplitWidget = new QWidget(splitter_3);
        sideSplitWidget->setObjectName(QStringLiteral("sideSplitWidget"));
        verticalLayout_2 = new QVBoxLayout(sideSplitWidget);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        sideSplitter = new QSplitter(sideSplitWidget);
        sideSplitter->setObjectName(QStringLiteral("sideSplitter"));
        sizePolicy2.setHeightForWidth(sideSplitter->sizePolicy().hasHeightForWidth());
        sideSplitter->setSizePolicy(sizePolicy2);
        sideSplitter->setOrientation(Qt::Vertical);
        sideSplitter->setOpaqueResize(false);
        sideSplitter->setHandleWidth(2);
        traditionalSide = new QWidget(sideSplitter);
        traditionalSide->setObjectName(QStringLiteral("traditionalSide"));
        QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(1);
        sizePolicy3.setHeightForWidth(traditionalSide->sizePolicy().hasHeightForWidth());
        traditionalSide->setSizePolicy(sizePolicy3);
        verticalLayout_6 = new QVBoxLayout(traditionalSide);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        verticalLayout_6->setContentsMargins(0, 0, 0, 0);
        traditionalLabelWidget = new QWidget(traditionalSide);
        traditionalLabelWidget->setObjectName(QStringLiteral("traditionalLabelWidget"));
        sizePolicy2.setHeightForWidth(traditionalLabelWidget->sizePolicy().hasHeightForWidth());
        traditionalLabelWidget->setSizePolicy(sizePolicy2);
        verticalLayout_8 = new QVBoxLayout(traditionalLabelWidget);
        verticalLayout_8->setObjectName(QStringLiteral("verticalLayout_8"));
        verticalLayout_8->setContentsMargins(-1, -1, -1, 0);

        verticalLayout_6->addWidget(traditionalLabelWidget);

        sideSplitter->addWidget(traditionalSide);
        overviewSide = new QWidget(sideSplitter);
        overviewSide->setObjectName(QStringLiteral("overviewSide"));
        sizePolicy2.setHeightForWidth(overviewSide->sizePolicy().hasHeightForWidth());
        overviewSide->setSizePolicy(sizePolicy2);
        overviewSide->setMaximumSize(QSize(16777215, 70));
        verticalLayout_4 = new QVBoxLayout(overviewSide);
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        overviewLabelWidget = new QWidget(overviewSide);
        overviewLabelWidget->setObjectName(QStringLiteral("overviewLabelWidget"));
        sizePolicy2.setHeightForWidth(overviewLabelWidget->sizePolicy().hasHeightForWidth());
        overviewLabelWidget->setSizePolicy(sizePolicy2);

        verticalLayout_4->addWidget(overviewLabelWidget);

        sideSplitter->addWidget(overviewSide);

        verticalLayout_2->addWidget(sideSplitter);

        splitter_3->addWidget(sideSplitWidget);
        splitWidget = new QWidget(splitter_3);
        splitWidget->setObjectName(QStringLiteral("splitWidget"));
        sizePolicy1.setHeightForWidth(splitWidget->sizePolicy().hasHeightForWidth());
        splitWidget->setSizePolicy(sizePolicy1);
        verticalLayout = new QVBoxLayout(splitWidget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        splitter = new QSplitter(splitWidget);
        splitter->setObjectName(QStringLiteral("splitter"));
        splitter->setOrientation(Qt::Vertical);
        splitter->setOpaqueResize(false);
        splitter->setHandleWidth(2);
        traditionalContainer = new QWidget(splitter);
        traditionalContainer->setObjectName(QStringLiteral("traditionalContainer"));
        QSizePolicy sizePolicy4(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy4.setHorizontalStretch(1);
        sizePolicy4.setVerticalStretch(1);
        sizePolicy4.setHeightForWidth(traditionalContainer->sizePolicy().hasHeightForWidth());
        traditionalContainer->setSizePolicy(sizePolicy4);
        traditionalContainer->setMouseTracking(true);
        horizontalLayout_3 = new QHBoxLayout(traditionalContainer);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        splitter->addWidget(traditionalContainer);
        overviewContainer = new QWidget(splitter);
        overviewContainer->setObjectName(QStringLiteral("overviewContainer"));
        overviewContainer->setEnabled(true);
        QSizePolicy sizePolicy5(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy5.setHorizontalStretch(1);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(overviewContainer->sizePolicy().hasHeightForWidth());
        overviewContainer->setSizePolicy(sizePolicy5);
        overviewContainer->setMaximumSize(QSize(16777215, 70));
        overviewContainer->setMouseTracking(true);
        horizontalLayout_5 = new QHBoxLayout(overviewContainer);
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        horizontalLayout_5->setContentsMargins(0, 0, 0, 0);
        splitter->addWidget(overviewContainer);

        verticalLayout->addWidget(splitter);

        splitter_3->addWidget(splitWidget);

        horizontalLayout_4->addWidget(splitter_3);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 1600, 25));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        menuOptions = new QMenu(menubar);
        menuOptions->setObjectName(QStringLiteral("menuOptions"));
        menuViews = new QMenu(menubar);
        menuViews->setObjectName(QStringLiteral("menuViews"));
        menuTraces = new QMenu(menubar);
        menuTraces->setObjectName(QStringLiteral("menuTraces"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuOptions->menuAction());
        menubar->addAction(menuViews->menuAction());
        menubar->addAction(menuTraces->menuAction());
        menuFile->addAction(actionOpen_Trace);
        menuFile->addAction(actionClose);
        menuFile->addAction(actionQuit);
        menuOptions->addAction(actionVisualization);
        menuViews->addAction(actionPhysical_Time);
        menuViews->addAction(actionMetric_Overview);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "Ravel", 0));
        actionOpen_JSON->setText(QApplication::translate("MainWindow", "Open Legacy JSON", 0));
        actionOpen_Trace->setText(QApplication::translate("MainWindow", "Open Trace", 0));
        actionTrace_Importing->setText(QApplication::translate("MainWindow", "Trace Importing", 0));
        actionVisualization->setText(QApplication::translate("MainWindow", "Visualization", 0));
        actionLogical_Steps->setText(QApplication::translate("MainWindow", "Logical Steps", 0));
        actionClustered_Logical_Steps->setText(QApplication::translate("MainWindow", "Clustered Logical Steps", 0));
        actionPhysical_Time->setText(QApplication::translate("MainWindow", "Physical Time", 0));
        actionMetric_Overview->setText(QApplication::translate("MainWindow", "Metric Overview", 0));
        actionQuit->setText(QApplication::translate("MainWindow", "Quit", 0));
        actionClose->setText(QApplication::translate("MainWindow", "Close", 0));
        actionSave->setText(QApplication::translate("MainWindow", "Save Current Trace", 0));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", 0));
        menuOptions->setTitle(QApplication::translate("MainWindow", "Options", 0));
        menuViews->setTitle(QApplication::translate("MainWindow", "Views", 0));
        menuTraces->setTitle(QApplication::translate("MainWindow", "Traces", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
