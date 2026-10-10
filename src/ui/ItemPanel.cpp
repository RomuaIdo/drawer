#include "ItemPanel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
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
    , m_decrease(new QPushButton(QString::fromUtf8("\u2212")))
    , m_increase(new QPushButton("+"))
{
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setDragEnabled(true);
    m_table->setDragDropMode(QAbstractItemView::DragOnly);
    m_table->setDefaultDropAction(Qt::MoveAction);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_add);
    buttons->addWidget(m_edit);
    buttons->addWidget(m_remove);
    buttons->addStretch();
    buttons->addWidget(m_decrease);
    buttons->addWidget(m_increase);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_title);
    layout->addWidget(m_table);
    layout->addLayout(buttons);

    connect(m_add, &QPushButton::clicked, this, &ItemPanel::addItem);
    connect(m_edit, &QPushButton::clicked, this, &ItemPanel::editItem);
    connect(m_remove, &QPushButton::clicked, this, &ItemPanel::removeItem);
    m_decrease->setToolTip(tr("Take one from the selected items"));
    m_increase->setToolTip(tr("Add one to the selected items"));
    for (QPushButton *button : {m_decrease, m_increase})
        button->setFixedWidth(36);
    connect(m_decrease, &QPushButton::clicked, this, [this] { adjustQuantity(-1); });
    connect(m_increase, &QPushButton::clicked, this, [this] { adjustQuantity(+1); });
    connect(m_table, &QTableView::doubleClicked, this, &ItemPanel::editItem);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &ItemPanel::updateButtons);

    connect(m_table, &QWidget::customContextMenuRequested, this, &ItemPanel::showContextMenu);
    auto *copy = new QShortcut(QKeySequence::Copy, this);
    copy->setContext(Qt::WidgetWithChildrenShortcut);
    connect(copy, &QShortcut::activated, this, [this] {
        const QList<int> ids = selectedItemIds();
        if (!ids.isEmpty())
            emit copyRequested(ids);
    });
    auto *paste = new QShortcut(QKeySequence::Paste, this);
    paste->setContext(Qt::WidgetWithChildrenShortcut);
    connect(paste, &QShortcut::activated, this, [this] {
        if (m_canPaste && m_drawerId)
            emit pasteRequested();
    });

    clear();
}

void ItemPanel::setCanPaste(bool canPaste)
{
    m_canPaste = canPaste;
}

void ItemPanel::showContextMenu(const QPoint &pos)
{
    const bool hasSelection = m_table->selectionModel()->hasSelection();
    // Right click on a row that is not selected selects it first.
    const QModelIndex under = m_table->indexAt(pos);
    if (under.isValid() && !m_table->selectionModel()->isRowSelected(under.row()))
        m_table->selectRow(under.row());

    QMenu menu;
    const QList<int> ids = selectedItemIds();
    menu.addAction(tr("Copy"), QKeySequence::Copy, this, [this, ids] { emit copyRequested(ids); })
        ->setEnabled(!ids.isEmpty());
    menu.addAction(tr("Paste"), QKeySequence::Paste, this, [this] { emit pasteRequested(); })
        ->setEnabled(m_canPaste && m_drawerId != 0);
    menu.addSeparator();
    menu.addAction(tr("Edit"), this, &ItemPanel::editItem)->setEnabled(!ids.isEmpty());
    menu.addAction(tr("Remove"), this, &ItemPanel::removeItem)->setEnabled(!ids.isEmpty());
    Q_UNUSED(hasSelection)
    menu.exec(m_table->viewport()->mapToGlobal(pos));
}

void ItemPanel::setDrawer(const Drawer &drawer, const QString &groupName,
                          const QList<ItemDialog::DrawerChoice> &destinations)
{
    m_drawerId = drawer.id;
    m_destinations = destinations;
    m_title->setText(tr("Items in drawer %1 (group %2)").arg(drawer.label, groupName));
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
    const QList<int> previouslySelected = selectedItemIds();
    try {
        m_model->setItems(m_items.inDrawer(m_drawerId));
        for (int id : previouslySelected) {
            const int row = m_model->rowOfItem(id);
            if (row >= 0)
                m_table->selectionModel()->select(
                    m_model->index(row, 0),
                    QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
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
    m_decrease->setEnabled(hasSelection);
    m_increase->setEnabled(hasSelection);
}

QList<int> ItemPanel::selectedItemIds() const
{
    QList<int> ids;
    for (const QModelIndex &index : m_table->selectionModel()->selectedRows())
        ids.append(m_model->itemAt(index.row()).id);
    return ids;
}

void ItemPanel::adjustQuantity(int delta)
{
    const QList<int> ids = selectedItemIds();
    if (ids.isEmpty())
        return;
    try {
        m_items.adjustQuantity(ids, delta);
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    reload();
    emit itemsChanged();
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
    const QList<int> ids = selectedItemIds();
    if (ids.isEmpty())
        return;
    const QString what = ids.size() == 1
        ? tr("Remove \"%1\"?").arg(m_model->itemAt(m_table->currentIndex().row()).name)
        : tr("Remove %n selected item(s)?", "", ids.size());
    if (QMessageBox::question(this, tr("Remove item"), what) != QMessageBox::Yes)
        return;
    try {
        for (int id : ids)
            m_items.remove(id);
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    reload();
    emit itemsChanged();
}
