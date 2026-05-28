#include "excelresultwriter.h"

#include "xlsxdocument.h"
#include "xlsxformat.h"

#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>

bool ExcelResultWriter::write(const QVector<OcrResult> &rows,
                              const QString &path,
                              QString *errorMessage) const
{
    if (path.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Output path is empty.");
        return false;
    }

    const QString excelPath = excelPathFor(path);
    return writeXlsx(rows, excelPath, errorMessage);
}

QString ExcelResultWriter::excelPathFor(const QString &path) const
{
    QFileInfo info(path);
    return info.dir().filePath(info.completeBaseName() + QStringLiteral(".xlsx"));
}

QString ExcelResultWriter::excelSheetName(const QString &folder) const
{
    const QStringList parts = QDir::fromNativeSeparators(folder).split('/', Qt::SkipEmptyParts);
    QString name;

    if (parts.size() >= 2) {
        const QString kind = parts.at(1).compare(QStringLiteral("voltage"), Qt::CaseInsensitive) == 0
            ? QStringLiteral("voltage")
            : parts.at(1).toLower();
        name = QStringLiteral("%1_%2_sheet").arg(parts.at(0), kind);
    } else {
        name = folder.trimmed().isEmpty() ? QStringLiteral("unknown_data_sheet") : folder + QStringLiteral("_sheet");
    }

    name.replace(QRegularExpression(QStringLiteral(R"([\\/\?\*\[\]:])")), QStringLiteral("_"));
    return name.left(31);
}

QString ExcelResultWriter::compactRawResponse(const QString &value) const
{
    return value.simplified();
}

QString ExcelResultWriter::peakValueFor(const QString &value) const
{
    const QString simplified = value.simplified();
    static const QRegularExpression peakPattern(
        QStringLiteral(R"(^([+-]?)(\d+(?:\.\d+)?)\s*([munpkKMGT]?)([VAW])\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = peakPattern.match(simplified);
    if (!match.hasMatch())
        return simplified;

    const QString sign = match.captured(1);
    const QString number = match.captured(2);
    const QString prefix = match.captured(3);
    if (!number.contains(QLatin1Char('.')) && prefix.compare(QStringLiteral("m"), Qt::CaseInsensitive) == 0)
        return sign + number.rightJustified(4, QLatin1Char('0'));

    return sign + number;
}

bool ExcelResultWriter::writeXlsx(const QVector<OcrResult> &rows,
                                  const QString &path,
                                  QString *errorMessage) const
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    QMap<QString, QVector<OcrResult>> rowsByFolder;
    for (const OcrResult &row : rows)
        rowsByFolder[row.folder].append(row);

    QXlsx::Document workbook;

    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setFontColor(Qt::white);
    headerFormat.setFillPattern(QXlsx::Format::PatternSolid);
    headerFormat.setPatternForegroundColor(QColor(QStringLiteral("#1F4E79")));
    headerFormat.setVerticalAlignment(QXlsx::Format::AlignTop);

    QXlsx::Format bodyFormat;
    bodyFormat.setTextWrap(false);
    bodyFormat.setVerticalAlignment(QXlsx::Format::AlignTop);

    for (auto it = rowsByFolder.cbegin(); it != rowsByFolder.cend(); ++it) {
        const QString sheetName = excelSheetName(it.key());
        workbook.addSheet(sheetName);
        workbook.selectSheet(sheetName);

        workbook.setColumnWidth(1, 12);
        workbook.setColumnWidth(2, 8, 11);
        workbook.setColumnWidth(9, 60);

        const QStringList headers = {
            QStringLiteral("filename"),
            QStringLiteral("max"),
            QStringLiteral("min"),
            QStringLiteral("rms"),
            QStringLiteral("mean"),
            QStringLiteral("peak"),
            QStringLiteral("peak value"),
            QStringLiteral("status"),
            QStringLiteral("raw_response")
        };

        for (int col = 0; col < headers.size(); ++col)
            workbook.write(1, col + 1, headers.at(col), headerFormat);
        workbook.setRowHeight(1, 16.5);

        int excelRow = 2;
        for (const OcrResult &row : it.value()) {
            const QStringList values = {
                row.filename,
                row.max,
                row.min,
                row.rms,
                row.mean,
                row.peak,
                peakValueFor(row.peak),
                row.status,
                compactRawResponse(row.rawResponse)
            };

            for (int col = 0; col < values.size(); ++col)
                workbook.write(excelRow, col + 1, values.at(col), bodyFormat);
            workbook.setRowHeight(excelRow, 16.5);
            ++excelRow;
        }
    }

    workbook.deleteSheet(QStringLiteral("Sheet1"));

    if (!workbook.saveAs(path)) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to save Excel file: %1").arg(path);
        return false;
    }

    return true;
}
