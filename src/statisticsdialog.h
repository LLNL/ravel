#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include <QRect>
#include <QDialog>
#include <QPainter>
#include <QMouseEvent>

namespace Ui {
class StatisticsDialog;
}

class StatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatisticsDialog(QWidget *parent = 0);
    ~StatisticsDialog();
    void paintHistogram(QPainter& painter);
    void setData(QList<double> values, QList<QString> labels);

private:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void calculateValues();
    QString getLabel(size_t i);

    Ui::StatisticsDialog *ui;
    QList<double> values;
    QList<QString> labels;
    QList<QRect> bars;
    QBrush brush;
    bool tooltip;
    int minimumValue;
    int maximumValue;
    bool showrelative;
    int margin;
    int barwidth;
    bool showlabel;
};

#endif // STATISTICSDIALOG_H
