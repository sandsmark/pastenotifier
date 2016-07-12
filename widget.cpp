#include "widget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QClipboard>
#include <QMimeData>
#include <QDebug>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QDir>

Widget::Widget() : QWidget(),
      m_label(new QLabel)
{
    setLayout(new QVBoxLayout);
    layout()->addWidget(m_label);

    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_label->setTextFormat(Qt::PlainText);
    setMaximumWidth(512);
    setMaximumHeight(1024);
    setMinimumWidth(512);
    setMinimumHeight(128);
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);


    connect(qApp->clipboard(), &QClipboard::dataChanged, this, &Widget::onClipboardUpdated);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &QWidget::hide);

     QString stylesheet(
                "QLabel {\n"
                "    border: 3px solid white;\n"
                "    background-color: rgba(0, 0, 0, 128);\n"
                "    selection-color: black;\n"
                "    selection-background-color: white;\n"
                "}\n"
                );

    const QString configLocation = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configLocation);
    if (!configDir.exists()) {
        configDir.mkpath(configLocation);
    }

    QString stylePath = configDir.absoluteFilePath("pastenotifier.qss");
    QFile styleFile(stylePath);
    if (styleFile.exists()) {
        if (styleFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Loading stylesheet" << stylePath;
            stylesheet = QString::fromLocal8Bit(styleFile.readAll());
        } else {
            qWarning() << "Unable to open qss file:" << stylePath << styleFile.errorString();
        }
    } else {
        if (styleFile.open(QIODevice::WriteOnly)) {
            styleFile.write(stylesheet.toUtf8());
        } else {
            qWarning() << "Unable to open qss file for writing:" << stylePath << styleFile.errorString();
        }
    }

    setStyleSheet(stylesheet);

    onClipboardUpdated();
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
    QClipboard *clip = qApp->clipboard();

    if (clip->mimeData()->hasImage()) {
        QPixmap image = clip->pixmap().scaled(maximumSize(), Qt::KeepAspectRatio);
        m_label->setPixmap(image);
        resize(image.size());
    } else {
        QString text = clip->text().toHtmlEscaped();

        QFontMetrics metrics(font());
        resize(metrics.size(Qt::TextExpandTabs, text));

        QString newText;
        int h = metrics.height() * 2;
        for (const QString &line : text.split('\n')) {
            newText.append(metrics.elidedText(line, Qt::ElideRight, width() - 10) + '\n');
            if (h > height()) {
                break;
            }
            h += metrics.height();
        }
        m_label->setText(newText);
        m_label->setSelection(0, newText.length());
    }

    move(qApp->desktop()->availableGeometry(this).width() - width() - 10, 10);
    show();

    m_timer.start(5000);
}
