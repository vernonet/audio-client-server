#include "audioreceiver.h"
#include "ui_audioreceiver.h"
#include <QMenu>
#include <QMessageBox>
#include <portaudio.h>


AudioReceiver::AudioReceiver(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AudioReceiver)
{
    ui -> setupUi(this);
    this -> setTrayIconActions();
    this -> showTrayIcon();

    setWindowTitle("Audio server");
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &AudioReceiver::newConnection);

    readForLogindata();

    err = Pa_Initialize();
    if( err != paNoError ) qDebug() << "PortAudio error: " << Pa_GetErrorText( err );
}

void AudioReceiver::start()
{
    if (!tcpServer->listen(QHostAddress::Any, loginData[2].toInt())) {
        qDebug() << "Failed to start server.";
        return;
    }

    qDebug() << "Server started, waiting for connection...";
}

void AudioReceiver::newConnection()
{
    tcpSocket = tcpServer->nextPendingConnection();
    //tcpSocket->setReadBufferSize(1024*8);
    QString msg = "New client connected IP:" + tcpSocket->peerAddress().toString() + " PORT:" + QString::number(tcpSocket->localPort());
    qDebug() << " newConnection()...";
#ifdef SNOW_ALERTS
    trayIcon->showMessage(tr("Info"), msg);
#endif
    posic = 0;
    started = false;
    connect(tcpSocket, &QTcpSocket::readyRead, this, &AudioReceiver::readData);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &AudioReceiver::clientDisconnected);
}

void AudioReceiver::readData()
{
    QByteArray data;

    if (first) {
        data = tcpSocket->readAll();
        qDebug() << " data" << QString(data);
        // Get the Authorization header from the request
        QString authorizationHeader = getAuthorizationHeader(data);
        mode = getModeHeader(data);
        //qDebug() << " sample_rate ->" << getSampleRateHeader(data);
        setAudioFormat(getSampleRateHeader(data), SAMPLE_SIZE);

        if (checkCredentials(authorizationHeader) && mode != "ERROR") {
            // Credentials are correct, allow access to the resource
            first = false;
            qDebug() << " Authentication OK!";
            QByteArray responseData = "HTTP/1.1 200 OK\r\n"
                                      "Content-Type: text/plain\r\n"
                                      "Content-Length: 0\r\n"
                                      "\r\n";
            tcpSocket->write(responseData);
            tcpSocket->flush();

            if (!Pa_IsStreamActive (stream_full)) {
                if (mode == "OUT" || mode == "FULL") {
                     QThread::msleep(100);
                    err = Pa_StartStream( stream_full );
                    if( err != paNoError ) qDebug() << "PortAudio error: " << Pa_GetErrorText( err );
                }
            }
        }
        else {
            qDebug() << "Authentication failed!";
            // Credentials are incorrect, deny access to resource
            QByteArray responseData = "HTTP/1.1 401 Unauthorized\r\n"
                                      "Content-Type: text/plain\r\n"
                                      "Content-Length: 0\r\n"
                                      "\r\n";
            tcpSocket->write(responseData);
            tcpSocket->flush();
            tcpSocket->deleteLater();
            first = true;
        }
        data.clear();
        return;
    }

    else {

        if (!Pa_IsStreamActive (stream_full)) {
            if (mode == "IN") {
                disconnect(tcpSocket, &QTcpSocket::readyRead,this, &AudioReceiver::readData);   //not need in this mode
                err = Pa_StartStream( stream_full );
                if( err != paNoError ) qDebug() << "PortAudio error: " << Pa_GetErrorText( err );
            }

        }

        if (mode == "OUT" || mode == "FULL") {
            if (audioBuffer->pos()) {
                qDebug() << " output_buf->pos() ->" << audioBuffer->pos();
                tcpSocket->write(audioBuffer->data(), audioBuffer->pos());
                audioBuffer->seek(0);
                tcpSocket->flush();
            }
        }
    }

}


QString AudioReceiver::getAuthorizationHeader(const QByteArray& requestData)
{
    // Split the request into lines
    QStringList lines = QString(requestData).split("\r\n");

    // Find Authorization header
    for (int i=0; i<= lines.size(); i++) {
        if (lines[i].startsWith("Authorization: ")) {
            qDebug() << " getAuthorizationHeader -> " << lines[i].mid(21);
            return lines[i].mid(21);  //16
        }
    }

    // Authorization header not found
    return QString();
}

quint32 AudioReceiver::getSampleRateHeader(const QByteArray& requestData)
{
    // Split the request into lines
    QStringList lines = QString(requestData).split("\r\n");

    // Find Authorization header
    for (int i=0; i<= lines.size(); i++) {
        if (lines[i].startsWith("SampleRate: ")) {
            qDebug() << " getSampleRateHeader -> " << lines[i].midRef(12, -1);
            quint32 rate = lines[i].midRef(12, -1).toInt();
            return rate;
        }
    }

    // SampleRateHeader not found
    return 0;
}

QString AudioReceiver::getModeHeader(const QByteArray& requestData)
{
    // Split the request into lines
    QStringList lines = QString(requestData).split("\r\n");

    // Find Authorization header
    for (int i=0; i<= lines.size(); i++) {
        if (lines[i].startsWith("Mode: ")) {
            qDebug() << " getModeHeader -> " << lines[i].mid(6);
            return lines[i].mid(6);  //16
        }
    }

    // Authorization header not found
    return QString();
}


void AudioReceiver::setAudioFormat(quint32 smprate, quint16 bitpersmp)
{
    quint64 smp_format;
    switch (bitpersmp)
    {
    case 16:
        smp_format = paInt16;
        break;
    case 24:
        smp_format = paInt24;
        break;
    case 32:
        smp_format = paInt32;
        break;

    default:
        smp_format = paInt16;
        break;
    }

    audioBuffer = new QBuffer();
    audioBuffer->open(QBuffer::ReadWrite);

    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    qDebug() <<  "Input device #" << inputParameters.device ;
    inputInfo = Pa_GetDeviceInfo( inputParameters.device );
    qDebug() <<  " inputInfo ->" << inputInfo->name << inputInfo->defaultHighInputLatency << inputInfo->defaultLowInputLatency ;
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = smp_format;
    inputParameters.suggestedLatency = inputInfo->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default input device */
    qDebug() <<  "Output device #" << outputParameters.device ;
    outputInfo = Pa_GetDeviceInfo( outputParameters.device );
    qDebug() <<  " outputInfo ->" << outputInfo->name << outputInfo->defaultHighOutputLatency << outputInfo->defaultLowOutputLatency ;
    outputParameters.channelCount = 1;
    outputParameters.sampleFormat = smp_format;
    outputParameters.suggestedLatency = outputInfo->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    if (mode == "IN") {
        err = Pa_OpenStream(
                    &stream_full,
                    NULL,
                    &outputParameters,
                    smprate,
                    FRAMES_PER_BUFFER,
                    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                    &AudioReceiver::PaCallback_full, /* no callback, use blocking API */
                    this ); /* no callback, so no callback userData */
    }

    else if (mode == "OUT") {
        err = Pa_OpenStream(
                    &stream_full,
                    &inputParameters,
                    NULL,
                    smprate,
                    FRAMES_PER_BUFFER,
                    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                    &AudioReceiver::PaCallback_full, /* no callback, use blocking API */
                    this ); /* no callback, so no callback userData */
    }

    else  if (mode == "FULL") {
        err = Pa_OpenStream(
                    &stream_full,
                    &inputParameters,
                    &outputParameters,
                    smprate,
                    FRAMES_PER_BUFFER,
                    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                    &AudioReceiver::PaCallback_full, /* no callback, use blocking API */
                    this ); /* no callback, so no callback userData */
    }

    else {
        ui->statusBar->showMessage(" Audio mode not selected!", 2000);
        mode = "ERROR";
    }

}


bool AudioReceiver::checkCredentials(const QString& authorizationHeader)
{
    // Decode authorization data
    QByteArray credentialsEncoded = authorizationHeader.toUtf8();
    QByteArray credentialsDecoded = QByteArray::fromBase64(credentialsEncoded);
    QString credentials = QString::fromUtf8(credentialsDecoded);
    qDebug() << " credentials -> " << credentials;

    // Split username and password
    QStringList parts = credentials.split(":");
    QString username = parts.value(0);
    QString password = parts.value(1);

    // Check username and password
    if (username == loginData[0] && password == loginData[1]) {
        return true;
    } else {
        return false;
    }
}

void AudioReceiver::clientDisconnected() {

    ui->statusBar->showMessage("clientDisconnected", 2000);
#ifdef SNOW_ALERTS
    trayIcon->showMessage(tr("Info"), tr("Client disconnected"));
#endif
    qDebug() << " clientDisconnected()";
    tcpSocket->close();
    err = Pa_StopStream(stream_full);
    if( err != paNoError ) qDebug() << "PortAudio error: " << Pa_GetErrorText( err );
    err = Pa_CloseStream(stream_full);
    if( err != paNoError ) qDebug() << "PortAudio error: " << Pa_GetErrorText( err );
    audioBuffer->close();
    first = true;

}


int AudioReceiver::audioCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags)
{
    /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;

    int finished;

    if (mode == "OUT" || mode == "FULL") {
        audioBuffer->write((const char *)inputBuffer, framesPerBuffer * SAMPLE_SIZE);
        emit tcpSocket->readyRead();
    }
    if (mode == "IN" || mode == "FULL") {
        qDebug() << " bytesAvailable -> " << tcpSocket->bytesAvailable();// << " output_buf.pos ->" << output_buf->pos();
        QByteArray data = tcpSocket->read(framesPerBuffer * SAMPLE_SIZE);
        memcpy(outputBuffer, data.data(), framesPerBuffer * SAMPLE_SIZE);
    }

    finished = paContinue;

    return finished;
}


void AudioReceiver::showTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    QIcon trayImage(":/1234.png");
    trayIcon -> setIcon(trayImage);
    trayIcon -> setContextMenu(trayIconMenu);
    trayIcon->setToolTip("audio server");

    connect(trayIcon, &QSystemTrayIcon::activated, this, &AudioReceiver::trayIconActivated);

    trayIcon->setVisible(true);
    trayIcon -> show();
}

void AudioReceiver::trayActionExecute()
{
    //QMessageBox::information(this, "TrayIcon", "Bla bla bla bla bla bla bla bla bla bla bla......");
    //showNormal();
}

void AudioReceiver::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            this -> trayActionExecute();
            break;
        default:
            break;
    }
}

void AudioReceiver::setAccess()
{
    QString loginpass;
    bool ok = false;
    while (loginpass.count(":") != 2 || loginData[2].toInt() == 0) {
        if (loginpass.size() != 0) {
            QMessageBox msgBox;
            msgBox.setText("You entered incorrect data, please try again");
            msgBox.exec();
        }
        loginpass = QInputDialog::getText(this, tr("Accses settings"), tr("Enter login:password:port"), QLineEdit::Normal, loginData.join(":"), &ok);
        if (ok && !loginpass.isEmpty()) {
            loginData = QString(loginpass).split(":");
            tcpServer->close();
            if (!tcpServer->listen(QHostAddress::Any, loginData[2].toInt())) {
                qDebug() << "Failed to start server.";
                return;
            }
            updateLogindata(loginData.join(":"));
            qDebug() << "Server restarted, waiting for connection...";
        }
        if (!ok) return;
    }

}

void AudioReceiver::setTrayIconActions()
{
    // Setting actions...
    minimizeAction = new QAction("minimize", this);
    restoreAction = new QAction("Restore", this);
    quitAction = new QAction("Quit", this);
    accessAction = new QAction("Access options", this);

    // Connecting actions to slots...
    connect (minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));
    connect (restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));
    connect (quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect (accessAction, &QAction::triggered, this, &AudioReceiver::setAccess);

    // Setting system tray's icon menu...
    trayIconMenu = new QMenu(this);
    trayIconMenu -> addAction (accessAction);
    trayIconMenu -> addAction (minimizeAction);
    trayIconMenu -> addAction (restoreAction);
    trayIconMenu -> addAction (quitAction);
}

void AudioReceiver::readForLogindata(){

    QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::UserScope, ".");
    QSettings settings("login_data.ini", QSettings::IniFormat);
    QStringList login_data = settings.value("login_data").toStringList();
    if (login_data.size() && login_data[0].size())  loginData = login_data;
      else loginData = QString("user:password:8082").split(":");

}

void AudioReceiver::updateLogindata(QString str){

    QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::UserScope, ".");
    QSettings settings("login_data.ini", QSettings::IniFormat);
    QStringList login_data = QString(str).split(":");
    if (!settings.isWritable() || settings.status() != QSettings::NoError)
              qDebug() << " error ->" << settings.status();
    settings.setValue("login_data", login_data);
    settings.sync();
}

void AudioReceiver::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event -> type() == QEvent::WindowStateChange)
    {
        if (isMinimized())
        {
            this -> hide();
        }
    }
}

AudioReceiver::~AudioReceiver()
{
    delete ui;
}
