#pragma once

#include <QSqlDatabase>

#include "Repositories.h"

class SqliteItemRepository : public IItemRepository
{
public:
    explicit SqliteItemRepository(QSqlDatabase db) : m_db(db) {}

    QList<Item> inDrawer(int drawerId) const override;
    QList<Item> search(const QString &text) const override;
    Item add(Item item) override;
    void update(const Item &item) override;
    void remove(int id) override;
    QList<Item> lowStock() const override;
    QList<ItemLocation> locationsOf(int itemId) const override;
    QList<ItemEntry> inCabinet(int cabinetId) const override;
    void adjustQuantity(const QList<int> &ids, int delta) override;
    void moveItems(const QList<int> &ids, int drawerId) override;
    void copyItems(const QList<int> &ids, int drawerId) override;

private:
    QSqlDatabase m_db;
};
