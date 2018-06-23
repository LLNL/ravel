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
    connect(ui->filterString, SIGNAL(textChanged()), this, SLOT(captureInput()));
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
    return selectedEvents;
}

QSet<Event *> AddFunctionsDialog::getDeletedEvents()
{
    return deletedEvents;
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
    QString filterString = ui->filterString->toPlainText();
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
        {
            deletedEvents.insert(matchingEvents[item->row()]);
            selectedEvents.remove(matchingEvents[item->row()]);
        }
    }
}

void AddFunctionsDialog::getMatches(QString string)
{
    for (QList<Trace *>::Iterator trc = traces.begin();
         trc != traces.end(); ++trc)
    {
        for (QMap<int, Function *>::Iterator fnc = (*trc)->functions->begin();
             fnc != (*trc)->functions->end(); ++fnc)
        {
            switch (ui->filterOptions->currentIndex()) {
            case 0:
                if (QString(fnc.value()->name).contains(QRegExp(string)) && string.size() != 0)
                {
                    matchingFunctions.insert(fnc.key(), fnc.value());
                }
                break;
            case 1:
                if (!QString::compare(fnc.value()->name, string, Qt::CaseInsensitive))
                {
                    matchingFunctions.insert(fnc.key(), fnc.value());
                }
                break;
            }
        }
    }
}

void AddFunctionsDialog::filterByString(QString name)
{
    if (!traces.empty())
    {
        if (name.length() == 0)
        {
            matchingEvents.clear();
        }
        else
        {
            qDebug("Name length = " + QString::number(name.length()).toLatin1());
            qDebug("Matching events length = " + QString::number(matchingEvents.size()).toLatin1());
            getMatches(name);
            for (QMap<int, Function *>::Iterator fnc = matchingFunctions.begin();
                 fnc != matchingFunctions.end(); ++fnc)
            {
                for (QList<Trace *>::Iterator trc = traces.begin();
                     trc != traces.end(); ++trc)
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
        }
        populateTable();
    }
}

void AddFunctionsDialog::filterByTime(unsigned long long start, unsigned long long end)
{
    if (!traces.empty())
    {
        for (QList<Trace *>::Iterator trc = traces.begin();
             trc != traces.end(); ++trc)
        {
            for (QVector<QVector<Event *> *>::Iterator eitr = (*trc)->events->begin();
                 eitr != (*trc)->events->end(); ++eitr)
            {
                for (QVector<Event *>::Iterator itr = (*eitr)->begin();
                     itr != (*eitr)->end(); ++itr)
                {
                    if ((*itr)->enter >= start && (*itr)->exit <= end)
                    {
                        if (!matchingEvents.contains(*itr)) {
                            matchingEvents.append(*itr);
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
    ui->filterTable->setRowCount(matchingEvents.size());
    qDebug("Size = " + QString::number(matchingEvents.size()).toLatin1());
    int counter = 0;
    foreach (Event * evt, matchingEvents)
    {
        QTableWidgetItem *lastItemCheckBox = new QTableWidgetItem();
        if (selectedEvents.contains(evt))
            lastItemCheckBox->setCheckState(Qt::Checked);
        else
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
