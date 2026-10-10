#include "SqliteDrawerRepository.h"

#include "SqlUtil.h"

QList<DrawerGroup> SqliteDrawerRepository::groupsOf(int cabinetId) const
{
    QList<DrawerGroup> result;
    QSqlQuery q = SqlUtil::exec(m_db,
        "SELECT id, cabinet_id, name, rows, cols, pos_x, pos_y, default_size, color "
        "FROM drawer_groups WHERE cabinet_id = ? ORDER BY pos_y, pos_x, id",
        {cabinetId});
    while (q.next()) {
        result.append({q.value(0).toInt(), q.value(1).toInt(), q.value(2).toString(),
                       q.value(3).toInt(), q.value(4).toInt(), q.value(5).toDouble(),
                       q.value(6).toDouble(), drawerSizeFromString(q.value(7).toString()),
                       q.value(8).toString()});
    }
    return result;
}

DrawerGroup SqliteDrawerRepository::addGroup(DrawerGroup group)
{
    SqlUtil::Transaction tx(m_db);
    QSqlQuery q = SqlUtil::exec(m_db,
        "INSERT INTO drawer_groups (cabinet_id, name, rows, cols, pos_x, pos_y, default_size, color) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
        {group.cabinetId, group.name, group.rows, group.cols, group.posX, group.posY,
         drawerSizeToString(group.defaultSize), group.color});
    group.id = q.lastInsertId().toInt();

    for (int r = 0; r < group.rows; ++r) {
        for (int c = 0; c < group.cols; ++c) {
            SqlUtil::exec(m_db,
                "INSERT INTO drawers (group_id, row, col, size, label) VALUES (?, ?, ?, ?, ?)",
                {group.id, r, c, drawerSizeToString(group.defaultSize),
                 QString("R%1C%2").arg(r + 1).arg(c + 1)});
        }
    }
    tx.commit();
    return group;
}

void SqliteDrawerRepository::updateGroup(const DrawerGroup &group)
{
    SqlUtil::exec(m_db,
        "UPDATE drawer_groups SET name = ?, pos_x = ?, pos_y = ?, default_size = ?, color = ? "
        "WHERE id = ?",
        {group.name, group.posX, group.posY, drawerSizeToString(group.defaultSize), group.color,
         group.id});
}

void SqliteDrawerRepository::moveGroups(const QList<DrawerGroup> &groups)
{
    SqlUtil::Transaction tx(m_db);
    for (const DrawerGroup &g : groups)
        SqlUtil::exec(m_db, "UPDATE drawer_groups SET pos_x = ?, pos_y = ? WHERE id = ?",
                      {g.posX, g.posY, g.id});
    tx.commit();
}

void SqliteDrawerRepository::resizeGroup(const DrawerGroup &group,
                                          const QList<DrawerGroup> &displaced)
{
    SqlUtil::Transaction tx(m_db);

    QSqlQuery before = SqlUtil::exec(m_db, "SELECT rows, cols FROM drawer_groups WHERE id = ?",
                                     {group.id});
    before.next();
    const int oldRows = before.value(0).toInt();
    const int oldCols = before.value(1).toInt();

    SqlUtil::exec(m_db, "DELETE FROM drawers WHERE group_id = ? AND (row >= ? OR col >= ?)",
                  {group.id, group.rows, group.cols});

    // Only cells outside the old size are new; drawers deleted earlier stay deleted.
    for (int r = 0; r < group.rows; ++r) {
        for (int c = 0; c < group.cols; ++c) {
            if (r < oldRows && c < oldCols)
                continue;
            SqlUtil::exec(m_db,
                "INSERT OR IGNORE INTO drawers (group_id, row, col, size, label) "
                "VALUES (?, ?, ?, ?, ?)",
                {group.id, r, c, drawerSizeToString(group.defaultSize),
                 QString("R%1C%2").arg(r + 1).arg(c + 1)});
        }
    }

    SqlUtil::exec(m_db,
        "UPDATE drawer_groups SET name = ?, rows = ?, cols = ?, pos_x = ?, pos_y = ?, "
        "default_size = ?, color = ? WHERE id = ?",
        {group.name, group.rows, group.cols, group.posX, group.posY,
         drawerSizeToString(group.defaultSize), group.color, group.id});

    for (const DrawerGroup &other : displaced) {
        SqlUtil::exec(m_db, "UPDATE drawer_groups SET pos_x = ?, pos_y = ? WHERE id = ?",
                      {other.posX, other.posY, other.id});
    }
    tx.commit();
}

int SqliteDrawerRepository::itemsLostByResize(int groupId, int rows, int cols) const
{
    QSqlQuery q = SqlUtil::exec(m_db,
        "SELECT COUNT(*) FROM items WHERE drawer_id IN "
        "(SELECT id FROM drawers WHERE group_id = ? AND (row >= ? OR col >= ?))",
        {groupId, rows, cols});
    q.next();
    return q.value(0).toInt();
}

void SqliteDrawerRepository::removeDrawer(int drawerId)
{
    SqlUtil::Transaction tx(m_db);
    QSqlQuery q = SqlUtil::exec(m_db, "SELECT group_id FROM drawers WHERE id = ?", {drawerId});
    if (!q.next())
        return;
    const int groupId = q.value(0).toInt();

    SqlUtil::exec(m_db, "DELETE FROM drawers WHERE id = ?", {drawerId});
    QSqlQuery left = SqlUtil::exec(m_db, "SELECT COUNT(*) FROM drawers WHERE group_id = ?", {groupId});
    left.next();
    if (left.value(0).toInt() == 0)
        SqlUtil::exec(m_db, "DELETE FROM drawer_groups WHERE id = ?", {groupId});
    tx.commit();
}

void SqliteDrawerRepository::removeGroup(int id)
{
    SqlUtil::exec(m_db, "DELETE FROM drawer_groups WHERE id = ?", {id});
}

QList<Drawer> SqliteDrawerRepository::drawersOf(int groupId) const
{
    QList<Drawer> result;
    QSqlQuery q = SqlUtil::exec(m_db,
        "SELECT id, group_id, row, col, size, label FROM drawers "
        "WHERE group_id = ? ORDER BY row, col",
        {groupId});
    while (q.next()) {
        result.append({q.value(0).toInt(), q.value(1).toInt(), q.value(2).toInt(),
                       q.value(3).toInt(), drawerSizeFromString(q.value(4).toString()),
                       q.value(5).toString()});
    }
    return result;
}

void SqliteDrawerRepository::updateDrawer(const Drawer &drawer)
{
    SqlUtil::exec(m_db, "UPDATE drawers SET size = ?, label = ? WHERE id = ?",
                  {drawerSizeToString(drawer.size), drawer.label, drawer.id});
}
