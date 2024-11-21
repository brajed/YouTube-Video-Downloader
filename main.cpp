#include <QApplication>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEvent>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QMessageBox>

QNetworkAccessManager *manager;
QLabel *videoInfoLabel;
QPushButton *downloadButton;
QLabel *progressLabel;

void fetchVideoInfo(const QString &videoLink, QWidget *window)
{
    QUrl url("http://127.0.0.1:5000/get_video_info");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["video_link"] = videoLink;

    QNetworkReply *reply = manager->post(request, QJsonDocument(json).toJson());

    QObject::connect(reply, &QNetworkReply::finished, [reply,window](){
        if (reply->error() == QNetworkReply::NoError){
            QByteArray responseDate = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseDate);
            QJsonObject jsonObject = jsonDoc.object();
            QString videoTitle = jsonObject["title"].toString();
            videoInfoLabel->setText("Title: "+videoTitle);
            downloadButton->show();
        }
        else{
            qDebug() << "Error Fetching video Info: " << reply->errorString();
        }
        reply->deleteLater();
    });
}
void downloadVideo(const QString &videoLink, QWidget *window)
{
    QUrl url("http://127.0.0.1:5000/download_video");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["video_link"] = videoLink;

    QNetworkReply *reply = manager->post(request, QJsonDocument(json).toJson());

    QObject::connect(reply, &QNetworkReply::readyRead, [reply](){
        while(reply->canReadLine()){
            QString line = reply->readLine();
            if(line.startsWith("data:")){
                QString jsonText = line.mid(5).trimmed();
                QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonText.toUtf8());
                QJsonObject jsonObject = jsonDoc.object();

                if(jsonObject.contains("error")){
                    qDebug() << "Error Downloading Video: "<< jsonObject["error"].toString();
                }
                else{
                    double downloaded = jsonObject["downloaded"].toDouble();
                    double total = jsonObject["total"].toDouble();
                    double speed = jsonObject["speed"].toDouble();
                    int eta = jsonObject["eta"].toInt();


                    QString percent = jsonObject["percent"].toString();

                    QString progressText = QString("%1 of %2 MiB at %3 KiB/s ETA %4s ")
                                                .arg(downloaded, 0, 'f', 2)
                                                .arg(total, 0, 'f', 2)
                                                .arg(speed, 0, 'f', 2)
                                               .arg(eta);

                    progressLabel->setText(progressText);
                }
            }
        }
    });

    QObject::connect(reply, &QNetworkReply::finished, [reply, window](){
        if(reply->error() == QNetworkReply::NoError){
            QMessageBox::information(window, "Download Complete", "Video Downloaded Successfully");
        }
        else{
            qDebug() << "Error Downloading Video: Try Again - " << reply->errorString();
        }

        reply->deleteLater();
    });
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget mainWindow;
    mainWindow.setWindowTitle("YouTube Download Manager");
    mainWindow.resize(600,400);

    QVBoxLayout *layout = new QVBoxLayout(&mainWindow);

    QLabel *titleLabel = new QLabel("Your Download Manager");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size:28px; color: red");
    layout->addWidget(titleLabel);

    QLabel *instructionLabel = new QLabel("Please Input YouTube Video Link to Download");
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setStyleSheet("font-size:26px; color: green");
    layout->addWidget(instructionLabel);

    QTextEdit *linkInput = new QTextEdit();
    linkInput->setFixedSize(500,45);
    linkInput->setStyleSheet("background-color: lightgray; font-size: 18px");
    linkInput->setAcceptRichText(false);
    layout->addWidget(linkInput, 0, Qt::AlignCenter);

    QPushButton *getLinkButton = new QPushButton("Get Link");
    getLinkButton->setFixedSize(150,50);
    getLinkButton->setStyleSheet("background-color:blue; color:white; font-size: 20px");
    layout->addWidget(getLinkButton, 0, Qt::AlignCenter);

    videoInfoLabel = new QLabel("");
    videoInfoLabel ->setAlignment(Qt::AlignCenter);
    videoInfoLabel ->setStyleSheet("font-size:18px; color: green");
    layout->addWidget(videoInfoLabel );

    downloadButton = new QPushButton("Download");
    downloadButton ->setFixedSize(150,50);
    downloadButton ->setStyleSheet("background-color:red; color:white; font-size: 20px");
    downloadButton->hide();
    layout->addWidget(downloadButton , 0, Qt::AlignCenter);

    manager = new QNetworkAccessManager();
    QObject::connect(getLinkButton, &QPushButton::clicked, [&](){
        QString videoLink = linkInput->toPlainText();
        fetchVideoInfo(videoLink, &mainWindow);
    });

    QObject::connect(downloadButton, &QPushButton::clicked, [&](){
        QString videoLink = linkInput->toPlainText();
        QWidget *downloadWindow = new QWidget;
        downloadWindow->setWindowTitle("Downloading...");
        downloadWindow->resize(400,200);

        QVBoxLayout *downloadLayout = new QVBoxLayout(downloadWindow);

        QLabel *downloadLabel = new QLabel("Downloading...");
        downloadLabel->setAlignment(Qt::AlignCenter);
        downloadLabel->setStyleSheet("font-size:18px; color: red");

        downloadLayout->addWidget(downloadLabel);

        progressLabel = new QLabel("");
        progressLabel->setAlignment(Qt::AlignCenter);
        progressLabel->setStyleSheet("font-size:18px; color: green");
        downloadLayout->addWidget(progressLabel);

        downloadWindow->show();

        downloadVideo(videoLink, downloadWindow);

    });

    mainWindow.show();
    return app.exec();
}
