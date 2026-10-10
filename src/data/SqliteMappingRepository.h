#pragma once

#include <QSqlDatabase>

#include "Repositories.h"

class SqliteMappingRepository : public IMappingRepository
{
public:
    explicit SqliteMappingRepository(QSqlDatabase db) : m_db(db) {}

    QList<MappingRow> rows() const override;

private:
    QSqlDatabase m_db;
};
