#include "AllItemsPanel.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

#include "AllItemsModel.h"
#include "data/DatabaseError.h"

AllItemsPanel::AllItemsPanel(IItemRepository &items, QWidget *parent)
    : QWidget(parent)
    , m_items(items)
    , m_model(new AllItemsModel(this))
    , m_table(new QTableView)
    , m_group(new QCheckBox(tr("Group duplicates")))
    , m_add(new QPushButton(tr("Add")))
    , m_edit(new QPushButton(tr("Edit")))
    , m_remove(new QPushButton(tr("Remove")))
    , m_decrease(new QPushButton(QString::fromUtf8("−")))
    , m_increase(new QPushButton("+"))
{
    m_group->setToolTip(tr("Show an item stored in several drawers on a single row"));
    m_group->setChecked(false);

    m_model->setLocationProvider([this](int itemId) {
        try {
            return m_items.locationsOf(itemId);
        } catch (const DatabaseError &) {
            return QList<ItemLocation>();
        }
    });
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();

    m_decrease->setToolTip(tr("Take one from the selected item"));
    m_increase->setToolTip(tr("Add one to the selected item"));
    for (QPushButton *button : {m_decrease, m_increase})
        button->setFixedWidth(36);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_add);
    buttons->addWidget(m_edit);
    buttons->addWidget(m_remove);
    buttons->addStretch();
    buttons->addWidget(m_decrease);
    buttons->addWidget(m_increase);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_group);
    layout->addWidget(m_table);
    layout->addLayout(buttons);

    connect(m_group, &QCheckBox::toggled, this, [this](bool grouped) {
        m_table->clearSelection();
        m_model->setGrouped(grouped);
        updateButtons();
    });
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        updateButtons();
        emit drawersChosen(selectedDrawerIds());
    });
    connect(m_table, &QTableView::doubleClicked, this,
            [this] { emit drawersActivated(selectedDrawerIds()); });
    connect(m_add, &QPushButton::clicked, this, &AllItemsPanel::addItem);
    connect(m_edit, &QPushButton::clicked, this, &AllItemsPanel::editItem);
    connect(m_remove, &QPushButton::clicked, this, &AllItemsPanel::removeItems);
    connect(m_decrease, &QPushButton::clicked, this, [this] { adjustQuantity(-1); });
    connect(m_increase, &QPushButton::clicked, this, [this] { adjustQuantity(+1); });
    updateButtons();
}

void AllItemsPanel::setCabinet(int cabinetId)
{
    m_cabinetId = cabinetId;
    reload();
    updateButtons();
}

void AllItemsPanel::setCategoryFilter(const QString &category)
{
    if (category == m_category)
        return;
    m_category = category;
    reload();
}

void AllItemsPanel::setDestinations(const QList<ItemDialog::DrawerChoice> &drawers)
{
    m_destinations = drawers;
    updateButtons();
}

void AllItemsPanel::reload()
{
    m_table->clearSelection();
    try {
        QList<ItemEntry> entries = m_cabinetId ? m_items.inCabinet(m_cabinetId) : QList<ItemEntry>();
        if (!m_category.isEmpty()) {
            entries.removeIf([this](const ItemEntry &e) {
                return e.item.category.trimmed().compare(m_category, Qt::CaseInsensitive) != 0;
            });
        }
        m_model->setEntries(entries);
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
    }
}

void AllItemsPanel::reloadKeeping(int itemId)
{
    reload();
    const int row = m_model->rowOfItem(itemId);
    if (row >= 0)
        m_table->selectRow(row);
}

void AllItemsPanel::updateButtons()
{
    const bool hasSelection = m_table->selectionModel()->hasSelection();
    m_add->setEnabled(m_cabinetId != 0 && !m_destinations.isEmpty());
    m_edit->setEnabled(hasSelection);
    m_remove->setEnabled(hasSelection);
    // One count per entry: with duplicates merged there is no single quantity to nudge.
    const bool single = hasSelection && !m_model->grouped();
    m_decrease->setEnabled(single);
    m_increase->setEnabled(single);
    m_decrease->setToolTip(m_model->grouped() ? tr("Ungroup duplicates to adjust a quantity")
                                              : tr("Take one from the selected item"));
    m_increase->setToolTip(m_model->grouped() ? tr("Ungroup duplicates to adjust a quantity")
                                              : tr("Add one to the selected item"));
}

QList<int> AllItemsPanel::selectedDrawerIds() const
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? QList<int>() : m_model->drawerIdsAt(rows.first().row());
}

QList<int> AllItemsPanel::selectedItemIds() const
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? QList<int>() : m_model->itemIdsAt(rows.first().row());
}

void AllItemsPanel::clearSelection()
{
    m_table->clearSelection();
}

void AllItemsPanel::addItem()
{
    if (m_destinations.isEmpty())
        return;
    Item item;
    item.drawerId = m_defaultDrawerId ? m_defaultDrawerId : m_destinations.first().id;
    ItemDialog dialog(item, m_destinations, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    try {
        const Item added = m_items.add(dialog.item());
        reloadKeeping(added.id);
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    emit itemsChanged(tr("Add item"));
}

// One entry opens the usual dialog; an item stored in several drawers (grouped rows) opens it
// with a sub-table of its occurrences, so each quantity can be changed.
void AllItemsPanel::editItem()
{
    const QList<int> ids = selectedItemIds();
    if (ids.isEmpty())
        return;
    try {
        const auto first = m_items.find(ids.first());
        if (!first)
            return;

        if (!m_model->grouped()) {
            ItemDialog dialog(*first, m_destinations, this);
            if (dialog.exec() != QDialog::Accepted)
                return;
            m_items.update(dialog.item());
        } else {
            ItemDialog dialog(*first, m_items.locationsOf(ids.first()), this);
            if (dialog.exec() != QDialog::Accepted)
                return;
            m_items.updateEntries(dialog.item(), dialog.quantities());
        }
        reloadKeeping(ids.first());
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    emit itemsChanged(tr("Edit item"));
}

void AllItemsPanel::removeItems()
{
    const QList<int> ids = selectedItemIds();
    if (ids.isEmpty())
        return;
    const QString question = ids.size() == 1
        ? tr("Remove this item?")
        : tr("Remove this item from all %n drawer(s)?", "", ids.size());
    if (QMessageBox::question(this, tr("Remove item"), question) != QMessageBox::Yes)
        return;
    try {
        for (int id : ids)
            m_items.remove(id);
        reload();
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    emit itemsChanged(tr("Remove item(s)"));
}

void AllItemsPanel::adjustQuantity(int delta)
{
    if (m_model->grouped())
        return;
    const QList<int> ids = selectedItemIds();
    if (ids.isEmpty())
        return;
    try {
        m_items.adjustQuantity(ids, delta);
        reloadKeeping(ids.first());
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    emit itemsChanged(tr("Adjust quantity"));
}
