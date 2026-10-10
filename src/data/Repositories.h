#pragma once

#include <QList>

#include "core/Models.h"
#include "core/Organization.h"

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
    // Name, position and default size only; use resizeGroup to change the dimensions.
    virtual void updateGroup(const DrawerGroup &group) = 0;
    // Stores the position of several blocks at once.
    virtual void moveGroups(const QList<DrawerGroup> &groups) = 0;
    // Applies the group's new rows/cols: new cells get drawers (default size), cells that
    // no longer exist lose their drawers and items. `displaced` blocks (only their
    // position is used) are moved in the same transaction.
    virtual void resizeGroup(const DrawerGroup &group, const QList<DrawerGroup> &displaced) = 0;
    // Items that resizing the group to rows x cols would delete.
    virtual int itemsLostByResize(int groupId, int rows, int cols) const = 0;
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
    // Items at or below their minimum stock, across every cabinet.
    virtual QList<Item> lowStock() const = 0;
    // Adds `delta` to each item's quantity, never going below zero.
    virtual void adjustQuantity(const QList<int> &ids, int delta) = 0;
    // Moves the items to another drawer, or adds copies of them there. Both are atomic;
    // ids that no longer exist are skipped.
    virtual void moveItems(const QList<int> &ids, int drawerId) = 0;
    virtual void copyItems(const QList<int> &ids, int drawerId) = 0;
};

// Read-only flattened view used by exporters.
class IMappingRepository
{
public:
    virtual ~IMappingRepository() = default;
    // One row per item, plus one row per empty drawer.
    virtual QList<MappingRow> rows() const = 0;
};


// Whole-database snapshot, for backup and sharing.
class IOrganizationRepository
{
public:
    virtual ~IOrganizationRepository() = default;
    virtual Organization load() const = 0;
    // Adds everything as new records in a single transaction; existing data is untouched.
    virtual void import(const Organization &organization) = 0;
};

// Undo/redo of data changes. Changes accumulate until commit() closes them as one step.
class IUndoHistory
{
public:
    virtual ~IUndoHistory() = default;
    // Closes everything changed since the last call as one undoable step; no-op if nothing
    // changed. Starting a new step discards the redo history.
    virtual void commit(const QString &label) = 0;
    virtual bool canUndo() const = 0;
    virtual bool canRedo() const = 0;
    virtual QString undoLabel() const = 0;
    virtual QString redoLabel() const = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;
};

// Bundles the repositories handed to the UI.
struct Repositories {
    ICabinetRepository &cabinets;
    IDrawerRepository &drawers;
    IItemRepository &items;
    IMappingRepository &mapping;
    IOrganizationRepository &organization;
    IUndoHistory &undo;
};
