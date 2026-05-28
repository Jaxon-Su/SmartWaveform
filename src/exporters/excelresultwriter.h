#pragma once

#include "iresultwriter.h"

class ExcelResultWriter : public IResultWriter
{
public:
    bool write(const QVector<OcrResult> &rows,
               const QString &path,
               QString *errorMessage = nullptr) const override;

private:
    QString excelPathFor(const QString &path) const;
    QString excelSheetName(const QString &folder) const;
    QString compactRawResponse(const QString &value) const;
    QString peakValueFor(const QString &value) const;
    bool writeXlsx(const QVector<OcrResult> &rows,
                   const QString &path,
                   QString *errorMessage = nullptr) const;
};
