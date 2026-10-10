#pragma once

#include <QWidget>

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
    ListPanel(ListKind kind, IListRepository &lists, QWidget *parent = nullptr);

    ListKind kind() const { return m_kind; }
    int lineCount() const;
    void reload();
    // Drawers of the selected lines; empty when nothing is selected.
    QList<int> selectedDrawerIds() const;
    void clearSelection();

    // The buttons; apply asks first.
    void removeSelected();
    void clearAll();
    void applyToStock();

signals:
    // The list itself changed; `description` names what, for the undo history.
    void listChanged(const QString &description);
    // The list was applied to the stock.
    void applied(const QString &description);
    void drawersChosen(const QList<int> &drawerIds);

private:
    void updateSummary();

    ListKind m_kind;
    IListRepository &m_lists;
    ListModel *m_model;
    QTableView *m_table;
    QLabel *m_summary;
    QPushButton *m_remove;
    QPushButton *m_clear;
    QPushButton *m_apply;
};
