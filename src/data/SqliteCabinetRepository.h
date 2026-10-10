#pragma once

#include <QSqlDatabase>

#include "Repositories.h"

class SqliteCabinetRepository : public ICabinetRepository
{
public:
    explicit SqliteCabinetRepository(QSqlDatabase db) : m_db(db) {}

    QList<Cabinet> all() const override;
    Cabinet add(Cabinet cabinet) override;
    void update(const Cabinet &cabinet) override;
    void remove(int id) override;

private:
    QSqlDatabase m_db;
};
