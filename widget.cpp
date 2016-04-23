#include "widget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QClipboard>
#include <QMimeData>
#include <QDebug>

#define WIDTH 200
#define HEIGHT 50

Widget::Widget(QWidget *parent)
    : QWidget(parent),
      m_label(new QLabel)
{
    m_label->setTextFormat(Qt::PlainText);
    m_label->setMaximumWidth(512);
    m_label->setMaximumHeight(512);
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
    setLayout(new QVBoxLayout);
    layout()->addWidget(m_label);
    layout()->setContentsMargins(0, 0, 0, 0);

    connect(qApp->clipboard(), &QClipboard::dataChanged, this, &Widget::onClipboardUpdated);
    connect(&m_timer, &QTimer::timeout, this, &QWidget::hide);

}

Widget::~Widget()
{

}

void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        hide();
    } else if (event->button()) {
        qApp->quit();
    }
}

void Widget::onClipboardUpdated()
{
    resize(WIDTH, HEIGHT);
    move(qApp->desktop()->availableGeometry(this).width() - width() - 10, 10);
    show();

    QClipboard *clip = qApp->clipboard();
    if (clip->mimeData()->hasImage()) {
        m_label->setPixmap(clip->pixmap().scaled(m_label->size(), Qt::KeepAspectRatio));
    } else {
        m_label->setText(clip->text());
    }

    m_timer.start(2000);
}
