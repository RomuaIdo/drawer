#pragma once

#include <QDialog>
#include <QList>
#include <QPair>

#include "core/Models.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QTableWidget;

// Edits one item, or all the entries of an item that is stored in several drawers.
class ItemDialog : public QDialog
{
    Q_OBJECT

public:
    struct DrawerChoice {
        int id;
        QString text;
    };

    // One entry: `drawers` lists the valid destinations, so editing an item can also move it.
    ItemDialog(const Item &item, const QList<DrawerChoice> &drawers, QWidget *parent = nullptr);
    // Every entry of an item: the common fields apply to all of them, and a sub-table lists
    // each occurrence with its own editable quantity.
    ItemDialog(const Item &item, const QList<ItemLocation> &occurrences, QWidget *parent = nullptr);

    Item item() const;
    // (item id, quantity) per occurrence; only for the second form.
    QList<QPair<int, int>> quantities() const;

private:
    void setup(const Item &item);
    QWidget *buildOccurrences(const QList<ItemLocation> &occurrences);

    Item m_item;
    QComboBox *m_drawer = nullptr;     // One-entry form only.
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
