#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>

class QLabel;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget();
    ~Widget();

protected:
    void mousePressEvent(QMouseEvent *event);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);

private slots:
    void onClipboardUpdated();

private:
    void updateGeometry();

    QLabel *m_label;
    QTimer m_timer;
    QTimer m_updateTimer;

    bool m_hasTrimmed = false;
    int m_imageRetries = 0;
};

#endif // WIDGET_H
