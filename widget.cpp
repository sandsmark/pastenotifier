#include "widget.h"

#include <KGlobalAccel>

#include <QAction>
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
#include <QPainter>

Widget::Widget() : QWidget(),
      m_label(new QLabel)
{
    setLayout(new QVBoxLayout);
    layout()->addWidget(m_label);

    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    m_label->setTextInteractionFlags(Qt::TextSelectableByKeyboard);
    m_label->setTextFormat(Qt::PlainText);
    m_label->setWordWrap(true);
    layout()->setMargin(0);
    setMaximumWidth(1000);
    setMaximumHeight(1000);
    setMinimumWidth(500);
    setMinimumHeight(128);
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_label->setMargin(5);

    QFont font;
    font.setPixelSize(15);
    setFont(font);

    connect(qApp->clipboard(), &QClipboard::dataChanged, &m_updateTimer, [=]() {
        if (!m_updateTimer.isActive()) {
            m_updateTimer.start(100);
        }
    });

    connect(&m_updateTimer, &QTimer::timeout, this, &Widget::onClipboardUpdated);
    m_updateTimer.setSingleShot(true);

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

    QAction *showAction = new QAction(this);
    connect(showAction, &QAction::triggered, [=](){
        setWindowOpacity(1);
        show();
        m_timer.start(5000);
    });
    showAction->setObjectName("showpastenotifier");
    KGlobalAccel::setGlobalShortcut(showAction, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_V));

    setStyleSheet(stylesheet);

    onClipboardUpdated();
}

Widget::~Widget()
{
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_timer.stop();
        hide();
    } else if (event->button()) {
        qApp->quit();
    }
}

void Widget::enterEvent(QEvent *)
{
    setWindowOpacity(0.3);
}

void Widget::leaveEvent(QEvent *)
{
    setWindowOpacity(1);
}

void Widget::onClipboardUpdated()
{
    QClipboard *clip = qApp->clipboard();

    if (clip->mimeData()->hasImage()) {
        QImage image = clip->image();

        if (image.isNull()) {
            // Workaround for what I think is a bug introduced in qt base with
            // commit 730cbad8824bcfcb7ab60371a6563cfb6dd5658d
            // It means that the entire image isn't fetched when we first
            // request it, so we have to retry later.
            if (m_imageRetries++ < 5) {
                qWarning() << "Got null image, retry number" << m_imageRetries;
                if (!m_updateTimer.isActive()) {
                    m_updateTimer.start(100);
                }
                return;
            }

            qWarning() << "Got null image, too many retries";
            m_imageRetries = 0;

            QFont f = font();
            f.setPixelSize(15);

            const QString text = tr("Failed to fetch image from clipboard");
            QRect rect = QFontMetrics(f).boundingRect(text).marginsAdded(QMargins(5, 5, 5, 5));
            rect.moveTo(2, 2);

            image = QImage(rect.size(), QImage::Format_ARGB32);
            image.fill(Qt::red);
            QPainter p(&image);

            p.setFont(f);
            f.setBold(true);
            p.setPen(Qt::white);
            p.drawText(rect, text);
        } else {
            QSize origSize = image.size();
            const int minWidth = minimumWidth() - 20;
            const int minHeight = minimumHeight() - 20;
            if (image.width() <= image.height() && image.height() < minimumHeight() - 20) {
                image = image.scaledToHeight(minHeight);
            } else if (image.width() < minWidth) {
                image = image.scaledToWidth(minWidth);
            }
            if (image.width() > maximumWidth() - 10) {
                image = image.scaledToWidth(maximumWidth() - 10);
            }
            if (image.height() > maximumHeight() - 10) {
                image = image.scaledToHeight(maximumHeight() - 10);
            }

            image = image.convertToFormat(QImage::Format_ARGB32);

            {
                QPainter p(&image);
                QFont font;
                font.setPixelSize(15);
                p.setFont(font);
                QFontMetrics metrics(font);
                QString text = QString::number(origSize.width()) + "x" + QString::number(origSize.height());
                QRect rect = metrics.boundingRect(text).marginsAdded(QMargins(5, 5, 5, 5));
                rect.moveTopLeft(QPoint(0, 0));
                p.fillRect(rect, QColor(0, 0, 0, 128));
                p.setPen(Qt::white);
                p.drawText(rect, Qt::AlignCenter, text);
            }
        }

        m_label->setPixmap(QPixmap::fromImage(image));
        resize(image.size());
        updateGeometry();
        m_hasTrimmed = false;
        m_imageRetries = 0;
        return;
    }

    m_imageRetries = 0;

    QString text = clip->text();

    const QStringList formats = clip->mimeData()->formats();
    bool onlyTextPlain = formats.contains("text/plain");
    if (onlyTextPlain) {
        for (const QString &format : formats) {
            // "Special" fake mimetypes to indicate multiselections etc.
            if (!format.contains('/') || format.toUpper() == format) {
                continue;
            }
            if (format == "text/plain" || format == "text/html") {
                continue;
            }
            onlyTextPlain = false;
            break;
        }

        QString trimmed = text.trimmed();

        if (onlyTextPlain && trimmed != text && !m_hasTrimmed) {
            clip->setText(trimmed);
            m_hasTrimmed = true;
            return;
        }
    }

    m_hasTrimmed = false;

    QFontMetrics metrics(font());

    QString filteredText;
    for (const QChar &c : text) {
        if ((c.isPrint() || c == '\n' || c == '\t') && (c.script() == QChar::Script_Latin || c.script() == QChar::Script_Common)) {
            filteredText += c;
            continue;
        }

        qDebug() << c << c.script() << c.category();

        filteredText += "0x" + QString::number(c.unicode(), 16);
    }
    text = filteredText;

    QRect boundingRect = QRect(0, 0, maximumWidth(), maximumHeight());
    boundingRect = metrics.boundingRect(boundingRect, Qt::TextWordWrap, text);
    boundingRect = boundingRect.marginsAdded(QMargins(20, 20, 20, 20));

    if (boundingRect.height() > maximumHeight() || boundingRect.width() > maximumWidth()) {
        m_label->setWordWrap(false);

        QString newText;
        int h = metrics.height() * 2;
        for (const QString &line : text.split('\n')) {
            newText.append(metrics.elidedText(line, Qt::ElideRight, maximumWidth() - 20) + '\n');
            h += metrics.height();
        }
        text = newText;
        boundingRect.setHeight(h);
    } else {
        m_label->setWordWrap(true);
    }
    resize(boundingRect.size());

    m_label->setText(text);
    m_label->setSelection(0, text.length());
    updateGeometry();
}

void Widget::updateGeometry()
{
    move(qApp->desktop()->availableGeometry(this).width() - width() - 10, 10);
    show();
    setWindowOpacity(1);

    m_timer.start(5000);
}
