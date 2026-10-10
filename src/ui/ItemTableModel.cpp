#include "ItemTableModel.h"

#include <QColor>
#include <QMimeData>

#include "ItemMime.h"

namespace {
enum Column { Name, Category, Value, Package, Quantity, MinStock, Notes, ColumnCount };
}

int ItemTableModel::rowOfItem(int itemId) const
{
    for (int row = 0; row < m_items.size(); ++row) {
        if (m_items[row].id == itemId)
            return row;
    }
    return -1;
}

Qt::ItemFlags ItemTableModel::flags(const QModelIndex &index) const
{
    const Qt::ItemFlags base = QAbstractTableModel::flags(index);
    return index.isValid() ? base | Qt::ItemIsDragEnabled : base;
}

QStringList ItemTableModel::mimeTypes() const
{
    return {ItemMime::kType};
}

QMimeData *ItemTableModel::mimeData(const QModelIndexList &indexes) const
{
    QList<int> ids;
    for (const QModelIndex &index : indexes) {
        const int id = m_items.at(index.row()).id;
        if (!ids.contains(id))
            ids.append(id);
    }
    return ItemMime::encode(ids);
}

Qt::DropActions ItemTableModel::supportedDragActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

void ItemTableModel::setItems(const QList<Item> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

int ItemTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

int ItemTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ItemTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const Item &item = m_items.at(index.row());
    if (role == Qt::TextAlignmentRole && (index.column() == Quantity || index.column() == MinStock))
        return int(Qt::AlignRight | Qt::AlignVCenter);
    // Items at or below their minimum stock stand out.
    if (role == Qt::BackgroundRole && item.isLowStock())
        return QColor(0xd3, 0x20, 0x20, 60);
    if (role == Qt::ToolTipRole && item.isLowStock())
        return tr("Low stock: %1 left, minimum %2").arg(item.quantity).arg(*item.minStock);
    if (role != Qt::DisplayRole)
        return {};
    switch (index.column()) {
    case Name: return item.name;
    case Category: return item.category;
    case Value: return item.value;
    case Package: return item.package;
    case Quantity: return item.quantity;
    case MinStock: return item.minStock ? QVariant(*item.minStock) : QVariant();
    case Notes: return item.notes;
    }
    return {};
}

QVariant ItemTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case Name: return tr("Name");
    case Category: return tr("Category");
    case Value: return tr("Value");
    case Package: return tr("Package");
    case Quantity: return tr("Qty");
    case MinStock: return tr("Min");
    case Notes: return tr("Notes");
    }
    return {};
}
