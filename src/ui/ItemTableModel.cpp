#include "ItemTableModel.h"

namespace {
enum Column { Name, Category, Value, Package, Quantity, Notes, ColumnCount };
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
    if (role == Qt::TextAlignmentRole && index.column() == Quantity)
        return int(Qt::AlignRight | Qt::AlignVCenter);
    if (role != Qt::DisplayRole)
        return {};
    switch (index.column()) {
    case Name: return item.name;
    case Category: return item.category;
    case Value: return item.value;
    case Package: return item.package;
    case Quantity: return item.quantity;
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
    case Notes: return tr("Notes");
    }
    return {};
}
