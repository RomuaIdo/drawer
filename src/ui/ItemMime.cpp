#include "ItemMime.h"

#include <QMimeData>
#include <QStringList>

namespace ItemMime {

const char *const kType = "application/x-drawer-items";

QMimeData *encode(const QList<int> &itemIds)
{
    QStringList parts;
    for (int id : itemIds)
        parts.append(QString::number(id));
    auto *data = new QMimeData;
    data->setData(kType, parts.join(',').toUtf8());
    return data;
}

bool accepts(const QMimeData *data)
{
    return data && data->hasFormat(kType);
}

QList<int> decode(const QMimeData *data)
{
    QList<int> ids;
    if (!accepts(data))
        return ids;
    for (const QString &part : QString::fromUtf8(data->data(kType)).split(',', Qt::SkipEmptyParts)) {
        bool ok = false;
        const int id = part.toInt(&ok);
        if (ok)
            ids.append(id);
    }
    return ids;
}

} // namespace ItemMime
