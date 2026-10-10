#include "SqliteOrganizationRepository.h"

#include "SqlUtil.h"
#include "SqliteCabinetRepository.h"
#include "SqliteDrawerRepository.h"
#include "SqliteItemRepository.h"

Organization SqliteOrganizationRepository::load() const
{
    const SqliteCabinetRepository cabinets(m_db);
    const SqliteDrawerRepository drawers(m_db);
    const SqliteItemRepository items(m_db);

    Organization result;
    for (const Cabinet &cabinet : cabinets.all()) {
        CabinetEntry cabinetEntry{cabinet, {}};
        for (const DrawerGroup &group : drawers.groupsOf(cabinet.id)) {
            GroupEntry groupEntry{group, {}};
            for (const Drawer &drawer : drawers.drawersOf(group.id))
                groupEntry.drawers.append({drawer, items.inDrawer(drawer.id)});
            cabinetEntry.groups.append(groupEntry);
        }
        result.cabinets.append(cabinetEntry);
    }
    return result;
}

void SqliteOrganizationRepository::import(const Organization &organization)
{
    SqlUtil::Transaction tx(m_db);
    for (const CabinetEntry &cabinet : organization.cabinets) {
        const int cabinetId = SqlUtil::exec(m_db,
            "INSERT INTO cabinets (name, description) VALUES (?, ?)",
            {cabinet.cabinet.name, cabinet.cabinet.description}).lastInsertId().toInt();

        for (const GroupEntry &entry : cabinet.groups) {
            const DrawerGroup &g = entry.group;
            const int groupId = SqlUtil::exec(m_db,
                "INSERT INTO drawer_groups (cabinet_id, name, rows, cols, pos_x, pos_y, default_size, "
                "color) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                {cabinetId, g.name, g.rows, g.cols, g.posX, g.posY,
                 drawerSizeToString(g.defaultSize), g.color}).lastInsertId().toInt();

            for (const DrawerEntry &drawerEntry : entry.drawers) {
                const Drawer &d = drawerEntry.drawer;
                const int drawerId = SqlUtil::exec(m_db,
                    "INSERT INTO drawers (group_id, row, col, size, label) VALUES (?, ?, ?, ?, ?)",
                    {groupId, d.row, d.col, drawerSizeToString(d.size), d.label})
                                         .lastInsertId().toInt();
                for (const Item &i : drawerEntry.items) {
                    SqlUtil::exec(m_db,
                        "INSERT INTO items (drawer_id, name, category, value, package, quantity, notes) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?)",
                        {drawerId, i.name, i.category, i.value, i.package, i.quantity, i.notes});
                }
            }
        }
    }
    tx.commit();
}
