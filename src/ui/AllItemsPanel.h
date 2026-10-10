#pragma once

#include <QWidget>

#include "ItemDialog.h"
#include "data/Repositories.h"

class AllItemsModel;
class QCheckBox;
class QPushButton;
class QTableView;

// Table of every item in a cabinet. Picking a row points at the drawers holding the item,
// and the buttons add, edit, remove and adjust items from here.
class AllItemsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AllItemsPanel(IItemRepository &items, QWidget *parent = nullptr);

    void setCabinet(int cabinetId);
    // Only items of this category are listed; empty lists everything.
    void setCategoryFilter(const QString &category);
    // Drawers an item can be put in, and the one a new item starts in.
    void setDestinations(const DrawerLayouts &drawers);
    void setDefaultDrawer(int drawerId) { m_defaultDrawerId = drawerId; }

    void reload();
    // Drawers of the selected row; empty when nothing is selected.
    QList<int> selectedDrawerIds() const;
    // Item entries behind the selected row.
    QList<int> selectedItemIds() const;
    void clearSelection();

    // The buttons; each asks for what it needs and reports through itemsChanged().
    void addItem();
    void editItem();
    void removeItems(); // Asks first.
    void adjustQuantity(int delta); // Only when duplicates are not grouped.

signals:
    // The selection changed; empty list when it was cleared.
    void drawersChosen(const QList<int> &drawerIds);
    // Double click: the user wants to work on the item.
    void drawersActivated(const QList<int> &drawerIds);
    // Items were changed; `description` names what, for the undo history.
    void itemsChanged(const QString &description);
    // The user asked to put the selected entry on the pick or the restock list.
    void addToListRequested(ListKind kind, const QList<int> &itemIds);

private:
    void updateButtons();
    void showContextMenu(const QPoint &pos);
    // Reloads and selects the row of `itemId` again, if it is still listed.
    void reloadKeeping(int itemId);

    IItemRepository &m_items;
    AllItemsModel *m_model;
    QTableView *m_table;
    QCheckBox *m_group;
    QPushButton *m_add;
    QPushButton *m_edit;
    QPushButton *m_remove;
    QPushButton *m_decrease;
    QPushButton *m_increase;
    int m_cabinetId = 0;
    int m_defaultDrawerId = 0;
    QString m_category;
    DrawerLayouts m_destinations;
};
