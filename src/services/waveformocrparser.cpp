#include "waveformocrparser.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <cmath>
namespace {
QString jsonValueForAliases(const QVariantMap &json, const QStringList &aliases)
{
    for (const QString &alias : aliases) {
        for (auto it = json.cbegin(); it != json.cend(); ++it) {
            if (it.key().compare(alias, Qt::CaseInsensitive) != 0)
                continue;

            const QString value = it.value().toString().trimmed();
            if (!value.isEmpty() &&
                value.compare(QStringLiteral("null"), Qt::CaseInsensitive) != 0 &&
                value.compare(QStringLiteral("none"), Qt::CaseInsensitive) != 0) {
                return value;
            }
        }
    }

    return {};
}
}

OcrResult WaveformOcrParser::parse(const QString &imagePath,
                                   const QString &rootPath,
                                   const QByteArray &responseBody,
                                   const QString &networkError) const
{
    OcrResult result;
    const QFileInfo info(imagePath);
    QDir root(rootPath);
    QString relativeFolder = root.relativeFilePath(info.absolutePath());
    if (relativeFolder == QStringLiteral(".") || relativeFolder.startsWith(QStringLiteral("..")))
        relativeFolder = info.dir().dirName();

    result.folder = QDir::toNativeSeparators(QDir::cleanPath(relativeFolder));
    result.filename = info.fileName();

    if (!networkError.isEmpty()) {
        result.status = QStringLiteral("ERROR: %1").arg(networkError);
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(responseBody);
    const QString response = doc.object().value(QStringLiteral("response")).toString().trimmed();
    result.rawResponse = response;

    const QVariantMap json = extractJsonObject(response);
    const QMap<QString, QString> fields = extractFieldsFromOcr(response);

    result.max = normalizeValue(json, fields, QStringLiteral("max"));
    result.min = normalizeValue(json, fields, QStringLiteral("min"));
    result.rms = normalizeValue(json, fields, QStringLiteral("rms"));
    result.mean = normalizeValue(json, fields, QStringLiteral("mean"));
    result.peak = peakValue(result.max, result.min);

    result.status = (result.max.isEmpty() && result.min.isEmpty() && result.rms.isEmpty() && result.mean.isEmpty())
        ? QStringLiteral("NO_FIELDS_FOUND")
        : QStringLiteral("OK");

    return result;
}

QVariantMap WaveformOcrParser::extractJsonObject(const QString &text) const
{
    const QJsonDocument direct = QJsonDocument::fromJson(text.toUtf8());
    if (direct.isObject())
        return direct.object().toVariantMap();

    const QRegularExpression objectRe(QStringLiteral("\\{[\\s\\S]*\\}"));
    const QRegularExpressionMatch match = objectRe.match(text);
    if (!match.hasMatch())
        return {};

    const QJsonDocument embedded = QJsonDocument::fromJson(match.captured(0).toUtf8());
    return embedded.isObject() ? embedded.object().toVariantMap() : QVariantMap{};
}

QMap<QString, QString> WaveformOcrParser::extractFieldsFromOcr(const QString &text) const
{
    const QString number = QStringLiteral("([-+]?\\d+(?:\\.\\d+)?)");
    const QString unit = QStringLiteral("([A-Za-z%\\x{00B5}\\x{03BC}]+)?");
    const QMap<QString, QString> labels = {
        {QStringLiteral("max"), QStringLiteral("(?:Max|Maximum)")},
        {QStringLiteral("min"), QStringLiteral("(?:Min|Minimum)")},
        {QStringLiteral("rms"), QStringLiteral("RMS")},
        {QStringLiteral("mean"), QStringLiteral("Mean")},
    };

    QMap<QString, QString> fields;
    for (auto it = labels.cbegin(); it != labels.cend(); ++it) {
        const QString pattern = QStringLiteral("\\b(?:C\\d+\\s+)?%1\\b[^\\d+\\-]{0,80}%2\\s*%3")
            .arg(it.value(), number, unit);
        const QRegularExpression re(pattern,
            QRegularExpression::CaseInsensitiveOption |
            QRegularExpression::DotMatchesEverythingOption);

        const QRegularExpressionMatch match = re.match(text);
        if (match.hasMatch()) {
            const QString value = match.captured(1);
            const QString unitText = match.captured(2);
            fields.insert(it.key(), QStringLiteral("%1 %2").arg(value, unitText).trimmed());
        }
    }

    return fields;
}

QString WaveformOcrParser::normalizeValue(const QVariantMap &json,
                                          const QMap<QString, QString> &fields,
                                          const QString &key) const
{
    const QStringList aliases = key == QStringLiteral("max")
        ? QStringList{QStringLiteral("max"), QStringLiteral("maximum"), QStringLiteral("max value"), QStringLiteral("maximum value")}
        : key == QStringLiteral("min")
            ? QStringList{QStringLiteral("min"), QStringLiteral("minimum"), QStringLiteral("min value"), QStringLiteral("minimum value")}
            : key == QStringLiteral("rms")
                ? QStringList{QStringLiteral("rms"), QStringLiteral("rms value")}
                : key == QStringLiteral("mean")
                    ? QStringList{QStringLiteral("mean"), QStringLiteral("mean value")}
                    : QStringList{key};

    const QString jsonValue = jsonValueForAliases(json, aliases);
    if (!jsonValue.isEmpty())
        return jsonValue;

    return fields.value(key).trimmed();
}

QString WaveformOcrParser::peakValue(const QString &maxValue, const QString &minValue) const
{
    bool maxOk = false;
    bool minOk = false;
    const double maxVolts = valueInVolts(maxValue, &maxOk);
    const double minVolts = valueInVolts(minValue, &minOk);

    if (!maxOk)
        return minOk ? minValue : QString();
    if (!minOk)
        return maxValue;

    return std::abs(minVolts) > std::abs(maxVolts) ? minValue : maxValue;
}

double WaveformOcrParser::valueInVolts(const QString &value, bool *ok) const
{
    const QRegularExpression re(QStringLiteral("^\\s*([-+]?\\d+(?:\\.\\d+)?)\\s*([A-Za-z%\\x{00B5}\\x{03BC}]*)"));
    const QRegularExpressionMatch match = re.match(value);
    if (!match.hasMatch()) {
        if (ok) *ok = false;
        return 0.0;
    }

    bool numberOk = false;
    double volts = match.captured(1).toDouble(&numberOk);
    if (!numberOk) {
        if (ok) *ok = false;
        return 0.0;
    }

    const QString unit = match.captured(2).toLower();
    if (unit == QStringLiteral("mv"))
        volts /= 1000.0;
    else if (unit == QStringLiteral("uv") || unit == QStringLiteral("µv") || unit == QStringLiteral("μv"))
        volts /= 1000000.0;
    else if (unit == QStringLiteral("kv"))
        volts *= 1000.0;

    if (ok) *ok = true;
    return volts;
}
