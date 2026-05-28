#pragma once

#include <QString>
#include <QVector>

class ImageScanner
{
public:
    // Recursively scans the selected root for supported oscilloscope image files.
    // Folder names are intentionally not interpreted here, so any hierarchy works.
    QVector<QString> scanPngFiles(const QString &rootPath) const;
};
