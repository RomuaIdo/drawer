#include "ListPanel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>

#include "ListModel.h"
#include "data/DatabaseError.h"

namespace {

// Quantity editor: a spin box that allows real stock quantities (the default stops at 99).
class QuantityDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                          const QModelIndex &) const override
    {
        auto *spin = new QSpinBox(parent);
        spin->setRange(1, 1000000);
        return spin;
    }
    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        static_cast<QSpinBox *>(editor)->setValue(index.data(Qt::EditRole).toInt());
    }
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override
    {
        model->setData(index, static_cast<QSpinBox *>(editor)->value(), Qt::EditRole);
    }
};

} // namespace

ListPanel::ListPanel(ListKind kind, IListRepository &lists, QWidget *parent)
    : QWidget(parent)
    , m_kind(kind)
    , m_lists(lists)
    , m_model(new ListModel(kind, this))
    , m_table(new QTableView)
    , m_summary(new QLabel)
    , m_remove(new QPushButton(tr("Remove line")))
    , m_clear(new QPushButton(tr("Clear")))
    , m_apply(new QPushButton(kind == ListKind::Pick ? tr("Take all from stock")
                                                     : tr("Restock all")))
{
    m_table->setModel(m_model);
    m_table->setItemDelegateForColumn(ListModel::Quantity, new QuantityDelegate(m_table));
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();

    m_remove->setToolTip(tr("Take the selected lines off the list"));
    m_apply->setToolTip(kind == ListKind::Pick
                            ? tr("Subtract every quantity from the stock and empty the list")
                            : tr("Add every quantity to the stock and empty the list"));
    m_summary->setWordWrap(true);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_remove);
    buttons->addWidget(m_clear);
    buttons->addStretch();
    buttons->addWidget(m_apply);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_summary);
    layout->addWidget(m_table);
    layout->addLayout(buttons);

    connect(m_model, &ListModel::quantityEdited, this, [this](int lineId, int quantity) {
        try {
            m_lists.setQuantity(lineId, quantity);
        } catch (const DatabaseError &e) {
            QMessageBox::critical(this, tr("Database error"), e.what());
            return;
        }
        reload();
        emit listChanged(m_kind == ListKind::Pick ? tr("Change pick list quantity")
                                                  : tr("Change restock list quantity"));
    });
    connect(m_remove, &QPushButton::clicked, this, &ListPanel::removeSelected);
    connect(m_clear, &QPushButton::clicked, this, &ListPanel::clearAll);
    connect(m_apply, &QPushButton::clicked, this, &ListPanel::applyToStock);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        m_remove->setEnabled(m_table->selectionModel()->hasSelection());
        emit drawersChosen(selectedDrawerIds());
    });
    reload();
}

int ListPanel::lineCount() const
{
    return m_model->rowCount();
}

void ListPanel::reload()
{
    m_table->clearSelection();
    try {
        m_model->setLines(m_lists.lines(m_kind));
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
    }
    m_remove->setEnabled(false);
    updateSummary();
}

void ListPanel::updateSummary()
{
    const int lines = m_model->rowCount();
    int short_ = 0;
    for (int row = 0; row < lines; ++row)
        short_ += m_model->lineAt(row).isShort() ? 1 : 0;

    QString text = lines == 0 ? (m_kind == ListKind::Pick
                                     ? tr("The list is empty. Right click items and choose Add to pick list.")
                                     : tr("The list is empty. Right click items and choose Add to restock list."))
                              : tr("%n line(s)", "", lines);
    if (short_ > 0)
        text += tr(" - %n line(s) ask for more than is in stock", "", short_);
    m_summary->setText(text);
    m_clear->setEnabled(lines > 0);
    m_apply->setEnabled(lines > 0);
}

QList<int> ListPanel::selectedDrawerIds() const
{
    QList<int> ids;
    for (const QModelIndex &index : m_table->selectionModel()->selectedRows())
        ids.append(m_model->lineAt(index.row()).item.drawerId);
    return ids;
}

void ListPanel::clearSelection()
{
    m_table->clearSelection();
}

void ListPanel::removeSelected()
{
    QList<int> ids;
    for (const QModelIndex &index : m_table->selectionModel()->selectedRows())
        ids.append(m_model->lineAt(index.row()).id);
    if (ids.isEmpty())
        return;
    try {
        m_lists.remove(ids);
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    reload();
    emit listChanged(tr("Remove list line(s)"));
}

void ListPanel::clearAll()
{
    if (m_model->rowCount() == 0)
        return;
    try {
        m_lists.clear(m_kind);
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    reload();
    emit listChanged(tr("Clear list"));
}

void ListPanel::applyToStock()
{
    const int lines = m_model->rowCount();
    if (lines == 0)
        return;

    int short_ = 0;
    int total = 0;
    for (int row = 0; row < lines; ++row) {
        short_ += m_model->lineAt(row).isShort() ? 1 : 0;
        total += m_model->lineAt(row).quantity;
    }

    QString question = m_kind == ListKind::Pick
        ? tr("Take %n piece(s) in %1 line(s) out of stock?", "", total).arg(lines)
        : tr("Add %n piece(s) in %1 line(s) to the stock?", "", total).arg(lines);
    if (short_ > 0)
        question += "\n\n" + tr("%n line(s) ask for more than is in stock; those drawers will be "
                                "emptied.", "", short_);
    if (QMessageBox::question(this, m_apply->text(), question) != QMessageBox::Yes)
        return;

    try {
        m_lists.apply(m_kind);
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), e.what());
        return;
    }
    reload();
    emit applied(m_kind == ListKind::Pick ? tr("Take pick list from stock")
                                          : tr("Restock from list"));
}
