#pragma once

#include <QWidget>

#include "CabinetView.h"
#include "data/Repositories.h"

class ListModel;
class QLabel;
class QPushButton;
class QTableView;

// A pick list or a restock list: shows its lines, lets the quantities be edited, and applies
// them to the stock in one step.
class ListPanel : public QWidget
{
    Q_OBJECT

public:
    ListPanel(ListKind kind, IListRepository &lists, IItemRepository &items,
              QWidget *parent = nullptr);

    ListKind kind() const { return m_kind; }
    // Drawers a new item can go in, and the one the picker starts on.
    void setDestinations(const DrawerLayouts &drawers);
    void setDefaultDrawer(int drawerId) { m_defaultDrawerId = drawerId; }
    int lineCount() const;
    void reload();
    // Drawers of the selected lines; empty when nothing is selected.
    QList<int> selectedDrawerIds() const;
    void clearSelection();

    // The buttons; apply asks first.
    void addNewItem(); // Restock list only: creates an item with no stock and lists it.
    void removeSelected();
    void clearAll();
    void applyToStock();

signals:
    // The list itself changed; `description` names what, for the undo history.
    void listChanged(const QString &description);
    // The list was applied to the stock.
    void applied(const QString &description);
    // A new item was created too, so item views have to refresh.
    void itemsChanged(const QString &description);
    void drawersChosen(const QList<int> &drawerIds);

private:
    void updateSummary();

    ListKind m_kind;
    IListRepository &m_lists;
    IItemRepository &m_items;
    ListModel *m_model;
    QTableView *m_table;
    QLabel *m_summary;
    QPushButton *m_add;
    QPushButton *m_remove;
    QPushButton *m_clear;
    QPushButton *m_apply;
    DrawerLayouts m_destinations;
    int m_defaultDrawerId = 0;
};
