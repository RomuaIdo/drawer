#include "Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>

#include "DatabaseError.h"
#include "UndoLog.h"
#include "SqlUtil.h"

namespace {

// Index + 1 == schema version. Append only; never edit released entries.
const QStringList kMigrations = {
    R"(
    CREATE TABLE cabinets (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        description TEXT NOT NULL DEFAULT ''
    );
    CREATE TABLE drawer_groups (
        id INTEGER PRIMARY KEY,
        cabinet_id INTEGER NOT NULL REFERENCES cabinets(id) ON DELETE CASCADE,
        name TEXT NOT NULL,
        rows INTEGER NOT NULL CHECK (rows > 0),
        cols INTEGER NOT NULL CHECK (cols > 0),
        pos_x INTEGER NOT NULL DEFAULT 0,
        pos_y INTEGER NOT NULL DEFAULT 0,
        default_size TEXT NOT NULL
    );
    CREATE TABLE drawers (
        id INTEGER PRIMARY KEY,
        group_id INTEGER NOT NULL REFERENCES drawer_groups(id) ON DELETE CASCADE,
        row INTEGER NOT NULL,
        col INTEGER NOT NULL,
        size TEXT NOT NULL,
        label TEXT NOT NULL DEFAULT '',
        UNIQUE (group_id, row, col)
    );
    CREATE TABLE items (
        id INTEGER PRIMARY KEY,
        drawer_id INTEGER NOT NULL REFERENCES drawers(id) ON DELETE CASCADE,
        name TEXT NOT NULL,
        category TEXT NOT NULL DEFAULT '',
        value TEXT NOT NULL DEFAULT '',
        package TEXT NOT NULL DEFAULT '',
        quantity INTEGER NOT NULL DEFAULT 0,
        notes TEXT NOT NULL DEFAULT ''
    );
    CREATE INDEX idx_items_drawer ON items(drawer_id);
    )",
    "ALTER TABLE drawer_groups ADD COLUMN color TEXT NOT NULL DEFAULT ''",
    "ALTER TABLE items ADD COLUMN min_stock INTEGER", // NULL: no alert.
    // Entries of one item share name, category, value and package (ignoring case and
    // surrounding spaces); stock is counted per cabinet.
    R"(
    CREATE VIEW item_stock AS
    SELECT i.id AS item_id,
           g.cabinet_id AS cabinet_id,
           lower(trim(i.name)) || char(31) || lower(trim(i.category)) || char(31) ||
               lower(trim(i.value)) || char(31) || lower(trim(i.package)) AS identity,
           SUM(i.quantity) OVER w AS total,
           MAX(i.min_stock) OVER w AS min_stock
    FROM items i
    JOIN drawers d ON d.id = i.drawer_id
    JOIN drawer_groups g ON g.id = d.group_id
    WINDOW w AS (PARTITION BY g.cabinet_id, lower(trim(i.name)), lower(trim(i.category)),
                 lower(trim(i.value)), lower(trim(i.package)))
    )",
};

} // namespace

Database::Database(const QString &path)
    : m_connectionName(QUuid::createUuid().toString())
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(path);
    if (!db.open())
        throw DatabaseError(db.lastError().text());

    SqlUtil::exec(db, "PRAGMA foreign_keys = ON");
    migrate();
    m_undoLog = std::make_unique<UndoLog>(db); // After migrating: schema changes are not undoable.
}

Database::~Database()
{
    m_undoLog.reset();
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

QSqlDatabase Database::connection() const
{
    return QSqlDatabase::database(m_connectionName);
}

void Database::migrate()
{
    QSqlDatabase db = connection();
    QSqlQuery version = SqlUtil::exec(db, "PRAGMA user_version");
    version.next();
    const int current = version.value(0).toInt();

    for (int v = current; v < kMigrations.size(); ++v) {
        SqlUtil::Transaction tx(db);
        // QSqlQuery runs one statement at a time.
        for (const QString &statement : kMigrations[v].split(';', Qt::SkipEmptyParts)) {
            if (!statement.trimmed().isEmpty())
                SqlUtil::exec(db, statement);
        }
        SqlUtil::exec(db, QString("PRAGMA user_version = %1").arg(v + 1));
        tx.commit();
    }
}
