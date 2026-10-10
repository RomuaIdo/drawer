#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "core/Models.h"

// Every item of a cabinet, either one row per entry or one row per item.
class AllItemsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    using QAbstractTableModel::QAbstractTableModel;

    void setEntries(const QList<ItemEntry> &entries);
    // Grouped: entries of the same item share a row listing its drawers and quantities.
    void setGrouped(bool grouped);
    bool grouped() const { return m_grouped; }

    // Drawers holding the item in this row; the first is where the row's own entry lives.
    QList<int> drawerIdsAt(int row) const;

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    enum Column { Name, Category, Value, Package, Quantity, MinStock, Location, Notes, ColumnCount };

private:
    struct Place {
        int drawerId;
        QString group;
        QString drawerLabel;
        int quantity;
    };
    struct Row {
        Item item;          // The first entry; its stock total covers the whole item.
        int quantity = 0;   // Quantity shown: the entry's own, or the item's total when grouped.
        QList<Place> places;
        QString notes;
    };

    void rebuild();
    static QString drawersText(const QList<Place> &places);

    QList<ItemEntry> m_entries;
    QList<Row> m_rows;
    bool m_grouped = false;
};
