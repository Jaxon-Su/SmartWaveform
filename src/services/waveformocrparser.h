#pragma once

#include "models/ocrresult.h"

#include <QMap>
#include <QVariantMap>

class WaveformOcrParser
{
public:
    // 將 Ollama /api/generate 回傳的 JSON body 轉成 OcrResult。
    // 解析規則集中在這裡，未來模型輸出格式改變時，不需要動 UI 或 ViewModel。
    OcrResult parse(const QString &imagePath,
                    const QString &rootPath,
                    const QByteArray &responseBody,
                    const QString &networkError) const;

private:
    QVariantMap extractJsonObject(const QString &text) const;
    QMap<QString, QString> extractFieldsFromOcr(const QString &text) const;
    QString normalizeValue(const QVariantMap &json,
                           const QMap<QString, QString> &fields,
                           const QString &key) const;
    QString peakValue(const QString &maxValue, const QString &minValue) const;
    double valueInVolts(const QString &value, bool *ok) const;
};
