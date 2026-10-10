#pragma once

#include <QList>

class QMimeData;

// Drag and drop payload: the ids of the items being dragged.
namespace ItemMime {

extern const char *const kType;

QMimeData *encode(const QList<int> &itemIds); // Caller takes ownership.
bool accepts(const QMimeData *data);
QList<int> decode(const QMimeData *data);

} // namespace ItemMime
