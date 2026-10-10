#include "AllItemsModel.h"

#include <QColor>
#include <QHash>
#include <QSet>

void AllItemsModel::setEntries(const QList<ItemEntry> &entries)
{
    beginResetModel();
    m_entries = entries;
    rebuild();
    endResetModel();
}

void AllItemsModel::setGrouped(bool grouped)
{
    if (grouped == m_grouped)
        return;
    beginResetModel();
    m_grouped = grouped;
    rebuild();
    endResetModel();
}

void AllItemsModel::rebuild()
{
    m_rows.clear();
    if (!m_grouped) {
        for (const ItemEntry &e : m_entries)
            m_rows.append({e.item, e.item.quantity,
                           {{e.item.drawerId, e.group, e.drawerLabel, e.item.quantity}},
                           e.item.notes});
        return;
    }

    QHash<QString, int> rowOfItem; // identity -> index in m_rows
    for (const ItemEntry &e : m_entries) {
        auto found = rowOfItem.constFind(e.identity);
        if (found == rowOfItem.constEnd()) {
            found = rowOfItem.insert(e.identity, m_rows.size());
            m_rows.append({e.item, 0, {}, QString()});
        }
        Row &row = m_rows[*found];
        row.quantity += e.item.quantity;

        // Several entries in one drawer count as a single place.
        bool merged = false;
        for (Place &place : row.places) {
            if (place.drawerId == e.item.drawerId) {
                place.quantity += e.item.quantity;
                merged = true;
            }
        }
        if (!merged)
            row.places.append({e.item.drawerId, e.group, e.drawerLabel, e.item.quantity});

        const QString note = e.item.notes.trimmed();
        if (!note.isEmpty() && !row.notes.split("; ").contains(note))
            row.notes += (row.notes.isEmpty() ? "" : "; ") + note;
    }
}

// "R1C1 (10), R2C3 (4)", with the group name in front when the drawers span several groups.
QString AllItemsModel::drawersText(const QList<Place> &places)
{
    QSet<QString> groups;
    for (const Place &p : places)
        groups.insert(p.group);

    QStringList parts;
    for (const Place &p : places) {
        const QString name = groups.size() > 1 ? p.group + "/" + p.drawerLabel : p.drawerLabel;
        parts << QString("%1 (%2)").arg(name).arg(p.quantity);
    }
    return parts.join(", ");
}

QList<int> AllItemsModel::drawerIdsAt(int row) const
{
    QList<int> ids;
    if (row < 0 || row >= m_rows.size())
        return ids;
    for (const Place &p : m_rows[row].places)
        ids.append(p.drawerId);
    return ids;
}

int AllItemsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

int AllItemsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant AllItemsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const Row &row = m_rows.at(index.row());
    const Item &item = row.item;

    if (role == Qt::TextAlignmentRole && (index.column() == Quantity || index.column() == MinStock))
        return int(Qt::AlignRight | Qt::AlignVCenter);
    if (role == Qt::BackgroundRole && item.isLowStock())
        return QColor(0xd3, 0x20, 0x20, 60);
    if (role == Qt::ToolTipRole && index.column() == Quantity && item.stockTotal() != row.quantity)
        return QString("%1/%2").arg(row.quantity).arg(item.stockTotal());
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case Name: return item.name;
    case Category: return item.category;
    case Value: return item.value;
    case Package: return item.package;
    case Quantity: return row.quantity;
    case MinStock: return item.minStock ? QVariant(*item.minStock) : QVariant();
    case Location:
        if (m_grouped)
            return drawersText(row.places);
        return QString("%1/%2").arg(row.places.first().group, row.places.first().drawerLabel);
    case Notes: return row.notes;
    }
    return {};
}

QVariant AllItemsModel::headerData(int section, Qt::Orientation orientation, int role) const
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
    case Location: return m_grouped ? tr("Drawers") : tr("Location / Drawer");
    case Notes: return tr("Notes");
    }
    return {};
}
