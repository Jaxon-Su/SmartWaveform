#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QUrl>

class QNetworkReply;

class OllamaClient : public QObject
{
    Q_OBJECT

public:
    explicit OllamaClient(QObject *parent = nullptr);

    void recognizeImage(const QString &imagePath,
                        const QString &model,
                        const QString &generateUrl,
                        const QString &prompt);
    void checkServer(const QString &generateUrl, const QString &model);
    void cancelActiveRequest();

signals:
    void serverCheckFinished(bool ok, const QString &message);
    void recognitionFinished(const QString &imagePath,
                             const QByteArray &responseBody,
                             const QString &errorMessage);

private slots:
    void handleReply(QNetworkReply *reply);

private:
    QUrl tagsUrlFromGenerateUrl(const QString &generateUrl) const;
    QByteArray buildImageRequestBody(const QString &imagePath,
                                     const QString &model,
                                     const QString &prompt) const;

    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_activeRecognitionReply;
};
