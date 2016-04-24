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
#include <QDir>

Widget::Widget()
    : QLabel()
{
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus | Qt::Tool);

    setTextInteractionFlags(Qt::TextSelectableByMouse);
    setTextFormat(Qt::PlainText);
    setMaximumWidth(512);
    setMaximumHeight(1024);
    setMinimumWidth(512);
    setMinimumHeight(128);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    connect(qApp->clipboard(), &QClipboard::dataChanged, this, &Widget::onClipboardUpdated);
    connect(&m_timer, &QTimer::timeout, this, &QWidget::hide);

     QString stylesheet(
                "Widget {\n"
                "    border: 3px solid white;\n"
                "    background-color: black;\n"
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
        setPixmap(image);
        resize(image.size());
    } else {
        QString text = clip->text().toHtmlEscaped();

        QFontMetrics metrics(font());
        resize(metrics.size(Qt::TextExpandTabs, text));

        QString newText;
        int h = metrics.height() * 2;
        for (const QString &line : text.split('\n')) {
            newText.append(metrics.elidedText(line, Qt::ElideRight, width()) + '\n');
            if (h > height()) {
                break;
            }
            h += metrics.height();
        }
        setText(newText);
        setSelection(0, newText.length());
    }

    move(qApp->desktop()->availableGeometry(this).width() - width() - 10, 10);
    show();

    m_timer.start(5000);
}
