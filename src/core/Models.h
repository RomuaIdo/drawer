#pragma once

#include <QMetaType>
#include <optional>
#include <QString>

#include "DrawerSize.h"

// Plain data types; id == 0 means "not persisted yet".

struct Cabinet {
    int id = 0;
    QString name;
    QString description;
};

// A rectangular block of drawers. A 1x1 group is an individual drawer.
struct DrawerGroup {
    int id = 0;
    int cabinetId = 0;
    QString name;
    int rows = 1;
    int cols = 1;
    double posX = 0; // Block position in the cabinet layout, in grid cells.
    double posY = 0; // Fractional when the block was moved freely.
    DrawerSize defaultSize = DrawerSize::Medium;
    QString color; // "#rrggbb"; empty colors the drawers by size.
};

Q_DECLARE_METATYPE(DrawerGroup)

struct Drawer {
    int id = 0;
    int groupId = 0;
    int row = 0;
    int col = 0;
    DrawerSize size = DrawerSize::Medium;
    QString label;
};

struct Item {
    int id = 0;
    int drawerId = 0;
    QString name;
    QString category;
    QString value; // Free text, e.g. "4.7k" or "100nF".
    QString package;
    int quantity = 0;
    QString notes;
    // Alert threshold, shared by every entry of the same item in the cabinet.
    std::optional<int> minStock;
    // Quantity of this item summed over all its drawers in the cabinet; set when read from
    // the database, -1 for items built in code.
    int totalQuantity = -1;

    int stockTotal() const { return totalQuantity >= 0 ? totalQuantity : quantity; }
    // True once the item's total reaches or drops below the minimum stock.
    bool isLowStock() const { return minStock && stockTotal() <= *minStock; }
};

// One place where an item is stored.
struct ItemLocation {
    int itemId = 0;
    int drawerId = 0;
    QString group;
    QString drawerLabel;
    int quantity = 0;
};

// An item together with where it is, for listing a whole cabinet.
struct ItemEntry {
    Item item;
    QString identity; // Equal for entries of the same item (name, category, value, package).
    QString group;
    QString drawerLabel;
};

// One flattened line of the cabinet mapping; empty drawers have no item fields.
struct MappingRow {
    QString cabinet;
    QString group;
    int row = 0; // 1-based within the group.
    int col = 0;
    QString drawerLabel;
    DrawerSize drawerSize = DrawerSize::Medium;
    QString itemName;
    QString category;
    QString value;
    QString package;
    int quantity = 0;
    QString notes;
    std::optional<int> minStock;
    bool lowStock = false;
    bool hasItem = false;
};
