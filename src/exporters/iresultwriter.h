#pragma once

#include "models/ocrresult.h"

#include <QString>
#include <QVector>

class IResultWriter
{
public:
    virtual ~IResultWriter() = default;

    // 輸出策略介面。
    // 未來 Excel、JSON、客製格式都可以新增 class 實作這個介面，ViewModel 不需要改流程。
    virtual bool write(const QVector<OcrResult> &rows,
                       const QString &path,
                       QString *errorMessage = nullptr) const = 0;
};
