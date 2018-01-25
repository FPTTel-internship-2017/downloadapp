#include "download.h"
#include "ui_download.h"
#include "JlCompress.h"
#include "QProcess"
#include <QtGlobal>
#include <QDebug>

enum OperatingSytem {OS_WINDOWS, OS_UNIX, OS_LINUX, OS_MAC};

#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
OperatingSytem os = OS_WINDOWS;
#elif (defined (Q_OS_LINUX))
OperatingSytem os = OS_LINUX;
#elif (defined (Q_OS_MAC))
OperatingSytem os = OS_MAC;
#endif

/**
 * Constructor
 * @brief download::download
 * @param parent
 */
download::download(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::download)
{
    ui->setupUi(this);
    QString url = "http://192.168.33.10/fpt/";
    switch(os) {
    case OS_WINDOWS:
        folderName = "win10-x64";
        isWin = true;
        break;
    case OS_LINUX  :
        folderName = "ubuntu.14.04-x64";
        isLinux = true;
        break;
    case OS_MAC    :
        folderName = "osx.10.12-x64";
        isMac = true;
        break;
    default        :
        qDebug() << "Unknown";
        return;
    }
    DownloadFile(url + folderName + ".zip");
}

/**
 * Destructor
 * @brief download::~download
 */
download::~download()
{
    delete ui;
}

/**
 * Download a File form a Url
 * @brief download::DownloadFile
 * @param URL
 */
void download::DownloadFile(QString URL)
{
    manager = new QNetworkAccessManager(this);
    // get url
    url = URL;

    QFileInfo fileInfo(url.path());
    QString fileName = QCoreApplication::applicationDirPath() + "/" + fileInfo.fileName();

    if (fileName.isEmpty())
        fileName = "index.html";

    if (QFile::exists(fileName)) {
        if (QMessageBox::question(this, tr("HTTP"),
                                  tr("There already exists a file called %1 in "
                                     "the current directory. Overwrite?").arg(fileName),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                == QMessageBox::No)
            return;
        QFile::remove(fileName);
    }

    file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("HTTP"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(fileName).arg(file->errorString()));
        delete file;
        file = 0;
        return;
    }

    // used for progressDialog
    // This will be set true when canceled from progress dialog
    httpRequestAborted = false;

    startRequest(url);
}

/**
 * @brief download::startRequest
 * @param url
 */
void download::startRequest(QUrl url)
{
    // get() method posts a request
    // to obtain the contents of the target request
    // and returns a new QNetworkReply object
    // opened for reading which emits
    // the readyRead() signal whenever new data arrives.
    reply = manager->get(QNetworkRequest(url));

    // Whenever more data is received from the network,
    // this readyRead() signal is emitted
    connect(reply, SIGNAL(readyRead()),
            this, SLOT(httpReadyRead()));

    // Also, downloadProgress() signal is emitted when data is received
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
            this, SLOT(updateDownloadProgress(qint64, qint64)));

    // This signal is emitted when the reply has finished processing.
    // After this signal is emitted,
    // there will be no more updates to the reply's data or metadata.
    connect(reply, SIGNAL(finished()),
            this, SLOT(httpDownloadFinished()));
}

/**
 * When download finished or canceled, this will be called. When download finished normally, it would unzip file and exec another app
 * @brief download::httpDownloadFinished
 */
void download::httpDownloadFinished()
{
    // when canceled
    if (httpRequestAborted) {
        if (file) {
            file->close();
            file->remove();
            delete file;
            file = 0;
        }
        reply->deleteLater();
        //progressDialog->hide();
        return;
    }

    // download finished normally
    //progressDialog->hide();
    file->flush();
    file->close();

    // get redirection url
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (reply->error()) {
        file->remove();
        QMessageBox::information(this, tr("HTTP"),
                                 tr("Download failed: %1.")
                                 .arg(reply->errorString()));
    }
    else if (!redirectionTarget.isNull()) {
        QUrl newUrl = url.resolved(redirectionTarget.toUrl());
        if (QMessageBox::question(this, tr("HTTP"),
                                  tr("Redirect to %1 ?").arg(newUrl.toString()),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            url = newUrl;
            reply->deleteLater();
            file->open(QIODevice::WriteOnly);
            file->resize(0);
            startRequest(url);
            return;
        }
    }
    else {
        QString fileName = QFileInfo(url.path()).fileName();
        ui->lblMessage->setText(tr("Downloaded %1 to %2.").arg(fileName).arg(QCoreApplication::applicationDirPath()));

        //Unzip file was dowloaded and run another app
        QString src = QCoreApplication::applicationDirPath() + "/" + fileName;
        QString des = QCoreApplication::applicationDirPath();
        JlCompress::extractDir(src, des);

        //On UNIX, must grant permission to all file to can execute
        if(isLinux || isMac)
        {
            QProcess* process1 = new QProcess(this);
            process1->start("chmod -R 775 " + des + "/" + folderName);
            process1->waitForFinished(-1);
            delete process1;
        }
        QProcess* process2 = new QProcess(this);
        process2->startDetached(des + "/" + folderName + "/App1");
        process2->waitForStarted();
    }
    reply->deleteLater();
    reply = 0;
    delete file;
    file = 0;
    manager = 0;
    QApplication::closeAllWindows();
}

/**
 * @brief download::httpReadyRead
 */
void download::httpReadyRead()
{
    // this slot gets called every time the QNetworkReply has new data.
    // We read all of its new data and write it into the file.
    // That way we use less RAM than when reading it at the finished()
    // signal of the QNetworkReply
    if (file)
        file->write(reply->readAll());
}

/**
 * Update value of progressbar
 * @brief download::updateDownloadProgress
 * @param bytesRead
 * @param totalBytes
 */
void download::updateDownloadProgress(qint64 bytesRead, qint64 totalBytes)
{
    if (httpRequestAborted)
        return;

    ui->progressBar->setMaximum(totalBytes);
    ui->progressBar->setValue(bytesRead);
    float p = ((float)bytesRead/totalBytes)*100;
    QString value = QString::fromLatin1("%1%").arg(QString::number(p, 'f', 2));
    ui->lblMessage->setText(value);
}
