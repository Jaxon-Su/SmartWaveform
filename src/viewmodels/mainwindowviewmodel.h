#pragma once

#include "exporters/excelresultwriter.h"
#include "models/ocrresult.h"
#include "services/imagescanner.h"
#include "services/ollamaclient.h"
#include "services/waveformocrparser.h"

#include <QElapsedTimer>
#include <QObject>
#include <QVector>

class MainWindowViewModel : public QObject
{
    Q_OBJECT

public:
    explicit MainWindowViewModel(QObject *parent = nullptr);

public slots:
    void startBatch(const QString &inputRoot,
                    const QString &outputPath,
                    const QString &model,
                    const QString &generateUrl,
                    const QString &prompt);
    void stopBatch();

signals:
    void resultsCleared();
    void resultReady(const OcrResult &result);
    void logMessage(const QString &message);
    void progressChanged(int current, int total);
    void statusChanged(const QString &status);
    void busyChanged(bool busy);
    void batchFinished();

private slots:
    void handleServerCheckFinished(bool ok, const QString &message);
    void handleRecognitionFinished(const QString &imagePath,
                                   const QByteArray &responseBody,
                                   const QString &errorMessage);

private:
    void processNextImage();
    void finishBatch();
    void stopWithError(const QString &message);

    // ViewModel owns batch state; the view only receives progress/result updates.
    QVector<QString> m_images;
    QVector<OcrResult> m_results;
    int m_currentIndex = 0;
    bool m_stopRequested = false;
    bool m_busy = false;
    bool m_waitingForServerCheck = false;

    QString m_outputPath;
    QString m_inputRoot;
    QString m_model;
    QString m_generateUrl;
    QString m_prompt;
    QElapsedTimer m_timer;

    ImageScanner m_scanner;
    WaveformOcrParser m_parser;
    ExcelResultWriter m_writer;
    OllamaClient m_ollama;
};
