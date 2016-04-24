#ifndef WIDGET_H
#define WIDGET_H

#include <QLabel>
#include <QTimer>

class QLabel;

class Widget : public QLabel
{
    Q_OBJECT

public:
    Widget();
    ~Widget();

protected:
    void mousePressEvent(QMouseEvent *event);

private slots:
    void onClipboardUpdated();

private:
    QTimer m_timer;
};

#endif // WIDGET_H
