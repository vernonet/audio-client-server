#include "audioreceiver.h"
#include <QApplication>

#ifndef QT_NO_SYSTEMTRAYICON

#include <QMessageBox>



int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(images);

     QApplication a(argc, argv);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(0, QObject::tr("Systray"),
                              QObject::tr("I couldn't detect any system tray "
                                          "on this system."));
        return 1;
    }
    QApplication::setQuitOnLastWindowClosed(false);


    AudioReceiver receiver;
    receiver.setWindowIcon(QIcon(":/1234.png"));
    //receiver.showMinimized();
    //receiver.show();
    receiver.start();
    return a.exec();
}

#else

#include <QLabel>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString text("QSystemTrayIcon is not supported on this platform");

    QLabel *label = new QLabel(text);
    label->setWordWrap(true);

    label->show();
    qDebug() << text;

    app.exec();
}

#endif
