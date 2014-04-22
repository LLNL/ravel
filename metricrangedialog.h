#ifndef METRICRANGEDIALOG_H
#define METRICRANGEDIALOG_H

#include <QDialog>
#include <QAbstractButton>

// Dialog for changing colorbar range
namespace Ui {
class MetricRangeDialog;
}

class MetricRangeDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit MetricRangeDialog(QWidget *parent = 0, long long int current = 0, long long int original = 0);
    ~MetricRangeDialog();
    void setCurrent(long long int current);
    
signals:
    void valueChanged(long long int);

public slots:
    void onButtonClick(QAbstractButton* qab);
    void onOK();
    void onCancel();

private:
    Ui::MetricRangeDialog *ui;
    long long int start_value;
    long long int original_value;

};

#endif // METRICRANGEDIALOG_H
