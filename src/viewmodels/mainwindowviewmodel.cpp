#include "mainwindowviewmodel.h"

#include <QDir>
#include <QFileInfo>
#include <QTimer>

MainWindowViewModel::MainWindowViewModel(QObject *parent)
    : QObject(parent)
{
    connect(&m_ollama, &OllamaClient::serverCheckFinished,
            this, &MainWindowViewModel::handleServerCheckFinished);
    connect(&m_ollama, &OllamaClient::recognitionFinished,
            this, &MainWindowViewModel::handleRecognitionFinished);
}

void MainWindowViewModel::startBatch(const QString &inputRoot,
                                     const QString &outputPath,
                                     const QString &model,
                                     const QString &generateUrl,
                                     const QString &prompt)
{
    if (m_busy)
        return;

    m_stopRequested = false;
    m_waitingForServerCheck = false;
    m_currentIndex = 0;
    m_results.clear();
    m_images.clear();
    m_inputRoot = QDir::cleanPath(inputRoot);
    m_outputPath = outputPath;
    m_model = model.trimmed();
    m_generateUrl = generateUrl.trimmed();
    m_prompt = prompt.trimmed().isEmpty() ? QStringLiteral("Text Recognition:") : prompt;

    emit resultsCleared();

    m_images = m_scanner.scanPngFiles(m_inputRoot);
    if (m_images.isEmpty()) {
        emit logMessage(QStringLiteral("No supported image files found."));
        emit progressChanged(0, 0);
        emit statusChanged(QStringLiteral("Ready"));
        return;
    }

    m_busy = true;
    m_waitingForServerCheck = true;
    m_timer.restart();
    emit busyChanged(true);
    emit progressChanged(0, m_images.size());
    emit statusChanged(QStringLiteral("Checking Ollama"));
    emit logMessage(QStringLiteral("Found %1 image files.").arg(m_images.size()));
    emit logMessage(QStringLiteral("Checking Ollama URL: %1").arg(m_generateUrl));

    m_ollama.checkServer(m_generateUrl, m_model);
}

void MainWindowViewModel::stopBatch()
{
    if (!m_busy)
        return;

    m_stopRequested = true;
    m_ollama.cancelActiveRequest();
    emit logMessage(QStringLiteral("Stop requested. Current Ollama request was canceled."));
}

void MainWindowViewModel::handleServerCheckFinished(bool ok, const QString &message)
{
    if (!m_waitingForServerCheck)
        return;

    m_waitingForServerCheck = false;
    emit logMessage(message);

    if (!ok) {
        stopWithError(QStringLiteral("Ollama check failed"));
        return;
    }

    emit statusChanged(QStringLiteral("0 / %1").arg(m_images.size()));
    processNextImage();
}

void MainWindowViewModel::handleRecognitionFinished(const QString &imagePath,
                                                    const QByteArray &responseBody,
                                                    const QString &errorMessage)
{
    if (!m_busy || m_waitingForServerCheck)
        return;

    const OcrResult result = m_parser.parse(imagePath, m_inputRoot, responseBody, errorMessage);
    m_results.append(result);
    emit resultReady(result);

    if (!errorMessage.isEmpty())
        emit logMessage(QStringLiteral("Ollama error for %1: %2").arg(QFileInfo(imagePath).fileName(), errorMessage));

    ++m_currentIndex;
    emit progressChanged(m_currentIndex, m_images.size());

    QTimer::singleShot(0, this, &MainWindowViewModel::processNextImage);
}

void MainWindowViewModel::processNextImage()
{
    if (m_stopRequested || m_currentIndex >= m_images.size()) {
        finishBatch();
        return;
    }

    const QString imagePath = m_images.at(m_currentIndex);
    emit statusChanged(QStringLiteral("%1 / %2").arg(m_currentIndex + 1).arg(m_images.size()));
    emit logMessage(QStringLiteral("Processing %1").arg(QFileInfo(imagePath).fileName()));
    emit logMessage(QStringLiteral("Waiting for Ollama OCR response. The first image can take longer while the model loads."));

    m_ollama.recognizeImage(imagePath, m_model, m_generateUrl, m_prompt);
}

void MainWindowViewModel::finishBatch()
{
    QString error;
    if (m_writer.write(m_results, m_outputPath, &error)) {
        emit logMessage(QStringLiteral("Excel written: %1")
            .arg(QFileInfo(m_outputPath).dir().filePath(QFileInfo(m_outputPath).completeBaseName() + QStringLiteral(".xlsx"))));
    } else {
        emit logMessage(QStringLiteral("Excel write failed: %1").arg(error));
    }

    emit logMessage(QStringLiteral("Finished %1 images in %2 seconds.")
        .arg(m_results.size())
        .arg(m_timer.elapsed() / 1000.0, 0, 'f', 1));

    m_busy = false;
    m_waitingForServerCheck = false;
    emit statusChanged(QStringLiteral("Done"));
    emit busyChanged(false);
    emit batchFinished();
}

void MainWindowViewModel::stopWithError(const QString &message)
{
    emit logMessage(message);
    m_busy = false;
    m_waitingForServerCheck = false;
    m_stopRequested = false;
    emit statusChanged(QStringLiteral("Ready"));
    emit busyChanged(false);
    emit batchFinished();
}
