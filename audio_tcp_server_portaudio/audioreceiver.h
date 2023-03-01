#ifndef AUDIORECEIVER_H
#define AUDIORECEIVER_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QtWidgets>
#include <QtNetwork>
#include <QAudioOutput>
#include <QAudioInput>
#include <portaudio.h>

#define SIZE_AUDIO_BUF_IN_SEC    (1/4)     //1 sec
#define SAMPLE_RATE             (48000)
#define FRAMES_PER_BUFFER       (1024)
#define SAMPLE_SIZE             (2)
#define SAMPLE_SILENCE          (0)
//#define SNOW_ALERTS

namespace Ui {
class AudioReceiver;
}

class AudioReceiver : public QMainWindow
{
    Q_OBJECT

public:
    explicit AudioReceiver(QWidget *parent = nullptr);
    ~AudioReceiver();

    void start();

signals:


private slots:
    void newConnection();
    void readData();
    void clientDisconnected();

    void changeEvent(QEvent*);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void trayActionExecute();
    void setTrayIconActions();
    void showTrayIcon();
    QString getAuthorizationHeader(const QByteArray& requestData);
    quint32 getSampleRateHeader(const QByteArray& requestData);
    QString getModeHeader(const QByteArray& requestData);
    bool checkCredentials(const QString& authorizationHeader);
    void setAudioFormat(quint32 smprate, quint16 bitpersmp);
    void setAccess();
    void readForLogindata();
    void updateLogindata(QString str);
    int audioCallback( const void *inputBuffer, void *outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags);


    static int PaCallback_full(
            const void *input, void *output,
            unsigned long frameCount,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void *userData )

    {
        return ((AudioReceiver*)userData)->audioCallback(input, output, frameCount, timeInfo, statusFlags);
    }

private:
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket;
    QAudioOutput *audioOutput;
    QAudioInput *audioInput;
    QString username;
    QString password;
    Ui::AudioReceiver *ui;
    QBuffer *audioBuffer;
    QAudioFormat format;
    int posic = 0;
    bool started = false;
    bool first = true;
    QByteArray byteArray;
    QStringList loginData;
    QString mode = "FULL";

    QMenu *trayIconMenu;
    QAction *minimizeAction;
    QAction *restoreAction;
    QAction *quitAction;
    QAction *accessAction;
    QSystemTrayIcon *trayIcon;

    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream_full=nullptr;
    PaError err;
    const PaDeviceInfo* inputInfo;
    const PaDeviceInfo* outputInfo;
};

#endif // AUDIORECEIVER_H
