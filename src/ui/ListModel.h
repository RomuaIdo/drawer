#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "core/Models.h"

// Lines of a pick or restock list, with the quantity to take or add editable in the table.
class ListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ListModel(ListKind kind, QObject *parent = nullptr);

    void setLines(const QList<ListLine> &lines);
    const ListLine &lineAt(int row) const { return m_lines.at(row); }

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    enum Column { Name, Category, Value, Location, InStock, Quantity, After, ColumnCount };

signals:
    void quantityEdited(int lineId, int quantity);

private:
    ListKind m_kind;
    QList<ListLine> m_lines;
};
