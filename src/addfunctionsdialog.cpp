#include "addfunctionsdialog.h"
#include "ui_addfunctionsdialog.h"
#include "trace.h"
#include "function.h"
#include "event.h"

AddFunctionsDialog::AddFunctionsDialog(QWidget *parent, QList<Trace *> _traces, QSet<Event *> _filterEvents) :
    QDialog(parent),
    traces(_traces),
    start(0),
    end(0),
    allClicked(false),
    matchingFunctions(QMap<int, Function *>()),
    matchingEvents(QList<Event *>()),
    selectedEvents(_filterEvents),
    deletedEvents(QSet<Event *>()),
    ui(new Ui::AddFunctionsDialog)
{
    ui->setupUi(this);
    ui->filterTable->setColumnWidth(0, 50);
    ui->filterStartLabel->setVisible(false);
    ui->filterStart->setVisible(false);
    ui->filterEndLabel->setVisible(false);
    ui->filterEnd->setVisible(false);

    connect(ui->filterOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(switchVisibility(int)));
    connect(ui->filterButton, SIGNAL(clicked()), this, SLOT(captureInput()));
    connect(ui->selectAllFunctions, SIGNAL(clicked(bool)), this, SLOT(selectAll(bool)));
    connect(ui->filterTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(addToSelectedEvents(QTableWidgetItem*)));
    connect(ui->filterStart, SIGNAL(valueChanged(double)), this, SLOT(captureStartTime(double)));
    connect(ui->filterEnd, SIGNAL(valueChanged(double)), this, SLOT(captureEndTime(double)));
}

AddFunctionsDialog::~AddFunctionsDialog()
{
    delete ui;
}

QSet<Event *> AddFunctionsDialog::getSelectedEvents()
{
    return this->selectedEvents;
}

QSet<Event *> AddFunctionsDialog::getDeletedEvents()
{
    return this->deletedEvents;
}

void AddFunctionsDialog::switchVisibility(int option)
{
    switch(option)
    {
        case 0:
        case 1: ui->filterStringLabel->setVisible(true);
                ui->filterString->setVisible(true);
                ui->filterButton->setVisible(true);
                ui->filterStartLabel->setVisible(false);
                ui->filterStart->setVisible(false);
                ui->filterEndLabel->setVisible(false);
                ui->filterEnd->setVisible(false);
                break;
        case 2: ui->filterStringLabel->setVisible(false);
                ui->filterString->setVisible(false);
                ui->filterButton->setVisible(false);
                ui->filterStartLabel->setVisible(true);
                ui->filterStart->setVisible(true);
                ui->filterEndLabel->setVisible(true);
                ui->filterEnd->setVisible(true);
                break;
    }
}

void AddFunctionsDialog::captureInput()
{
    QString filterString = ui->filterString->toPlainText();
    this->matchingEvents.clear();
    this->matchingFunctions.clear();
    ui->filterTable->clear();
    filterByString(filterString);
}

void AddFunctionsDialog::captureStartTime(double time)
{
    if (ui->filterOptions->currentIndex() == 2)
    {
        start = time;
        if (start < end && end != 0)
        {
            filterByTime(start, end);
        }
    }
}

void AddFunctionsDialog::captureEndTime(double time)
{
    if (ui->filterOptions->currentIndex() == 2)
    {
        end = time;
        if (start < end && end != 0)
        {
            filterByTime(start, end);
        }
    }
}

void AddFunctionsDialog::selectAll(bool checked)
{
    if (checked)
    {
        if (this->matchingEvents.size())
        {
            allClicked = true;
            for (int counter = 0; counter != this->matchingEvents.size(); ++counter)
            {
                QTableWidgetItem *checkBox = ui->filterTable->item(counter, 0);
                checkBox->setCheckState(Qt::Checked);
                this->selectedEvents.insert(this->matchingEvents[counter]);
            }
        }
    }
    else
    {
        if (allClicked)
        {
            allClicked = false;
            for (int counter = 0; counter != this->matchingEvents.size(); ++counter)
            {
                QTableWidgetItem *checkBox = ui->filterTable->item(counter, 0);
                checkBox->setCheckState(Qt::Unchecked);
            }
            this->selectedEvents.clear();
        }
    }
}

void AddFunctionsDialog::addToSelectedEvents(QTableWidgetItem *item)
{
    if (item->column() == 0 && item->checkState() == Qt::Checked)
        this->selectedEvents.insert(this->matchingEvents[item->row()]);
    else if (item->column() == 0 && item->checkState() == Qt::Unchecked)
    {
        if (this->selectedEvents.contains(this->matchingEvents[item->row()]))
        {
            this->deletedEvents.insert(this->matchingEvents[item->row()]);
            this->selectedEvents.remove(this->matchingEvents[item->row()]);
        }
    }
}

void AddFunctionsDialog::getMatches(QString string)
{    
    for (QList<Trace *>::Iterator trc = this->traces.begin();
         trc != this->traces.end(); ++trc)
    {
        for (QMap<int, Function *>::Iterator fnc = (*trc)->functions->begin();
             fnc != (*trc)->functions->end(); ++fnc)
        {
            switch (ui->filterOptions->currentIndex())
            {
                case 0:
                {
                    QRegExp rx(string);
                    rx.setPatternSyntax(QRegExp::Wildcard);
                    if (rx.exactMatch(fnc.value()->name) && string.size() != 0)
                        this->matchingFunctions.insert(fnc.key(), fnc.value());
                    break;
                }
                case 1:
                {
                    if (!QString::compare(fnc.value()->name, string, Qt::CaseInsensitive))
                        this->matchingFunctions.insert(fnc.key(), fnc.value());
                    break;
                }
            }
        }
    }
}

void AddFunctionsDialog::filterByString(QString name)
{
    if (!this->traces.empty())
    {
        getMatches(name);
        for (QMap<int, Function *>::Iterator fnc = this->matchingFunctions.begin();
             fnc != this->matchingFunctions.end(); ++fnc)
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
                            if (!this->matchingEvents.contains(*itr))
                                this->matchingEvents.append(*itr);
                        }
                    }
                }
            }
        }
        populateTable();
    }
}

void AddFunctionsDialog::filterByTime(unsigned long long start, unsigned long long end)
{
    if (!this->traces.empty())
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
                    if ((*itr)->enter >= start && (*itr)->exit <= end)
                    {
                        if (!this->matchingEvents.contains(*itr)) {
                            this->matchingEvents.append(*itr);
                        }
                    }
                }
            }
        }
        populateTable();
    }
}

void AddFunctionsDialog::populateTable()
{
    ui->filterTable->setRowCount(this->matchingEvents.size());
    qDebug("Size = " + QString::number(this->matchingEvents.size()).toLatin1());
    int counter = 0;
    foreach (Event * evt, this->matchingEvents)
    {
        QTableWidgetItem *lastItemCheckBox = new QTableWidgetItem();
        if (this->selectedEvents.contains(evt))
            lastItemCheckBox->setCheckState(Qt::Checked);
        else
            lastItemCheckBox->setCheckState(Qt::Unchecked);
        ui->filterTable->setItem(counter, 0, lastItemCheckBox);
        QTableWidgetItem *lastItemStart = new QTableWidgetItem(QString::number(evt->enter));
        ui->filterTable->setItem(counter, 1, lastItemStart);
        QTableWidgetItem *lastItemEnd = new QTableWidgetItem(QString::number(evt->exit));
        ui->filterTable->setItem(counter, 2, lastItemEnd);

        for (QList<Trace *>::Iterator trc = this->traces.begin();
             trc != this->traces.end(); ++trc)
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
