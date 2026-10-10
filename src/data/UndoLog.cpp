#include "UndoLog.h"

#include "SqlUtil.h"

namespace {

const QStringList kTables = {"cabinets", "drawer_groups", "drawers", "items"};

QStringList columnsOf(const QSqlDatabase &db, const QString &table)
{
    QStringList columns;
    QSqlQuery q = SqlUtil::exec(db, QString("PRAGMA table_info(%1)").arg(table));
    while (q.next())
        columns.append(q.value(1).toString());
    return columns;
}

// SQL expression that renders `prefix.column` as a literal, e.g. quote(old.name).
QString literal(const QString &prefix, const QString &column)
{
    return QString("quote(%1.%2)").arg(prefix, column);
}

QString join(const QStringList &parts, const QString &separator)
{
    return parts.join(separator);
}

} // namespace

UndoLog::UndoLog(QSqlDatabase db)
    : m_db(db)
{
    install();
}

void UndoLog::install()
{
    SqlUtil::exec(m_db, "CREATE TEMP TABLE undolog (seq INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "sql TEXT NOT NULL)");

    for (const QString &table : kTables) {
        const QStringList columns = columnsOf(m_db, table);

        // Insert: remove the new row.
        SqlUtil::exec(m_db, QString(
            "CREATE TEMP TRIGGER undo_%1_insert AFTER INSERT ON %1 BEGIN "
            "INSERT INTO undolog (sql) VALUES ('DELETE FROM %1 WHERE id=' || new.id); END")
                .arg(table));

        // Delete: put the old row back.
        QStringList oldValues;
        for (const QString &c : columns)
            oldValues << literal("old", c);
        SqlUtil::exec(m_db, QString(
            "CREATE TEMP TRIGGER undo_%1_delete BEFORE DELETE ON %1 BEGIN "
            "INSERT INTO undolog (sql) VALUES ('INSERT INTO %1 (%2) VALUES (' || %3 || ')'); END")
                .arg(table, join(columns, ","), join(oldValues, " || ',' || ")));

        // Update: restore the old values.
        QStringList assignments;
        for (const QString &c : columns)
            assignments << QString("'%1=' || %2").arg(c, literal("old", c));
        SqlUtil::exec(m_db, QString(
            "CREATE TEMP TRIGGER undo_%1_update AFTER UPDATE ON %1 BEGIN "
            "INSERT INTO undolog (sql) VALUES ('UPDATE %1 SET ' || %2 || ' WHERE id=' || old.id); "
            "END")
                .arg(table, join(assignments, " || ',' || ")));
    }
}

QStringList UndoLog::drain()
{
    QStringList statements;
    QSqlQuery q = SqlUtil::exec(m_db, "SELECT sql FROM undolog ORDER BY seq");
    while (q.next())
        statements.append(q.value(0).toString());
    SqlUtil::exec(m_db, "DELETE FROM undolog");
    return statements;
}

void UndoLog::commit(const QString &label)
{
    const QStringList statements = drain();
    if (statements.isEmpty())
        return;
    m_undo.push_back({label, statements});
    if (m_undo.size() > kMaxSteps)
        m_undo.erase(m_undo.begin());
    m_redo.clear();
}

QStringList UndoLog::replay(const Step &step)
{
    SqlUtil::Transaction tx(m_db);
    // Children may be restored before their parents; check the keys once at the end.
    SqlUtil::exec(m_db, "PRAGMA defer_foreign_keys = ON");
    for (auto it = step.statements.crbegin(); it != step.statements.crend(); ++it)
        SqlUtil::exec(m_db, *it);
    tx.commit();
    return drain();
}

void UndoLog::clear()
{
    drain();
    m_undo.clear();
    m_redo.clear();
}

void UndoLog::undo()
{
    commit(QObject::tr("Change")); // Changes nobody closed yet are undone first.
    if (m_undo.empty())
        return;
    const Step step = m_undo.back();
    const QStringList reverse = replay(step);
    m_undo.pop_back();
    m_redo.push_back({step.label, reverse});
}

void UndoLog::redo()
{
    if (m_redo.empty())
        return;
    const Step step = m_redo.back();
    const QStringList reverse = replay(step);
    m_redo.pop_back();
    m_undo.push_back({step.label, reverse});
}
