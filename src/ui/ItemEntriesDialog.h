#pragma once

#include <QList>
#include <QPair>

#include "ItemFormDialog.h"

class QTableWidget;

// Edits every entry of an item that is stored in several drawers: the shared fields apply to
// all of them, and a sub-table lists each occurrence with its own editable quantity.
class ItemEntriesDialog : public ItemFormDialog
{
    Q_OBJECT

public:
    ItemEntriesDialog(const Item &item, const QList<ItemLocation> &occurrences,
                      QWidget *parent = nullptr);

    // (item id, quantity) per occurrence.
    QList<QPair<int, int>> quantities() const;

private:
    QTableWidget *buildTable(const QList<ItemLocation> &occurrences);

    QList<int> m_occurrenceIds; // Declared first: buildTable() fills it.
    QTableWidget *m_occurrences;
};
