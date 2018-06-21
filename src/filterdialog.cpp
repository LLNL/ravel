#include "filterdialog.h"
#include "ui_filterdialog.h"
#include "addfunctionsdialog.h"
#include "trace.h"
#include "event.h"
#include "function.h"
#include <QDebug>

FilterDialog::FilterDialog(QWidget *parent, QList<Trace *> _traces, QSet<Event *> _filterEvents) :
    QDialog(parent),
    traces(_traces),
    filterApplied(false),
    addFuncDialog(NULL),
    filterEvents(_filterEvents),
    ui(new Ui::FilterDialog)
{
    ui->setupUi(this);
    if (!filterEvents.empty())
    {
        foreach (Event * evt, filterEvents)
        {
            ui->infoFunctions->insertRow(ui->infoFunctions->rowCount());
            QTableWidgetItem *lastItemStart = new QTableWidgetItem(QString::number(evt->enter));
            ui->infoFunctions->setItem(ui->infoFunctions->rowCount() - 1, 0, lastItemStart);
            QTableWidgetItem *lastItemEnd = new QTableWidgetItem(QString::number(evt->exit));
            ui->infoFunctions->setItem(ui->infoFunctions->rowCount() - 1, 1, lastItemEnd);

            for (QList<Trace *>::Iterator trc = traces.begin();
                 trc != traces.end(); ++trc)
            {
                if ((*trc)->functions->contains(evt->function))
                {
                    QTableWidgetItem *lastItemName = new QTableWidgetItem((*trc)->functions->value(evt->function)->name);
                    ui->infoFunctions->setItem(ui->infoFunctions->rowCount() - 1, 2, lastItemName);
                    break;
                }
            }
        }
    }
    connect(ui->addFunctions, SIGNAL(clicked()), this, SLOT(openAddFunctionsDialog()));
    connect(ui->removeFunctions, SIGNAL(clicked()), this, SLOT(openRemoveFunctionsDialog()));
    connect(ui->importFunctions, SIGNAL(clicked()), this, SLOT(openImportFunctionsDialog()));
}

FilterDialog::~FilterDialog()
{
    delete ui;
}

QSet<Event *> FilterDialog::getFilterEvents()
{
    return filterEvents;
}

void FilterDialog::openAddFunctionsDialog()
{
    delete addFuncDialog;
    qDebug(ui->addFunctions->text().toLatin1());
    if (!filterEvents.empty())
        addFuncDialog = new AddFunctionsDialog(this, traces, filterEvents);
    else
        addFuncDialog = new AddFunctionsDialog(this, traces);
    int dialogCode = addFuncDialog->exec();
    if (dialogCode == QDialog::Accepted)
    {
        QSet<Event *> selectedEvents = addFuncDialog->getSelectedEvents();
        if (!selectedEvents.empty())
        {
            foreach (Event * evt, selectedEvents)
            {
                if (!filterEvents.contains(evt))
                {
                    ui->infoFunctions->insertRow(ui->infoFunctions->rowCount());
                    QTableWidgetItem *lastItemStart = new QTableWidgetItem(QString::number(evt->enter));
                    ui->infoFunctions->setItem(ui->infoFunctions->rowCount() - 1, 0, lastItemStart);
                    QTableWidgetItem *lastItemEnd = new QTableWidgetItem(QString::number(evt->exit));
                    ui->infoFunctions->setItem(ui->infoFunctions->rowCount() - 1, 1, lastItemEnd);

                    for (QList<Trace *>::Iterator trc = traces.begin();
                         trc != traces.end(); ++trc)
                    {
                        if ((*trc)->functions->contains(evt->function))
                        {
                            QTableWidgetItem *lastItemName = new QTableWidgetItem((*trc)->functions->value(evt->function)->name);
                            ui->infoFunctions->setItem(ui->infoFunctions->rowCount() - 1, 2, lastItemName);
                            break;
                        }
                    }
                    filterEvents.insert(evt);
                }
            }
            filterApplied = true;
        }
    }
}

void FilterDialog::openRemoveFunctionsDialog()
{
    qDebug(ui->removeFunctions->text().toLatin1());
}

void FilterDialog::openImportFunctionsDialog()
{
    qDebug(ui->importFunctions->text().toLatin1());
}
