#include "SqliteItemRepository.h"

#include "SqlUtil.h"

namespace {

const char *kColumns = "id, drawer_id, name, category, value, package, quantity, notes, min_stock";

Item readItem(const QSqlQuery &q)
{
    Item item{q.value(0).toInt(), q.value(1).toInt(), q.value(2).toString(),
              q.value(3).toString(), q.value(4).toString(), q.value(5).toString(),
              q.value(6).toInt(), q.value(7).toString(), std::nullopt};
    if (!q.value(8).isNull())
        item.minStock = q.value(8).toInt();
    return item;
}

QVariant minStockValue(const Item &item)
{
    return item.minStock ? QVariant(*item.minStock) : QVariant(QMetaType::fromType<int>());
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
        "INSERT INTO items (drawer_id, name, category, value, package, quantity, notes, "
        "min_stock) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
        {item.drawerId, item.name, item.category, item.value, item.package, item.quantity,
         item.notes, minStockValue(item)});
    item.id = q.lastInsertId().toInt();
    return item;
}

void SqliteItemRepository::update(const Item &item)
{
    SqlUtil::exec(m_db,
        "UPDATE items SET drawer_id = ?, name = ?, category = ?, value = ?, package = ?, "
        "quantity = ?, notes = ?, min_stock = ? WHERE id = ?",
        {item.drawerId, item.name, item.category, item.value, item.package, item.quantity,
         item.notes, minStockValue(item), item.id});
}

void SqliteItemRepository::remove(int id)
{
    SqlUtil::exec(m_db, "DELETE FROM items WHERE id = ?", {id});
}

QList<Item> SqliteItemRepository::lowStock() const
{
    return readAll(SqlUtil::exec(m_db,
        QString("SELECT %1 FROM items WHERE min_stock IS NOT NULL AND quantity <= min_stock "
                "ORDER BY name").arg(kColumns)));
}

void SqliteItemRepository::adjustQuantity(const QList<int> &ids, int delta)
{
    SqlUtil::Transaction tx(m_db);
    for (int id : ids)
        SqlUtil::exec(m_db, "UPDATE items SET quantity = MAX(0, quantity + ?) WHERE id = ?",
                      {delta, id});
    tx.commit();
}
