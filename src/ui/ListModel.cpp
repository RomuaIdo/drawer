#include "ListModel.h"

#include <QColor>

ListModel::ListModel(ListKind kind, QObject *parent)
    : QAbstractTableModel(parent)
    , m_kind(kind)
{
}

void ListModel::setLines(const QList<ListLine> &lines)
{
    beginResetModel();
    m_lines = lines;
    endResetModel();
}

int ListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_lines.size();
}

int ListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

Qt::ItemFlags ListModel::flags(const QModelIndex &index) const
{
    const Qt::ItemFlags base = QAbstractTableModel::flags(index);
    return index.column() == Quantity ? base | Qt::ItemIsEditable : base;
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const ListLine &line = m_lines.at(index.row());
    const Item &item = line.item;

    const bool numeric = index.column() == InStock || index.column() == Quantity
                         || index.column() == After;
    if (role == Qt::TextAlignmentRole && numeric)
        return int(Qt::AlignRight | Qt::AlignVCenter);
    // Asking for more than there is stands out.
    if (role == Qt::BackgroundRole && line.isShort())
        return QColor(0xd3, 0x20, 0x20, 60);
    if (role == Qt::ToolTipRole && line.isShort())
        return tr("Only %1 in stock").arg(item.quantity);
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return {};

    switch (index.column()) {
    case Name: return item.name;
    case Category: return item.category;
    case Value: return item.value;
    case Location: return QString("%1/%2").arg(line.group, line.drawerLabel);
    case InStock: return item.quantity;
    case Quantity: return line.quantity;
    case After:
        return m_kind == ListKind::Pick ? qMax(0, item.quantity - line.quantity)
                                        : item.quantity + line.quantity;
    }
    return {};
}

bool ListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || index.column() != Quantity)
        return false;
    bool ok = false;
    const int quantity = value.toInt(&ok);
    if (!ok || quantity < 1)
        return false;
    emit quantityEdited(m_lines.at(index.row()).id, quantity);
    return true;
}

QVariant ListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case Name: return tr("Name");
    case Category: return tr("Category");
    case Value: return tr("Value");
    case Location: return tr("Location / Drawer");
    case InStock: return tr("In stock");
    case Quantity: return m_kind == ListKind::Pick ? tr("To take") : tr("To add");
    case After: return tr("After");
    }
    return {};
}
