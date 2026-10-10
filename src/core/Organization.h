#pragma once

#include <QList>

#include "Models.h"

// The complete organization: cabinets, their drawer groups, drawers and items.
// Ids are ignored when importing; new ones are assigned.

struct DrawerEntry {
    Drawer drawer;
    QList<Item> items;
};

struct GroupEntry {
    DrawerGroup group;
    QList<DrawerEntry> drawers;
};

struct CabinetEntry {
    Cabinet cabinet;
    QList<GroupEntry> groups;
};

struct Organization {
    QList<CabinetEntry> cabinets;
};
