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

Widget::Widget() : QWidget(),
      m_label(new QLabel)
{
    setLayout(new QVBoxLayout);
    layout()->addWidget(m_label);

    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    m_label->setTextInteractionFlags(Qt::TextSelectableByKeyboard);
    m_label->setTextFormat(Qt::PlainText);
    setMaximumWidth(512);
    setMaximumHeight(1024);
    setMinimumWidth(512);
    setMinimumHeight(128);
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);


    //connect(qApp->clipboard(), &QClipboard::dataChanged, this, &Widget::onClipboardUpdated);
    connect(qApp->clipboard(), &QClipboard::dataChanged, &m_updateTimer, [=]() { m_updateTimer.start(10); });
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
    const QMimeData *mime = clip->mimeData();

    if (mime->hasImage()) {
        QImage image = qvariant_cast<QImage>(clip->mimeData()->imageData());
        image = image.scaled(maximumSize(), Qt::KeepAspectRatio);
        m_label->setPixmap(QPixmap::fromImage(image));
        resize(image.size());
        updateGeometry();
        m_hasTrimmed = false;
        return;
    }

    QString text = clip->text().toHtmlEscaped();
    QString trimmed = text.trimmed();

    if (trimmed != text && !m_hasTrimmed) {
        clip->setText(trimmed);
        m_hasTrimmed = true;
        return;
    }

    m_hasTrimmed = false;

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
    updateGeometry();
}

void Widget::updateGeometry()
{
    move(qApp->desktop()->availableGeometry(this).width() - width() - 10, 10);
    show();
    setWindowOpacity(1);

    m_timer.start(5000);
}
