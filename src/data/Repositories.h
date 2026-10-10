#pragma once

#include <QList>

#include "core/Models.h"

// Persistence interfaces. All methods throw DatabaseError on failure.

class ICabinetRepository
{
public:
    virtual ~ICabinetRepository() = default;
    virtual QList<Cabinet> all() const = 0;
    virtual Cabinet add(Cabinet cabinet) = 0; // Returns the cabinet with its id set.
    virtual void update(const Cabinet &cabinet) = 0;
    virtual void remove(int id) = 0;
};

class IDrawerRepository
{
public:
    virtual ~IDrawerRepository() = default;
    virtual QList<DrawerGroup> groupsOf(int cabinetId) const = 0;
    // Creates the group and one drawer per cell, using the default size.
    virtual DrawerGroup addGroup(DrawerGroup group) = 0;
    // Name, position and default size only; the dimensions are fixed once created.
    virtual void updateGroup(const DrawerGroup &group) = 0;
    virtual void removeGroup(int id) = 0;
    virtual QList<Drawer> drawersOf(int groupId) const = 0;
    virtual void updateDrawer(const Drawer &drawer) = 0;
};

class IItemRepository
{
public:
    virtual ~IItemRepository() = default;
    virtual QList<Item> inDrawer(int drawerId) const = 0;
    // Case-insensitive match on name, category, value, package and notes.
    virtual QList<Item> search(const QString &text) const = 0;
    virtual Item add(Item item) = 0;
    virtual void update(const Item &item) = 0; // Also used to move an item between drawers.
    virtual void remove(int id) = 0;
};

// Read-only flattened view used by exporters.
class IMappingRepository
{
public:
    virtual ~IMappingRepository() = default;
    // One row per item, plus one row per empty drawer.
    virtual QList<MappingRow> rows() const = 0;
};

// Bundles the repositories handed to the UI.
struct Repositories {
    ICabinetRepository &cabinets;
    IDrawerRepository &drawers;
    IItemRepository &items;
    IMappingRepository &mapping;
};
