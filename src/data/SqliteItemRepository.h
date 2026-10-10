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

private:
    QSqlDatabase m_db;
};
