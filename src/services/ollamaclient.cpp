#include "ollamaclient.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

OllamaClient::OllamaClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_network, &QNetworkAccessManager::finished,
            this, &OllamaClient::handleReply);
}

void OllamaClient::recognizeImage(const QString &imagePath,
                                  const QString &model,
                                  const QString &generateUrl,
                                  const QString &prompt)
{
    QFile file(imagePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        emit recognitionFinished(imagePath, {}, QStringLiteral("Cannot read image file: %1").arg(imagePath));
        return;
    }

    QNetworkRequest request(QUrl(generateUrl.trimmed()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setTransferTimeout(900000);

    QNetworkReply *reply = m_network.post(request, buildImageRequestBody(imagePath, model, prompt));
    reply->setProperty("kind", QStringLiteral("recognition"));
    reply->setProperty("imagePath", imagePath);
    m_activeRecognitionReply = reply;
}

void OllamaClient::checkServer(const QString &generateUrl, const QString &model)
{
    const QUrl tagsUrl = tagsUrlFromGenerateUrl(generateUrl);
    if (!tagsUrl.isValid() || tagsUrl.scheme().isEmpty() || tagsUrl.host().isEmpty()) {
        emit serverCheckFinished(false, QStringLiteral("Invalid Ollama URL: %1").arg(generateUrl));
        return;
    }

    QNetworkRequest request(tagsUrl);
    request.setTransferTimeout(15000);

    QNetworkReply *reply = m_network.get(request);
    reply->setProperty("kind", QStringLiteral("serverCheck"));
    reply->setProperty("model", model.trimmed());
}

void OllamaClient::cancelActiveRequest()
{
    if (m_activeRecognitionReply)
        m_activeRecognitionReply->abort();
}

void OllamaClient::handleReply(QNetworkReply *reply)
{
    const QString kind = reply->property("kind").toString();

    if (kind == QStringLiteral("serverCheck")) {
        const QByteArray body = reply->readAll();
        const QString model = reply->property("model").toString();
        const QString error = reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
        reply->deleteLater();

        if (!error.isEmpty()) {
            emit serverCheckFinished(false,
                                     QStringLiteral("Cannot connect to Ollama: %1. Start Ollama on this computer, or change Ollama URL to the computer running Ollama.")
                                         .arg(error));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(body);
        const QJsonArray models = doc.object().value(QStringLiteral("models")).toArray();
        QStringList names;
        for (const QJsonValue &value : models) {
            const QJsonObject object = value.toObject();
            const QString name = object.value(QStringLiteral("name")).toString(object.value(QStringLiteral("model")).toString());
            if (!name.isEmpty())
                names.append(name);
        }

        if (!model.isEmpty() && !names.contains(model)) {
            emit serverCheckFinished(false,
                                     QStringLiteral("Ollama is running, but model '%1' was not found. Install it with: ollama pull %1")
                                         .arg(model));
            return;
        }

        emit serverCheckFinished(true, QStringLiteral("Ollama OK. Model: %1").arg(model));
        return;
    }

    const QString imagePath = reply->property("imagePath").toString();
    const QByteArray body = reply->readAll();
    const QString error = reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();

    if (reply == m_activeRecognitionReply)
        m_activeRecognitionReply.clear();

    reply->deleteLater();

    emit recognitionFinished(imagePath, body, error);
}

QUrl OllamaClient::tagsUrlFromGenerateUrl(const QString &generateUrl) const
{
    QUrl url(generateUrl.trimmed());
    url.setPath(QStringLiteral("/api/tags"));
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

QByteArray OllamaClient::buildImageRequestBody(const QString &imagePath,
                                               const QString &model,
                                               const QString &prompt) const
{
    QFile file(imagePath);
    const bool opened = file.open(QIODevice::ReadOnly);

    QJsonObject payload;
    payload.insert(QStringLiteral("model"), model.trimmed());
    payload.insert(QStringLiteral("prompt"), prompt.trimmed().isEmpty() ? QStringLiteral("Text Recognition:") : prompt);
    payload.insert(QStringLiteral("stream"), false);
    payload.insert(QStringLiteral("images"), QJsonArray{
        opened ? QString::fromLatin1(file.readAll().toBase64()) : QString()
    });

    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}
