#include "ItemPanel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

#include "ItemTableModel.h"
#include "data/DatabaseError.h"

ItemPanel::ItemPanel(IItemRepository &items, QWidget *parent)
    : QWidget(parent)
    , m_items(items)
    , m_model(new ItemTableModel(this))
    , m_table(new QTableView)
    , m_title(new QLabel)
    , m_add(new QPushButton(tr("Add")))
    , m_edit(new QPushButton(tr("Edit")))
    , m_remove(new QPushButton(tr("Remove")))
{
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_add);
    buttons->addWidget(m_edit);
    buttons->addWidget(m_remove);
    buttons->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_title);
    layout->addWidget(m_table);
    layout->addLayout(buttons);

    connect(m_add, &QPushButton::clicked, this, &ItemPanel::addItem);
    connect(m_edit, &QPushButton::clicked, this, &ItemPanel::editItem);
    connect(m_remove, &QPushButton::clicked, this, &ItemPanel::removeItem);
    connect(m_table, &QTableView::doubleClicked, this, &ItemPanel::editItem);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &ItemPanel::updateButtons);

    clear();
}

void ItemPanel::setDrawer(const Drawer &drawer, const QList<ItemDialog::DrawerChoice> &destinations)
{
    m_drawerId = drawer.id;
    m_destinations = destinations;
    m_title->setText(tr("Items in drawer %1").arg(drawer.label));
    reload();
}

void ItemPanel::clear()
{
    m_drawerId = 0;
    m_title->setText(tr("Select a drawer"));
    m_model->setItems({});
    updateButtons();
}

void ItemPanel::reload()
{
    try {
        m_model->setItems(m_items.inDrawer(m_drawerId));
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
    }
    updateButtons();
}

void ItemPanel::updateButtons()
{
    const bool hasDrawer = m_drawerId != 0;
    const bool hasSelection = m_table->selectionModel()->hasSelection();
    m_add->setEnabled(hasDrawer);
    m_edit->setEnabled(hasSelection);
    m_remove->setEnabled(hasSelection);
}

void ItemPanel::addItem()
{
    Item item;
    item.drawerId = m_drawerId;
    ItemDialog dialog(item, m_destinations, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    try {
        m_items.add(dialog.item());
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    reload();
    emit itemsChanged();
}

void ItemPanel::editItem()
{
    const QModelIndex index = m_table->currentIndex();
    if (!index.isValid())
        return;
    ItemDialog dialog(m_model->itemAt(index.row()), m_destinations, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    try {
        m_items.update(dialog.item());
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    reload();
    emit itemsChanged();
}

void ItemPanel::removeItem()
{
    const QModelIndex index = m_table->currentIndex();
    if (!index.isValid())
        return;
    const Item &item = m_model->itemAt(index.row());
    if (QMessageBox::question(this, tr("Remove item"), tr("Remove \"%1\"?").arg(item.name))
        != QMessageBox::Yes)
        return;
    try {
        m_items.remove(item.id);
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    reload();
    emit itemsChanged();
}
