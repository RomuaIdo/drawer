#pragma once

#include <QWidget>

#include "data/Repositories.h"

class AllItemsModel;
class QCheckBox;
class QTableView;

// Table of every item in a cabinet; picking a row points at the drawers holding the item.
class AllItemsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AllItemsPanel(IItemRepository &items, QWidget *parent = nullptr);

    void setCabinet(int cabinetId);
    void reload();
    // Drawers of the selected row; empty when nothing is selected.
    QList<int> selectedDrawerIds() const;
    void clearSelection();

signals:
    // The selection changed; empty list when it was cleared.
    void drawersChosen(const QList<int> &drawerIds);
    // Double click: the user wants to work on the item.
    void drawersActivated(const QList<int> &drawerIds);

private:
    IItemRepository &m_items;
    AllItemsModel *m_model;
    QTableView *m_table;
    QCheckBox *m_group;
    int m_cabinetId = 0;
};
