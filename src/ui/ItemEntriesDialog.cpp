#include "ItemEntriesDialog.h"

#include <QHeaderView>
#include <QSpinBox>
#include <QTableWidget>

ItemEntriesDialog::ItemEntriesDialog(const Item &item, const QList<ItemLocation> &occurrences,
                                     QWidget *parent)
    : ItemFormDialog(item, parent)
    , m_occurrences(buildTable(occurrences))
{
    setWindowTitle(tr("Edit item in every drawer"));
    buildForm({}, {tr("Quantities"), m_occurrences});
}

QTableWidget *ItemEntriesDialog::buildTable(const QList<ItemLocation> &occurrences)
{
    auto *table = new QTableWidget(occurrences.size(), 2);
    table->setHorizontalHeaderLabels({tr("Drawer"), tr("Quantity")});
    table->verticalHeader()->hide();
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setMinimumHeight(30 + 28 * qMin(occurrences.size(), 6));

    for (int row = 0; row < occurrences.size(); ++row) {
        const ItemLocation &location = occurrences[row];
        m_occurrenceIds.append(location.itemId);
        table->setItem(
            row, 0, new QTableWidgetItem(QString("%1/%2").arg(location.group, location.drawerLabel)));
        auto *spin = new QSpinBox;
        spin->setRange(0, 1000000);
        spin->setValue(location.quantity);
        table->setCellWidget(row, 1, spin);
    }
    return table;
}

QList<QPair<int, int>> ItemEntriesDialog::quantities() const
{
    QList<QPair<int, int>> result;
    for (int row = 0; row < m_occurrenceIds.size(); ++row) {
        const auto *spin = qobject_cast<QSpinBox *>(m_occurrences->cellWidget(row, 1));
        result.append({m_occurrenceIds[row], spin->value()});
    }
    return result;
}
