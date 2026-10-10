#pragma once

#include <QWidget>

#include "ItemDialog.h"
#include "data/Repositories.h"

class ItemTableModel;
class QLabel;
class QPushButton;
class QTableView;

// Lists and edits the items stored in the selected drawer.
class ItemPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ItemPanel(IItemRepository &items, QWidget *parent = nullptr);

    // `destinations` are the drawers an item may be moved to.
    void setDrawer(const Drawer &drawer, const QString &groupName,
                   const QList<ItemDialog::DrawerChoice> &destinations);
    void clear();
    void selectAll();
    void removeSelected() { removeItem(); } // Asks first.
    QList<int> selectedItemIds() const;
    // Reloads the drawer's items, keeping the selection.
    void reload();

    // Whether Paste is offered (something was copied).
    void setCanPaste(bool canPaste);

signals:
    // `description` names what changed, for the undo history.
    void itemsChanged(const QString &description);
    void copyRequested(const QList<int> &itemIds);
    void cutRequested(const QList<int> &itemIds);
    // The user asked to put the selected entries on the pick or the restock list.
    void addToListRequested(ListKind kind, const QList<int> &itemIds);
    void pasteRequested(); // Into the drawer shown in the panel.

private:
    void addItem();
    void editItem();
    void removeItem();
    void adjustQuantity(int delta);
    void updateButtons();
    void showContextMenu(const QPoint &pos);

    IItemRepository &m_items;
    ItemTableModel *m_model;
    QTableView *m_table;
    QLabel *m_title;
    QPushButton *m_add;
    QPushButton *m_edit;
    QPushButton *m_remove;
    QPushButton *m_decrease;
    QPushButton *m_increase;
    int m_drawerId = 0;
    bool m_canPaste = false;
    QList<ItemDialog::DrawerChoice> m_destinations;
};
