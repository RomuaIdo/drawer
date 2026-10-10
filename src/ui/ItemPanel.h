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

signals:
    void itemsChanged();

private:
    void reload();
    void addItem();
    void editItem();
    void removeItem();
    void updateButtons();

    IItemRepository &m_items;
    ItemTableModel *m_model;
    QTableView *m_table;
    QLabel *m_title;
    QPushButton *m_add;
    QPushButton *m_edit;
    QPushButton *m_remove;
    int m_drawerId = 0;
    QList<ItemDialog::DrawerChoice> m_destinations;
};
