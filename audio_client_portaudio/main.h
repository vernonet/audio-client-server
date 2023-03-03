#ifndef MAIN_H
#define MAIN_H

#include <QtGui>
#include <QtNetwork>
#include <QApplication>
#include <QDialog>
#include <QAudioInput>
#include <QAudioOutput>
#include <QPushButton>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QComboBox>
#include <QRadioButton>
#include <QLabel>
#include <QTimer>
#include <portaudio.h>

#define SIZE_BUF_IN_SEC    (1/4)
#define SAMPLE_RATE        (48000)
#define FRAMES_PER_BUFFER   (1024)
#define SAMPLE_SIZE (2)
#define SAMPLE_SILENCE      (0)
#define MAXSRVNR            (5)

class Client : public QDialog
{
    Q_OBJECT

public:
    Client(QWidget *parent = 0);
    ~Client();


signals:
    void resultReady(QTcpSocket * socket);

private slots:
    void start();
    void stop();
    void disconnected();
    void readyRead();
    bool isAuthorized(QByteArray *requestData);
    void displayError(QAbstractSocket::SocketError socketError);
    void adjustForCurrentServers(QString &server_str);
    void updateRecentServers();
    void server_url_index_change(int index);
    int audioCallback(
            const void *inputBuffer, void *outputBuffer,
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
        return ((Client*)userData)->audioCallback(input, output, frameCount, timeInfo, statusFlags);
    }

private:
    QAudioInput *audio_in = nullptr;
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *quitButton;
    QRadioButton *rbOut;
    QRadioButton *rbIn;
    QRadioButton *rbFull;
    QStatusBar  *statusBar;
    QComboBox  *server_url;
    QLabel     *sample_lbl;
    QComboBox  *sample_rate;
    QLineEdit  *loginpass;
    QString    temp_str;
    QBuffer audioBuffer_out;
    bool started = false;
    bool authorization_passed = false;
    QTcpSocket *tcpSocket;
    QTimer *tmr;
    qintptr socdesk;
    QStringList access_data;


    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream_out=nullptr, *stream_in=nullptr;
    PaStream *stream_full=nullptr;
    PaError err;
    const PaDeviceInfo* inputInfo;
    const PaDeviceInfo* outputInfo;
    int i;
    int numBytes;
    int numChannels = 1;
    QBuffer *output_buf;
};


#endif // MAIN_H
