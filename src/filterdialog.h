#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include <QDialog>
#include <QSet>

class AddFunctionsDialog;
class Trace;
class Event;

namespace Ui {
class FilterDialog;
}

class FilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDialog(QWidget *parent = 0,
                          QList<Trace *> _traces = QList<Trace *>(),
                          QSet<Event *> _filterEvents = QSet<Event *>());
    ~FilterDialog();
    QSet<Event *> getFilterEvents();
    bool filterApplied;

public slots:
    void openAddFunctionsDialog();
    void openImportFunctionsDialog();

private:
    Ui::FilterDialog *ui;
    QList<Trace *> traces;
    QSet<Event *> filterEvents;

    // Add functions
    AddFunctionsDialog *addFuncDialog;

    // Import Functions
};

#endif // FILTERDIALOG_H
