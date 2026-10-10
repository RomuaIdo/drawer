#pragma once

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariantList>

#include "DatabaseError.h"

namespace SqlUtil {

// Runs a statement with positional bindings, throwing DatabaseError on failure.
inline QSqlQuery exec(const QSqlDatabase &db, const QString &sql, const QVariantList &args = {})
{
    QSqlQuery query(db);
    if (!query.prepare(sql))
        throw DatabaseError(query.lastError().text());
    for (const QVariant &arg : args) {
        // Text columns are NOT NULL; an unset QString must be stored as "", not NULL.
        const bool nullString = arg.metaType().id() == QMetaType::QString && arg.toString().isNull();
        query.addBindValue(nullString ? QVariant(QStringLiteral("")) : arg);
    }
    if (!query.exec())
        throw DatabaseError(query.lastError().text() + " [" + sql + "]");
    return query;
}

// Rolls back on destruction unless commit() was called.
class Transaction
{
public:
    explicit Transaction(QSqlDatabase db) : m_db(db)
    {
        if (!m_db.transaction())
            throw DatabaseError(m_db.lastError().text());
    }
    ~Transaction()
    {
        if (!m_committed)
            m_db.rollback();
    }
    Transaction(const Transaction &) = delete;
    Transaction &operator=(const Transaction &) = delete;

    void commit()
    {
        if (!m_db.commit())
            throw DatabaseError(m_db.lastError().text());
        m_committed = true;
    }

private:
    QSqlDatabase m_db;
    bool m_committed = false;
};

} // namespace SqlUtil
