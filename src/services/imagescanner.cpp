#include "imagescanner.h"

#include <QCollator>
#include <QDir>
#include <QDirIterator>
#include <QSet>
#include <algorithm>

namespace {
QStringList supportedImageFilters()
{
    return {
        QStringLiteral("*.png"), QStringLiteral("*.PNG"),
        QStringLiteral("*.jpg"), QStringLiteral("*.JPG"),
        QStringLiteral("*.jpeg"), QStringLiteral("*.JPEG"),
        QStringLiteral("*.bmp"), QStringLiteral("*.BMP"),
        QStringLiteral("*.webp"), QStringLiteral("*.WEBP")
    };
}
}

QVector<QString> ImageScanner::scanPngFiles(const QString &rootPath) const
{
    const QDir root(rootPath);
    if (!root.exists())
        return {};

    QSet<QString> unique;
    QDirIterator it(root.absolutePath(),
                    supportedImageFilters(),
                    QDir::Files | QDir::Readable,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        unique.insert(QDir::cleanPath(it.fileInfo().absoluteFilePath()));
    }

    QVector<QString> images(unique.cbegin(), unique.cend());
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    std::sort(images.begin(), images.end(), [&collator](const QString &a, const QString &b) {
        return collator.compare(QDir::toNativeSeparators(a), QDir::toNativeSeparators(b)) < 0;
    });

    return images;
}
