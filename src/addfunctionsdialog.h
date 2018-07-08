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
    QSet<Event *> getDeletedEvents();
    ~AddFunctionsDialog();

public slots:
    void switchVisibility(int);
    void captureInput();
    void selectAll(bool);
    void addToSelectedEvents(QTableWidgetItem *);
    void captureStartTime(double);
    void captureEndTime(double);
    void captureDuration(double);
    void captureOption(int);

private:
    Ui::AddFunctionsDialog *ui;
    QList<Trace *> traces;
    unsigned long long start;
    unsigned long long end;
    unsigned long long duration;
    int option;
    QMap<int, Function *> matchingFunctions;
    QList<Event *> matchingEvents;
    QSet<Event *> selectedEvents;
    QSet<Event *> deletedEvents;
    bool allClicked;

    void getMatches(QString string);
    void filterByString(QString name);
    void filterByTime(unsigned long long start, unsigned long long end);
    void filterByDuration(unsigned long long duration, int option);
    void populateTable();
    double decideFactor(QString, QString);
};

#endif // ADDFUNCTIONSDIALOG_H
