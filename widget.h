#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>

class QLabel;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

protected:
    void mousePressEvent(QMouseEvent *event);

private slots:
    void onClipboardUpdated();

private:
    QLabel *m_label;
    QTimer m_timer;
};

#endif // WIDGET_H
