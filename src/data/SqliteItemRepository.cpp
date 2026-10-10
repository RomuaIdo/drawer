#include "SqliteItemRepository.h"

#include "SqlUtil.h"

namespace {

const char *kColumns = "id, drawer_id, name, category, value, package, quantity, notes";

Item readItem(const QSqlQuery &q)
{
    return {q.value(0).toInt(), q.value(1).toInt(), q.value(2).toString(),
            q.value(3).toString(), q.value(4).toString(), q.value(5).toString(),
            q.value(6).toInt(), q.value(7).toString()};
}

QList<Item> readAll(QSqlQuery q)
{
    QList<Item> result;
    while (q.next())
        result.append(readItem(q));
    return result;
}

QString escapeLike(QString text)
{
    text.replace('\\', "\\\\").replace('%', "\\%").replace('_', "\\_");
    return text;
}

} // namespace

QList<Item> SqliteItemRepository::inDrawer(int drawerId) const
{
    return readAll(SqlUtil::exec(m_db,
        QString("SELECT %1 FROM items WHERE drawer_id = ? ORDER BY name").arg(kColumns),
        {drawerId}));
}

QList<Item> SqliteItemRepository::search(const QString &text) const
{
    const QString pattern = "%" + escapeLike(text.trimmed()) + "%";
    return readAll(SqlUtil::exec(m_db,
        QString("SELECT %1 FROM items WHERE "
                "name LIKE ? ESCAPE '\\' OR category LIKE ? ESCAPE '\\' OR "
                "value LIKE ? ESCAPE '\\' OR package LIKE ? ESCAPE '\\' OR "
                "notes LIKE ? ESCAPE '\\' ORDER BY name").arg(kColumns),
        {pattern, pattern, pattern, pattern, pattern}));
}

Item SqliteItemRepository::add(Item item)
{
    QSqlQuery q = SqlUtil::exec(m_db,
        "INSERT INTO items (drawer_id, name, category, value, package, quantity, notes) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)",
        {item.drawerId, item.name, item.category, item.value, item.package, item.quantity,
         item.notes});
    item.id = q.lastInsertId().toInt();
    return item;
}

void SqliteItemRepository::update(const Item &item)
{
    SqlUtil::exec(m_db,
        "UPDATE items SET drawer_id = ?, name = ?, category = ?, value = ?, package = ?, "
        "quantity = ?, notes = ? WHERE id = ?",
        {item.drawerId, item.name, item.category, item.value, item.package, item.quantity,
         item.notes, item.id});
}

void SqliteItemRepository::remove(int id)
{
    SqlUtil::exec(m_db, "DELETE FROM items WHERE id = ?", {id});
}
