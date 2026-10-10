#pragma once

#include <QSqlDatabase>

#include "Repositories.h"

class SqliteListRepository : public IListRepository
{
public:
    explicit SqliteListRepository(QSqlDatabase db) : m_db(db) {}

    QList<ListLine> lines(ListKind kind) const override;
    void add(ListKind kind, const QList<int> &itemIds, int quantity = 1) override;
    void setQuantity(int lineId, int quantity) override;
    void remove(const QList<int> &lineIds) override;
    void clear(ListKind kind) override;
    ListResult apply(ListKind kind) override;

private:
    QSqlDatabase m_db;
};
