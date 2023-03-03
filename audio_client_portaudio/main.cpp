#include <QtGui>
#include <QtNetwork>
#include <QApplication>
#include <QDialog>
#include <QAudioInput>
#include <QAudioOutput>
#include <QPushButton>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QLineEdit>
#include <QLabel>
#include <QRadioButton>
#include <QGroupBox>
#include <QTimer>
#include <QAbstractItemView>
#include <main.h>
#include <portaudio.h>


QT_BEGIN_NAMESPACE
class QAudioInput;
QT_END_NAMESPACE


Client::Client(QWidget *parent)
    : QDialog(parent)
{
    startButton = new QPushButton(tr("Start"));
    stopButton = new QPushButton(tr("Stop"));
    quitButton = new QPushButton(tr("Quit"));
    stopButton->setEnabled(false);
#ifdef __ANDROID__  //for android //__x86_64__
    startButton->setMinimumHeight(100);
    stopButton->setMinimumHeight(100);
    quitButton->setMinimumHeight(100);
#endif
    statusBar  = new QStatusBar();
    statusBar->setStyleSheet("color: gray; text-align: center");
    server_url = new QComboBox();
    loginpass = new QLineEdit();
    sample_lbl = new QLabel();
    sample_rate = new QComboBox();
    //loginpass->setText("user:password");
    sample_lbl->setText("SampleRates");

    QHBoxLayout *rbuttonLayot = new QHBoxLayout;
    rbOut= new QRadioButton("OUT");
    rbOut->setToolTip("remote mic to local audio out");
    rbIn= new QRadioButton("IN");
    rbIn->setToolTip("local mic to remote audio out");
    rbFull= new QRadioButton("FULL");
    rbFull->setToolTip("full duplex");
    rbuttonLayot->addWidget(rbOut);
    rbuttonLayot->addWidget(rbIn);
    rbuttonLayot->addWidget(rbFull);

#ifdef __ANDROID__
    server_url->setMinimumHeight(100);
    QFont font;
    font.setPointSize(23);
    server_url->setFont(font);
    loginpass->setFont(font);
    sample_lbl->setFont(font);
    sample_rate->setFont(font);
    rbOut->setFont(font);
    rbIn->setFont(font);
    rbFull->setFont(font);
#endif
//    server_url->addItem("http://192.168.1.100:8082");
//    server_url->addItem("http://127.0.0.1:8082");
//    server_url->addItem("http://192.168.123.1:8082");
    //updateRecentServers();
    QString temp = "";
    adjustForCurrentServers(temp);
    server_url->setEditable(true);

    const auto deviceInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (const QAudioDeviceInfo &deviceInfo : deviceInfos)
        qDebug() << "Device name: " << deviceInfo.deviceName();
    QAudioDeviceInfo infos(QAudioDeviceInfo::defaultInputDevice());
    QList<int> sampleList = infos.supportedSampleRates();
    qDebug() << " sampleList -> " << sampleList << " sampleList.size ->" << sampleList.size();
    for (int i=0; i<sampleList.size(); i++) {
#ifdef __ANDROID__
        if (sampleList[i] == 32000) continue;  //not worked on my smartphone
 #endif
        sample_rate->addItem(QString::number(sampleList[i]));
    }
  //sample_rate->setCurrentIndex(6);
#ifdef __ANDROID__
    if (sampleList.size() == 0) {
        sample_rate->addItem("16000");
        sample_rate->addItem("22050");
        //sample_rate->addItem("32000");
        sample_rate->addItem("44100");
        sample_rate->addItem("48000");
        sample_rate->setCurrentIndex(4);
    }
    server_url->adjustSize();
    loginpass->adjustSize();
    // server_url->setMaximumWidth();
#endif
    QHBoxLayout *sampLayout = new QHBoxLayout;
    sampLayout->addWidget(sample_lbl);
    sampLayout->addWidget(sample_rate);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(quitButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(server_url);
    mainLayout->addWidget(loginpass);
    mainLayout->addLayout(sampLayout);
    mainLayout->addLayout(rbuttonLayot);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(statusBar);

    setLayout(mainLayout);
    rbFull->click();

    setWindowTitle(tr("Audio Client"));

    connect(startButton, &QPushButton::clicked, this, &Client::start);
    connect(stopButton, &QPushButton::clicked, this, &Client::stop);
    connect(quitButton, &QPushButton::clicked, this, &QDialog::close);
    connect(server_url, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Client::server_url_index_change);


#if __ANDROID__
    //get permission - RECORD_AUDIO
    QAudioFormat format_in;
    format_in.setSampleRate(sample_rate->currentText().toInt());
    format_in.setChannelCount(1);
    format_in.setSampleSize(16);
    format_in.setCodec("audio/pcm");
    format_in.setByteOrder(QAudioFormat::LittleEndian);
    format_in.setSampleType(QAudioFormat::SignedInt);
    audio_in = new QAudioInput(format_in);    //local mic to remote audio out
    audio_in->setBufferSize(SIZE_BUF_IN_SEC * ((format_in.sampleRate() * format_in.sampleSize()/8)));
    output_buf = new QBuffer();
    output_buf->open(QBuffer::ReadWrite);
    audio_in->start(output_buf);
//    while (audio_in->state() != QAudio::State::ActiveState) {
//        QThread::msleep(5);
//    }
    audio_in->stop();
    output_buf->close();
    qDebug() << " Permission granted!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ";
    //get permission - RECORD_AUDIO
#endif

    err = Pa_Initialize();
    if( err != paNoError ) qDebug() << "PortAudio error: " << Pa_GetErrorText( err );


}

void Client::start()
{
    server_url->view()->setDisabled(true);
    authorization_passed = false;
    tcpSocket = new QTcpSocket(this);
    //tcpSocket->setReadBufferSize(8196);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &Client::disconnected);
    connect(tcpSocket, &QTcpSocket::readyRead,this, &Client::readyRead);

#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    connect(tcpSocket, &QAbstractSocket::error,this, &Client::displayError);
#else
    connect(tcpSocket, &QAbstractSocket::errorOccurred,this, &Client::displayError);
#endif

    output_buf = new QBuffer();
    output_buf->open(QBuffer::ReadWrite);

    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    qDebug() <<  "Input device #" << inputParameters.device ;
    inputInfo = Pa_GetDeviceInfo( inputParameters.device );
    qDebug() <<  " inputInfo ->" << inputInfo->name << inputInfo->defaultHighInputLatency << inputInfo->defaultLowInputLatency ;
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = inputInfo->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default input device */
    qDebug() <<  "Output device #" << outputParameters.device ;
    outputInfo = Pa_GetDeviceInfo( outputParameters.device );
    qDebug() <<  " outputInfo ->" << outputInfo->name << outputInfo->defaultHighOutputLatency << outputInfo->defaultLowOutputLatency ;
    outputParameters.channelCount = 1;
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = outputInfo->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    if (rbIn->isChecked()) {
        err = Pa_OpenStream(
                    &stream_full,
                    &inputParameters,
                    NULL,
                    sample_rate->currentText().toInt(),
                    FRAMES_PER_BUFFER,
                    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                    &Client::PaCallback_full, /* no callback, use blocking API */
                    this ); /* no callback, so no callback userData */
    }

    else if (rbOut->isChecked()) {
        err = Pa_OpenStream(
                    &stream_full,
                    NULL,
                    &outputParameters,
                    sample_rate->currentText().toInt(),
                    FRAMES_PER_BUFFER,
                    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                    &Client::PaCallback_full, /* no callback, use blocking API */
                    this ); /* no callback, so no callback userData */
    }

    else  if (rbFull->isChecked()) {
        err = Pa_OpenStream(
                    &stream_full,
                    &inputParameters,
                    &outputParameters,
                    sample_rate->currentText().toInt(),
                    FRAMES_PER_BUFFER,
                    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                    &Client::PaCallback_full, /* no callback, use blocking API */
                    this ); /* no callback, so no callback userData */
    }

    else {
        statusBar->showMessage(" Audio mode not selected, using FULL mode", 2000);
        err = Pa_OpenStream(
                            &stream_full,
                            &inputParameters,
                            &outputParameters,
                            sample_rate->currentText().toInt(),
                            FRAMES_PER_BUFFER,
                            paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                            &Client::PaCallback_full, /* no callback, use blocking API */
                            this ); /* no callback, so no callback userData */
    }

    if( err != paNoError ) {
        temp_str = "PortAudio error: " + QString(Pa_GetErrorText( err ));
        qDebug() << temp_str;
        statusBar->showMessage(temp_str, 2000);
    }

    disconnect(server_url, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Client::server_url_index_change);
    sample_rate->setDisabled(true);
    QString url_str = server_url->currentText();
    QString tmp = server_url->currentText() + "@" + loginpass->text();
    adjustForCurrentServers(tmp);
    uint16_t url_port;
    QString  url_host;
    if (url_str.contains("http://") && url_str.count(":") == 2){
        url_host = url_str.mid(7, url_str.indexOf(":", 7) - 7);
        url_port = url_str.mid(url_str.indexOf(":", 7) + 1, -1).toShort();
        //QString login = url_str.mid(7, url_str.indexOf(":", 7) - 7);
        //QString pass =  url_str.mid(url_str.indexOf(":", 7) + 1, url_str.indexOf("@") - (url_str.indexOf(":", 7) + 1));
        //qDebug() << ":" << url_str.indexOf(":") << "login ->" << login << "pass" << pass;
        qDebug() << " url_host -> " << url_host << " url_port -> " << url_port;

    }
    else {
        statusBar->showMessage("wrong url", 2000);
        return;
    }
    tcpSocket->connectToHost(QHostAddress(url_host), url_port);  //127.0.0.1
    if (tcpSocket->waitForConnected(2000)) {
        qDebug() << " Connected! " ;
        statusBar->showMessage(" Connected! ", 2000);
        // add basic authentication header
        QString userPass = loginpass->text();
        QByteArray auth = userPass.toLocal8Bit().toBase64();
        QByteArray smpr = sample_rate->currentText().toLocal8Bit();
        QString mode;
        if (rbIn->isChecked())   mode = "IN";
        if (rbOut->isChecked())  mode = "OUT";
        if (rbFull->isChecked()) mode = "FULL";
        QByteArray authHeader = "Authorization: Basic " + auth + "\r\n"
                                "SampleRate: " + smpr + "\r\n"
                                "Mode: " + mode.toLocal8Bit() + "\r\n"
                                                        "\r\n";
        tcpSocket->write(authHeader);

        QThread::msleep(100); //200

        startButton->setEnabled(false);
        stopButton->setEnabled(true);
    }
    else {
        qDebug() << " Not connected " ;
        statusBar->showMessage(" Not connected ", 2000);
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
    }
}

void Client::readyRead()
{
    QByteArray data;
    if (!authorization_passed) {
        data = tcpSocket->readAll();
        qDebug() << " tcpSocket->readAll() -> " << QString(data);
        if (!isAuthorized(&data)) {
            temp_str = " Unauthorized!!!";
            qDebug() << temp_str;
            statusBar->showMessage(temp_str, 2000);
        }
        else {
            temp_str = " Authorized!";
            qDebug() << temp_str;
            statusBar->showMessage(temp_str, 2000);
            authorization_passed = true;
            if (!Pa_IsStreamActive (stream_full)) {
                if (rbIn->isChecked() || rbFull->isChecked()) {
                    err = Pa_StartStream( stream_full );
                    if( err != paNoError ) {
                        temp_str = "PortAudio error: " + QString(Pa_GetErrorText( err ));
                        qDebug() << temp_str;
                        statusBar->showMessage(temp_str, 2000);
                    }
                }
            }

        }
    }
    else {
        if (!Pa_IsStreamActive (stream_full)) {
            if (rbOut->isChecked()) {
                disconnect(tcpSocket, &QTcpSocket::readyRead,this, &Client::readyRead);   //not need in this mode
                err = Pa_StartStream( stream_full );
                if( err != paNoError ) {
                    temp_str = "PortAudio error: " + QString(Pa_GetErrorText( err ));
                    qDebug() << temp_str;
                    statusBar->showMessage(temp_str, 2000);
                }
            }

        }

        if (rbIn->isChecked() || rbFull->isChecked()) {
            if (output_buf->pos()) {
                qDebug() << " output_buf->pos() ->" << output_buf->pos();
                tcpSocket->write(output_buf->data(), output_buf->pos());
                output_buf->seek(0);
                tcpSocket->flush();
            }
        }
    }
}

void Client::stop()
{
   qDebug() << " STOP ";
    if (stopButton->isEnabled()){
        tcpSocket->close();
        err = Pa_StopStream(stream_full);
        if( err != paNoError ) qDebug() << "PortAudio error: " << Pa_GetErrorText( err );
        err = Pa_CloseStream(stream_full);
        if( err != paNoError ) qDebug() << "PortAudio error: " << Pa_GetErrorText( err );
        output_buf->close();
    }
    server_url->view()->setDisabled(false);
    temp_str = " Stop -> Disconnected!";
    statusBar->showMessage(temp_str, 2000);
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    temp_str.clear();
    sample_rate->setDisabled(false);
    connect(server_url, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Client::server_url_index_change);

}

bool Client::isAuthorized(QByteArray  *requestData)
{
    // Split the query into lines
    QStringList lines = QString(*requestData).split("\r\n");

    // Find Authorization header
    for (int i=0; i< lines.size(); i++) {
        if (lines[i].contains("Unauthorized")) {
            return false;
        }
        if (lines[i].startsWith("HTTP/1.1 200 OK")) {
            return true;
        }
    }
    return false;
}


void Client::disconnected()
{

    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    if (temp_str == " Unauthorized!!!") {
        temp_str += " -> disconnected..." ;
    }
     else temp_str = " disconnected...";
    statusBar->showMessage(temp_str, 2000);
    qDebug() << temp_str;
    temp_str.clear();
    sample_rate->setDisabled(false);
}


void Client::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        qDebug() <<
                    tr("The host was not found. Please check the "
                       "host name and port settings.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        qDebug() <<
                    tr("The connection was refused by the peer. "
                       "Make sure the server is running, "
                       "and check that the host");
        break;
    case QAbstractSocket::SocketAccessError:
        qDebug() <<
                    tr("The socket operation failed because the "
                       "application lacked the required privileges.");
        break;
    case QAbstractSocket::SocketResourceError:
        qDebug() <<
                    tr("The local system ran out of resources.");
        break;
    case QAbstractSocket::SocketTimeoutError:
        qDebug() <<
                    tr("The socket operation timed out.");
        break;
    default:
        qDebug() <<
                    tr("The following error occurred: %1.")
                    .arg(tcpSocket->errorString());
    }
}

int Client::audioCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags)
{ 
    /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;

    int finished;

    if (rbIn->isChecked() || rbFull->isChecked())  {
        output_buf->write((const char *)inputBuffer, framesPerBuffer * SAMPLE_SIZE);
        emit tcpSocket->readyRead();
    }
    if (rbOut->isChecked() || rbFull->isChecked())  {
        qDebug() << " bytesAvailable -> " << tcpSocket->bytesAvailable();// << " output_buf.pos ->" << output_buf->pos();
        tcpSocket->read((char *)outputBuffer, framesPerBuffer * SAMPLE_SIZE);
        //QByteArray data = tcpSocket->read( framesPerBuffer * SAMPLE_SIZE);;
        //memcpy(outputBuffer, data.data(), framesPerBuffer * SAMPLE_SIZE);
    }


    finished = paContinue;

    return finished;
}

void Client::adjustForCurrentServers(QString &server_str){

    QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::UserScope, ".");
    QSettings settings("recent_srv.ini", QSettings::IniFormat);
    QStringList recentServers =
            settings.value("recentServers").toStringList();

    if (server_str != "") {
        for (int i = 0; i < recentServers.size();) {
            qDebug() << " recentServers[i] ->" <<  recentServers[i] << server_str.split("@")[0] << recentServers.size();
            if (recentServers[i].contains(server_str.split("@")[0])){
                recentServers.removeAt(i);
                qDebug() << " removed";
            }
            else i++;
        }
        if (recentServers.size()) recentServers.prepend(server_str);
    }
    while (recentServers.size() > MAXSRVNR)
        recentServers.removeLast();
    if (!settings.isWritable() || settings.status() != QSettings::NoError)
        qDebug() << " error ->" << settings.status();
    if (recentServers.size() > 0) {
        settings.setValue("recentServers", recentServers);
    }
    else {  //if the file is empty
        qDebug() << recentServers.size();
        recentServers.append("http://192.168.1.100:8082@user:password");
        recentServers.append("http://127.0.0.1:8082@user:password");
        recentServers.append("http://192.168.123.1:8082@user:password");
        settings.setValue("recentServers", recentServers);
        qDebug() << " fill defaults servers";
    }
    //settings.setValue("loginPass", loginpass->text());
    settings.sync();
    updateRecentServers();
}

void Client::updateRecentServers(){

    QStringList tmp;
    QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::UserScope, ".");
    QSettings settings("recent_srv.ini", QSettings::IniFormat);

    access_data = settings.value("recentServers").toStringList();

    QStringList recentServers_ =
            settings.value("recentServers").toStringList();

    auto itEnd = 0u;
    if(recentServers_.size() <= MAXSRVNR)
        itEnd = recentServers_.size();
    else
        itEnd = MAXSRVNR;

    server_url->clear();
    loginpass->clear();

    for (auto i = 0u; i < itEnd; ++i) {
        server_url->addItem(recentServers_.at(i).split("@")[0]);
        qDebug() << " server_url->addItem ->" << recentServers_.at(i).split("@")[0];
    }

    if (recentServers_.at(0).split("@").size() > 1) {
        tmp = access_data[0].split("@");
        loginpass->setText(tmp[1]);
    }
    else loginpass->setText("user:password");

}

  void Client::server_url_index_change (int index) {

      qDebug() << " server_url_ind_change";
      QStringList tmp = access_data.at(index).split("@");
      if (tmp.size() > 1)  loginpass->setText(tmp[1]);
         else loginpass->setText("user:password");
  }

//bool Client::requestPermissions()
//{
//    const auto writeRes = QtAndroidPrivate::requestPermission(QStringLiteral("android.permission.WRITE_EXTERNAL_STORAGE"));
//    if (writeRes.result() == QtAndroidPrivate::Authorized)
//        return true;

//    return false;
//}


Client::~Client()
{
    stop();
    Client::close();
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Client client;

    client.setWindowIcon(QIcon(":/123.png"));

#ifdef __ANDROID__
    //client.showMaximized();
    client.show();
#else
    client.show();
#endif
    return app.exec();
}
