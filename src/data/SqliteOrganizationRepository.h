#pragma once

#include <QSqlDatabase>

#include "Repositories.h"

class SqliteOrganizationRepository : public IOrganizationRepository
{
public:
    explicit SqliteOrganizationRepository(QSqlDatabase db) : m_db(db) {}

    Organization load() const override;
    void import(const Organization &organization) override;

private:
    QSqlDatabase m_db;
};
