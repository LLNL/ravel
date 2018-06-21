#include "addfunctionsdialog.h"
#include "ui_addfunctionsdialog.h"
#include "trace.h"
#include "function.h"
#include "event.h"

AddFunctionsDialog::AddFunctionsDialog(QWidget *parent, QList<Trace *> _traces, QSet<Event *> _filterEvents) :
    QDialog(parent),
    traces(_traces),
    filterName(""),
    allClicked(false),
    matchingFunctions(QMap<int, Function *>()),
    matchingEvents(QList<Event *>()),
    selectedEvents(_filterEvents),
    ui(new Ui::AddFunctionsDialog)
{
    ui->setupUi(this);
    ui->filterTable->setColumnWidth(0, 50);
    ui->filterStartLabel->setVisible(false);
    ui->filterStart->setVisible(false);
    ui->filterEndLabel->setVisible(false);
    ui->filterEnd->setVisible(false);

    connect(ui->filterOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(switchVisibility(int)));
    connect(ui->filterString, SIGNAL(textChanged()), this, SLOT(captureInput()));
    connect(ui->selectAllFunctions, SIGNAL(clicked(bool)), this, SLOT(selectAll(bool)));
    connect(ui->filterTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(addToSelectedEvents(QTableWidgetItem*)));
}

AddFunctionsDialog::~AddFunctionsDialog()
{
    delete ui;
}

QSet<Event *> AddFunctionsDialog::getSelectedEvents()
{
    return selectedEvents;
}

QString AddFunctionsDialog::getFilterName()
{
    return filterName;
}

void AddFunctionsDialog::switchVisibility(int option)
{
    switch(option)
    {
        case 0:
        case 1: ui->filterStringLabel->setVisible(true);
                ui->filterString->setVisible(true);
                ui->filterStartLabel->setVisible(false);
                ui->filterStart->setVisible(false);
                ui->filterEndLabel->setVisible(false);
                ui->filterEnd->setVisible(false);
                break;
        case 2: ui->filterStringLabel->setVisible(false);
                ui->filterString->setVisible(false);
                ui->filterStartLabel->setVisible(true);
                ui->filterStart->setVisible(true);
                ui->filterEndLabel->setVisible(true);
                ui->filterEnd->setVisible(true);
                break;
    }
}

void AddFunctionsDialog::captureInput()
{
    filterName = ui->filterString->toPlainText();
    if (ui->filterOptions->currentIndex() == 0)
    {
        // do something!
    }
    else if (ui->filterOptions->currentIndex() == 1)
        filterByName(filterName);
}

void AddFunctionsDialog::selectAll(bool checked)
{
    if (checked)
    {
        if (matchingEvents.size())
        {
            allClicked = true;
            for (int counter = 0; counter != matchingEvents.size(); ++counter)
            {
                QTableWidgetItem *checkBox = ui->filterTable->item(counter, 0);
                checkBox->setCheckState(Qt::Checked);
                selectedEvents.insert(matchingEvents[counter]);
            }
        }
    }
    else
    {
        if (allClicked)
        {
            allClicked = false;
            for (int counter = 0; counter != matchingEvents.size(); ++counter)
            {
                QTableWidgetItem *checkBox = ui->filterTable->item(counter, 0);
                checkBox->setCheckState(Qt::Unchecked);
            }
            selectedEvents.clear();
        }
    }
}

void AddFunctionsDialog::addToSelectedEvents(QTableWidgetItem *item)
{
    if (item->column() == 0 && item->checkState() == Qt::Checked)
        selectedEvents.insert(matchingEvents[item->row()]);
    else if (item->column() == 0 && item->checkState() == Qt::Unchecked)
    {
        if (selectedEvents.contains(matchingEvents[item->row()]))
            selectedEvents.remove(matchingEvents[item->row()]);
    }
}

void AddFunctionsDialog::filterByName(QString name)
{
    if (!this->traces.empty())
    {
        for (QList<Trace *>::Iterator trc = this->traces.begin();
             trc != this->traces.end(); ++trc)
        {
            for (QMap<int, Function *>::Iterator fnc = (*trc)->functions->begin();
                 fnc != (*trc)->functions->end(); ++fnc)
            {
                if (!QString::compare(fnc.value()->name, name, Qt::CaseInsensitive))
                {
                    matchingFunctions.insert(fnc.key(), fnc.value());
                    qDebug("Found it! " + name.toLatin1());
                }
            }
        }
        for (QMap<int, Function *>::Iterator fnc = matchingFunctions.begin();
             fnc != matchingFunctions.end(); ++fnc)
        {
            for (QList<Trace *>::Iterator trc = this->traces.begin();
                 trc != this->traces.end(); ++trc)
            {
                for (QVector<QVector<Event *> *>::Iterator eitr = (*trc)->events->begin();
                     eitr != (*trc)->events->end(); ++eitr)
                {
                    for (QVector<Event *>::Iterator itr = (*eitr)->begin();
                         itr != (*eitr)->end(); ++itr)
                    {
                        if (fnc.key() == (*itr)->function)
                        {
                            if (!matchingEvents.contains(*itr))
                                matchingEvents.append(*itr);
                        }
                    }
                }
            }
        }
        ui->filterTable->setRowCount(matchingEvents.size());
        if (!selectedEvents.empty())
        {
            foreach (Event * evt, selectedEvents)
            {
                if (matchingEvents.contains(evt))
                {
                    matchingEvents.removeAll(evt);
                }
            }
        }
        int counter = 0;
        foreach (Event * evt, matchingEvents)
        {
            QTableWidgetItem *lastItemCheckBox = new QTableWidgetItem();
            lastItemCheckBox->setCheckState(Qt::Unchecked);
            ui->filterTable->setItem(counter, 0, lastItemCheckBox);
            QTableWidgetItem *lastItemStart = new QTableWidgetItem(QString::number(evt->enter));
            ui->filterTable->setItem(counter, 1, lastItemStart);
            QTableWidgetItem *lastItemEnd = new QTableWidgetItem(QString::number(evt->exit));
            ui->filterTable->setItem(counter, 2, lastItemEnd);

            for (QList<Trace *>::Iterator trc = traces.begin();
                 trc != traces.end(); ++trc)
            {
                if ((*trc)->functions->contains(evt->function))
                {
                    QTableWidgetItem *lastItemName = new QTableWidgetItem((*trc)->functions->value(evt->function)->name);
                    ui->filterTable->setItem(counter, 3, lastItemName);
                    break;
                }
            }
            counter++;
        }
    }
}
