#include "SqliteListRepository.h"

#include "SqlUtil.h"

namespace {

QString kindName(ListKind kind)
{
    return kind == ListKind::Pick ? "pick" : "restock";
}

} // namespace

QList<ListLine> SqliteListRepository::lines(ListKind kind) const
{
    QSqlQuery q = SqlUtil::exec(m_db,
        "SELECT l.id, l.quantity, i.id, i.drawer_id, i.name, i.category, i.value, i.package, "
        "i.quantity, i.notes, s.min_stock, s.total, g.name, d.label "
        "FROM list_lines l "
        "JOIN items i ON i.id = l.item_id "
        "JOIN item_stock s ON s.item_id = i.id "
        "JOIN drawers d ON d.id = i.drawer_id "
        "JOIN drawer_groups g ON g.id = d.group_id "
        "WHERE l.kind = ? "
        "ORDER BY lower(i.name), g.pos_y, g.pos_x, g.id, d.row, d.col, i.id",
        {kindName(kind)});

    QList<ListLine> result;
    while (q.next()) {
        ListLine line;
        line.id = q.value(0).toInt();
        line.kind = kind;
        line.quantity = q.value(1).toInt();
        line.item = Item{q.value(2).toInt(), q.value(3).toInt(), q.value(4).toString(),
                         q.value(5).toString(), q.value(6).toString(), q.value(7).toString(),
                         q.value(8).toInt(), q.value(9).toString(), std::nullopt};
        if (!q.value(10).isNull())
            line.item.minStock = q.value(10).toInt();
        line.item.totalQuantity = q.value(11).toInt();
        line.group = q.value(12).toString();
        line.drawerLabel = q.value(13).toString();
        result.append(line);
    }
    return result;
}

void SqliteListRepository::add(ListKind kind, const QList<int> &itemIds, int quantity)
{
    quantity = std::max(1, quantity);
    SqlUtil::Transaction tx(m_db);
    for (int id : itemIds) {
        // Unknown ids are skipped; a second add of the same entry raises its quantity.
        SqlUtil::exec(m_db,
            "INSERT INTO list_lines (kind, item_id, quantity) "
            "SELECT ?, id, ? FROM items WHERE id = ? "
            "ON CONFLICT (kind, item_id) DO UPDATE SET quantity = quantity + ?",
            {kindName(kind), quantity, id, quantity});
    }
    tx.commit();
}

void SqliteListRepository::setQuantity(int lineId, int quantity)
{
    SqlUtil::exec(m_db, "UPDATE list_lines SET quantity = ? WHERE id = ?",
                  {std::max(1, quantity), lineId});
}

void SqliteListRepository::remove(const QList<int> &lineIds)
{
    SqlUtil::Transaction tx(m_db);
    for (int id : lineIds)
        SqlUtil::exec(m_db, "DELETE FROM list_lines WHERE id = ?", {id});
    tx.commit();
}

void SqliteListRepository::clear(ListKind kind)
{
    SqlUtil::exec(m_db, "DELETE FROM list_lines WHERE kind = ?", {kindName(kind)});
}

ListResult SqliteListRepository::apply(ListKind kind)
{
    SqlUtil::Transaction tx(m_db);

    ListResult result;
    QSqlQuery counts = SqlUtil::exec(m_db,
        "SELECT COUNT(*), COALESCE(SUM(l.quantity > i.quantity), 0) "
        "FROM list_lines l JOIN items i ON i.id = l.item_id WHERE l.kind = ?",
        {kindName(kind)});
    counts.next();
    result.lines = counts.value(0).toInt();
    result.shortLines = kind == ListKind::Pick ? counts.value(1).toInt() : 0;

    const QString sign = kind == ListKind::Pick ? "-" : "+";
    SqlUtil::exec(m_db,
        QString("UPDATE items SET quantity = MAX(0, quantity %1 "
                "(SELECT l.quantity FROM list_lines l WHERE l.kind = ? AND l.item_id = items.id)) "
                "WHERE id IN (SELECT item_id FROM list_lines WHERE kind = ?)").arg(sign),
        {kindName(kind), kindName(kind)});
    SqlUtil::exec(m_db, "DELETE FROM list_lines WHERE kind = ?", {kindName(kind)});
    tx.commit();
    return result;
}
