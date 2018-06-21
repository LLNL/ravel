#ifndef ADDFUNCTIONSDIALOG_H
#define ADDFUNCTIONSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QTableWidgetItem>

class Trace;
class Function;
class Event;

namespace Ui {
class AddFunctionsDialog;
}

class AddFunctionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddFunctionsDialog(QWidget *parent = 0,
                                QList<Trace *> _traces = QList<Trace *>(),
                                QSet<Event *> _selectedEvents = QSet<Event *>());
    QSet<Event *> getSelectedEvents();
    QString getFilterName();
    ~AddFunctionsDialog();

public slots:
    void switchVisibility(int);
    void captureInput();
    void selectAll(bool);
    void addToSelectedEvents(QTableWidgetItem *);

private:
    Ui::AddFunctionsDialog *ui;
    QList<Trace *> traces;
    QString filterName;
    QMap<int, Function *> matchingFunctions;
    QList<Event *> matchingEvents;
    QSet<Event *> selectedEvents;
    bool allClicked;

    void filterByName(QString name);
    void filterByRegex(QString regexString);
};

#endif // ADDFUNCTIONSDIALOG_H
