#ifndef VERTICALLABEL_H
#define VERTICALLABEL_H

#include <QLabel>

// Adapted from:
// http://stackoverflow.com/questions/9183050/vertical-qlabel-or-the-equivalent
// Draw a vertical label with appropriately rotated text
class VerticalLabel : public QLabel
{
    Q_OBJECT
public:
    explicit VerticalLabel(QWidget *parent = 0);
    explicit VerticalLabel(const QString &text, QWidget *parent=0);
    
signals:
    
public slots:

protected:
    void paintEvent(QPaintEvent*);
    QSize sizeHint() const ;
    QSize minimumSizeHint() const;
    
};

#endif // VERTICALLABEL_H
