#include "SqliteItemRepository.h"

#include "SqlUtil.h"

namespace {

// Items joined with their cabinet-wide stock (see the item_stock view).
const char *kSelect =
    "SELECT i.id, i.drawer_id, i.name, i.category, i.value, i.package, i.quantity, i.notes, "
    "s.min_stock, s.total "
    "FROM items i JOIN item_stock s ON s.item_id = i.id ";

Item readItem(const QSqlQuery &q)
{
    Item item{q.value(0).toInt(), q.value(1).toInt(), q.value(2).toString(),
              q.value(3).toString(), q.value(4).toString(), q.value(5).toString(),
              q.value(6).toInt(), q.value(7).toString(), std::nullopt};
    if (!q.value(8).isNull())
        item.minStock = q.value(8).toInt();
    item.totalQuantity = q.value(9).toInt();
    return item;
}

QVariant minStockValue(const std::optional<int> &minStock)
{
    return minStock ? QVariant(*minStock) : QVariant(QMetaType::fromType<int>());
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

// The minimum stock belongs to the item as a whole: give it to every entry of the same
// item in the cabinet, so clearing or changing it in one drawer applies everywhere.
void shareMinStock(const QSqlDatabase &db, int itemId, const std::optional<int> &minStock)
{
    SqlUtil::exec(db,
        "UPDATE items SET min_stock = ? WHERE id IN ("
        "SELECT o.item_id FROM item_stock me JOIN item_stock o "
        "ON o.cabinet_id = me.cabinet_id AND o.identity = me.identity WHERE me.item_id = ?)",
        {minStockValue(minStock), itemId});
}

} // namespace

QList<Item> SqliteItemRepository::inDrawer(int drawerId) const
{
    return readAll(SqlUtil::exec(m_db,
        QString(kSelect) + "WHERE i.drawer_id = ? ORDER BY i.name", {drawerId}));
}

QList<Item> SqliteItemRepository::search(const QString &text) const
{
    const QString pattern = "%" + escapeLike(text.trimmed()) + "%";
    return readAll(SqlUtil::exec(m_db,
        QString(kSelect) +
            "WHERE i.name LIKE ? ESCAPE '\\' OR i.category LIKE ? ESCAPE '\\' OR "
            "i.value LIKE ? ESCAPE '\\' OR i.package LIKE ? ESCAPE '\\' OR "
            "i.notes LIKE ? ESCAPE '\\' ORDER BY i.name",
        {pattern, pattern, pattern, pattern, pattern}));
}

Item SqliteItemRepository::add(Item item)
{
    SqlUtil::Transaction tx(m_db);
    QSqlQuery q = SqlUtil::exec(m_db,
        "INSERT INTO items (drawer_id, name, category, value, package, quantity, notes, "
        "min_stock) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
        {item.drawerId, item.name, item.category, item.value, item.package, item.quantity,
         item.notes, minStockValue(item.minStock)});
    item.id = q.lastInsertId().toInt();
    if (item.minStock) {
        shareMinStock(m_db, item.id, item.minStock);
    } else {
        // A new entry of an item that already has a minimum takes it over.
        QSqlQuery inherited = SqlUtil::exec(m_db,
            "SELECT MAX(o.min_stock) FROM item_stock me JOIN item_stock o "
            "ON o.cabinet_id = me.cabinet_id AND o.identity = me.identity "
            "WHERE me.item_id = ? AND o.item_id <> ?",
            {item.id, item.id});
        if (inherited.next() && !inherited.value(0).isNull()) {
            item.minStock = inherited.value(0).toInt();
            SqlUtil::exec(m_db, "UPDATE items SET min_stock = ? WHERE id = ?",
                          {*item.minStock, item.id});
        }
    }
    tx.commit();
    return item;
}

void SqliteItemRepository::update(const Item &item)
{
    SqlUtil::Transaction tx(m_db);
    SqlUtil::exec(m_db,
        "UPDATE items SET drawer_id = ?, name = ?, category = ?, value = ?, package = ?, "
        "quantity = ?, notes = ?, min_stock = ? WHERE id = ?",
        {item.drawerId, item.name, item.category, item.value, item.package, item.quantity,
         item.notes, minStockValue(item.minStock), item.id});
    shareMinStock(m_db, item.id, item.minStock);
    tx.commit();
}

void SqliteItemRepository::remove(int id)
{
    SqlUtil::exec(m_db, "DELETE FROM items WHERE id = ?", {id});
}

QList<Item> SqliteItemRepository::lowStock() const
{
    return readAll(SqlUtil::exec(m_db,
        QString(kSelect) +
            "WHERE s.min_stock IS NOT NULL AND s.total <= s.min_stock ORDER BY i.name"));
}

QList<ItemLocation> SqliteItemRepository::locationsOf(int itemId) const
{
    QSqlQuery q = SqlUtil::exec(m_db,
        "SELECT i.id, d.id, g.name, d.label, i.quantity "
        "FROM item_stock me "
        "JOIN item_stock o ON o.cabinet_id = me.cabinet_id AND o.identity = me.identity "
        "JOIN items i ON i.id = o.item_id "
        "JOIN drawers d ON d.id = i.drawer_id "
        "JOIN drawer_groups g ON g.id = d.group_id "
        "WHERE me.item_id = ? "
        "ORDER BY (i.id = ?) DESC, g.pos_y, g.pos_x, g.id, d.row, d.col, i.id",
        {itemId, itemId});
    QList<ItemLocation> result;
    while (q.next())
        result.append({q.value(0).toInt(), q.value(1).toInt(), q.value(2).toString(),
                       q.value(3).toString(), q.value(4).toInt()});
    return result;
}

QList<ItemEntry> SqliteItemRepository::inCabinet(int cabinetId) const
{
    QSqlQuery q = SqlUtil::exec(m_db,
        "SELECT i.id, i.drawer_id, i.name, i.category, i.value, i.package, i.quantity, i.notes, "
        "s.min_stock, s.total, s.identity, g.name, d.label "
        "FROM items i "
        "JOIN item_stock s ON s.item_id = i.id "
        "JOIN drawers d ON d.id = i.drawer_id "
        "JOIN drawer_groups g ON g.id = d.group_id "
        "WHERE g.cabinet_id = ? "
        "ORDER BY lower(i.name), s.identity, g.pos_y, g.pos_x, g.id, d.row, d.col, i.id",
        {cabinetId});
    QList<ItemEntry> result;
    while (q.next())
        result.append({readItem(q), q.value(10).toString(), q.value(11).toString(),
                       q.value(12).toString()});
    return result;
}

void SqliteItemRepository::adjustQuantity(const QList<int> &ids, int delta)
{
    SqlUtil::Transaction tx(m_db);
    for (int id : ids)
        SqlUtil::exec(m_db, "UPDATE items SET quantity = MAX(0, quantity + ?) WHERE id = ?",
                      {delta, id});
    tx.commit();
}

void SqliteItemRepository::moveItems(const QList<int> &ids, int drawerId)
{
    SqlUtil::Transaction tx(m_db);
    for (int id : ids)
        SqlUtil::exec(m_db, "UPDATE items SET drawer_id = ? WHERE id = ?", {drawerId, id});
    tx.commit();
}

void SqliteItemRepository::copyItems(const QList<int> &ids, int drawerId)
{
    SqlUtil::Transaction tx(m_db);
    for (int id : ids)
        SqlUtil::exec(m_db,
            "INSERT INTO items (drawer_id, name, category, value, package, quantity, notes, "
            "min_stock) SELECT ?, name, category, value, package, quantity, notes, min_stock "
            "FROM items WHERE id = ?",
            {drawerId, id});
    tx.commit();
}
