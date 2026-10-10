#pragma once

#include <QAbstractTableModel>
#include <QStringList>

#include "core/Models.h"

class ItemTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    using QAbstractTableModel::QAbstractTableModel;

    void setItems(const QList<Item> &items);
    const Item &itemAt(int row) const { return m_items.at(row); }
    int rowOfItem(int itemId) const; // -1 if absent.

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    Qt::DropActions supportedDragActions() const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QList<Item> m_items;
};
