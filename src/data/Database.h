#pragma once

#include <QSqlDatabase>
#include <QString>
#include <memory>

class UndoLog;

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
    // History of data changes made through this database.
    UndoLog &undoLog() const { return *m_undoLog; }

private:
    void migrate();

    QString m_connectionName;
    std::unique_ptr<UndoLog> m_undoLog;
};
