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
    // Reloads the drawer's items, keeping the selection.
    void reload();

signals:
    void itemsChanged();

private:
    void addItem();
    void editItem();
    void removeItem();
    void adjustQuantity(int delta);
    QList<int> selectedItemIds() const;
    void updateButtons();

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
    QList<ItemDialog::DrawerChoice> m_destinations;
};
