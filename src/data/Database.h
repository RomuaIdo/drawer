#pragma once

#include <QSqlDatabase>
#include <QString>

// Owns the SQLite connection and keeps the schema up to date.
class Database
{
public:
    // Use ":memory:" for a throwaway database.
    explicit Database(const QString &path);
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    QSqlDatabase connection() const;

private:
    void migrate();

    QString m_connectionName;
};
