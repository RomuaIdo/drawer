#include "AllItemsPanel.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QMessageBox>
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

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_group);
    layout->addWidget(m_table);

    connect(m_group, &QCheckBox::toggled, this, [this](bool grouped) {
        m_table->clearSelection();
        m_model->setGrouped(grouped);
    });
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this] { emit drawersChosen(selectedDrawerIds()); });
    connect(m_table, &QTableView::doubleClicked, this,
            [this] { emit drawersActivated(selectedDrawerIds()); });
}

void AllItemsPanel::setCabinet(int cabinetId)
{
    m_cabinetId = cabinetId;
    reload();
}

void AllItemsPanel::reload()
{
    m_table->clearSelection();
    try {
        m_model->setEntries(m_cabinetId ? m_items.inCabinet(m_cabinetId) : QList<ItemEntry>());
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
    }
}

QList<int> AllItemsPanel::selectedDrawerIds() const
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? QList<int>() : m_model->drawerIdsAt(rows.first().row());
}

void AllItemsPanel::clearSelection()
{
    m_table->clearSelection();
}
