#include "SqliteMappingRepository.h"

#include "SqlUtil.h"

QList<MappingRow> SqliteMappingRepository::rows() const
{
    QSqlQuery q = SqlUtil::exec(m_db,
        "SELECT c.name, g.name, d.row, d.col, d.label, d.size, "
        "       i.name, i.category, i.value, i.package, i.quantity, i.notes "
        "FROM drawers d "
        "JOIN drawer_groups g ON g.id = d.group_id "
        "JOIN cabinets c ON c.id = g.cabinet_id "
        "LEFT JOIN items i ON i.drawer_id = d.id "
        "ORDER BY c.name, g.pos_y, g.pos_x, g.id, d.row, d.col, i.name");

    QList<MappingRow> result;
    while (q.next()) {
        MappingRow r;
        r.cabinet = q.value(0).toString();
        r.group = q.value(1).toString();
        r.row = q.value(2).toInt() + 1;
        r.col = q.value(3).toInt() + 1;
        r.drawerLabel = q.value(4).toString();
        r.drawerSize = drawerSizeFromString(q.value(5).toString());
        r.hasItem = !q.value(6).isNull();
        if (r.hasItem) {
            r.itemName = q.value(6).toString();
            r.category = q.value(7).toString();
            r.value = q.value(8).toString();
            r.package = q.value(9).toString();
            r.quantity = q.value(10).toInt();
            r.notes = q.value(11).toString();
        }
        result.append(r);
    }
    return result;
}
