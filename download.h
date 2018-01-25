#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QObject>
#include <QMessageBox>
#include <QSysInfo>

namespace Ui {
class download;
}

class download : public QMainWindow
{
    Q_OBJECT

public:
    explicit download(QWidget *parent = 0);
    ~download();

public:
    void startRequest(QUrl url);
    void DownloadFile(QString);

private slots:
    // slot for readyRead() signal
    void httpReadyRead();

    // slot for finished() signal from reply
    void httpDownloadFinished();

    // slot for downloadProgress()
    void updateDownloadProgress(qint64, qint64);

private:
    bool isWin = false;
    bool isLinux = false;
    bool isMac = false;
    QString folderName;
    Ui::download *ui;
    QNetworkAccessManager *manager;
    QUrl url;
    QNetworkReply *reply;
    QFile *file;
    QDataStream dataStream;
    bool httpRequestAborted;
    qint64 fileSize;
};

#endif // DOWNLOAD_H
