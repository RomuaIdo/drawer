#pragma once

#include <QDialog>
#include <QList>
#include <QPair>

#include "CabinetView.h"
#include "core/Models.h"

class QCheckBox;
class QLabel;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QTableWidget;

// Edits one item, or all the entries of an item that is stored in several drawers.
class ItemDialog : public QDialog
{
    Q_OBJECT

public:
    // One entry. The drawer is picked by clicking it on a small view of the cabinet
    // (`drawers`), so editing an item can also move it.
    ItemDialog(const Item &item, const DrawerLayouts &drawers, QWidget *parent = nullptr);
    // Every entry of an item: the common fields apply to all of them, and a sub-table lists
    // each occurrence with its own editable quantity.
    ItemDialog(const Item &item, const QList<ItemLocation> &occurrences, QWidget *parent = nullptr);

    // For items that are about to be restocked: the quantity is how many to add.
    void setRestockMode();
    Item item() const;
    // (item id, quantity) per occurrence; only for the second form.
    QList<QPair<int, int>> quantities() const;

private:
    void setup(const Item &item);
    void chooseDrawer(int drawerId);
    void updateOk();
    QWidget *buildOccurrences(const QList<ItemLocation> &occurrences);

    Item m_item;
    CabinetView *m_picker = nullptr;   // One-entry form only.
    QLabel *m_drawerText = nullptr;
    QPushButton *m_ok = nullptr;
    DrawerLayouts m_layouts;
    int m_drawerId = 0;
    QSpinBox *m_quantity = nullptr;    // One-entry form only.
    QTableWidget *m_occurrences = nullptr; // Every-entry form only.
    QList<int> m_occurrenceIds;
    QLineEdit *m_name = nullptr;
    QLineEdit *m_category = nullptr;
    QLineEdit *m_value = nullptr;
    QLineEdit *m_package = nullptr;
    QCheckBox *m_alert = nullptr;
    QSpinBox *m_minStock = nullptr;
    QLineEdit *m_notes = nullptr;
};
