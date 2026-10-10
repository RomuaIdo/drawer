#pragma once

#include <QSqlDatabase>

#include "Repositories.h"

class SqliteDrawerRepository : public IDrawerRepository
{
public:
    explicit SqliteDrawerRepository(QSqlDatabase db) : m_db(db) {}

    QList<DrawerGroup> groupsOf(int cabinetId) const override;
    DrawerGroup addGroup(DrawerGroup group) override;
    void updateGroup(const DrawerGroup &group) override;
    void resizeGroup(const DrawerGroup &group, const QList<DrawerGroup> &displaced) override;
    int itemsLostByResize(int groupId, int rows, int cols) const override;
    void removeGroup(int id) override;
    QList<Drawer> drawersOf(int groupId) const override;
    void updateDrawer(const Drawer &drawer) override;

private:
    QSqlDatabase m_db;
};
