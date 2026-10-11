#include "ItemDialog.h"

#include <QCheckBox>
#include <QEventLoop>
#include <QPointer>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

ItemDialog::ItemDialog(const Item &item, const DrawerLayouts &drawers, CabinetView *cabinet,
                       QWidget *parent)
    : QDialog(parent)
    , m_item(item)
    , m_cabinet(cabinet)
    , m_drawerText(new QLabel)
    , m_layouts(drawers)
    , m_quantity(new QSpinBox)
{
    m_quantity->setRange(0, 1000000);
    m_quantity->setValue(item.quantity);
    setup(item);
    if (m_cabinet)
        connect(m_cabinet, &CabinetView::drawerSelected, this,
                [this](const Drawer &drawer) { chooseDrawer(drawer.id); });
    chooseDrawer(item.drawerId);
}

void ItemDialog::chooseDrawer(int drawerId)
{
    m_drawerId = 0;
    QString text = tr("Click a drawer in the cabinet");
    for (const CabinetView::GroupLayout &l : m_layouts) {
        for (const Drawer &d : l.drawers) {
            if (d.id == drawerId) {
                m_drawerId = d.id;
                text = tr("Drawer: %1 / %2").arg(l.group.name, d.label);
            }
        }
    }
    m_drawerText->setText(text);
    updateOk();
}

void ItemDialog::updateOk()
{
    // A name is required, and the drawer when it is being chosen.
    m_ok->setEnabled(!m_name->text().trimmed().isEmpty() && (!m_quantity || m_drawerId != 0));
}

void ItemDialog::setRestockMode()
{
    setWindowTitle(tr("Add item to the restock list"));
    if (m_quantity) {
        m_quantity->setMinimum(1);
        if (m_quantity->value() < 1)
            m_quantity->setValue(1);
        if (auto *form = qobject_cast<QFormLayout *>(layout()))
            if (auto *label = qobject_cast<QLabel *>(form->labelForField(m_quantity)))
                label->setText(tr("Quantity to add"));
    }
}

ItemDialog::ItemDialog(const Item &item, const QList<ItemLocation> &occurrences, QWidget *parent)
    : QDialog(parent)
    , m_item(item)
{
    setup(item);
    // Replace the single quantity by the sub-table, right under the common fields.
    auto *form = static_cast<QFormLayout *>(layout());
    form->insertRow(form->rowCount() - 1, tr("Quantities"), buildOccurrences(occurrences));
}

void ItemDialog::setup(const Item &item)
{
    const bool everyEntry = m_quantity == nullptr;
    setWindowTitle(everyEntry ? tr("Edit item in every drawer")
                              : (item.id ? tr("Edit item") : tr("New item")));

    m_name = new QLineEdit(item.name);
    m_category = new QLineEdit(item.category);
    m_value = new QLineEdit(item.value);
    m_package = new QLineEdit(item.package);
    m_alert = new QCheckBox(tr("Alert at or below"));
    m_minStock = new QSpinBox;
    m_notes = new QLineEdit(item.notes);

    m_minStock->setRange(0, 1000000);
    m_minStock->setValue(item.minStock.value_or(0));
    m_alert->setChecked(item.minStock.has_value());
    m_minStock->setEnabled(item.minStock.has_value());
    connect(m_alert, &QCheckBox::toggled, m_minStock, &QWidget::setEnabled);
    m_alert->setToolTip(tr("Applies to the item as a whole: its total over every drawer of the cabinet."));
    m_value->setPlaceholderText(tr("e.g. 4.7k, 100nF"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto *form = new QFormLayout(this);
    if (m_quantity)
        form->addRow(tr("Drawer"), m_drawerText);
    form->addRow(tr("Name"), m_name);
    form->addRow(tr("Category"), m_category);
    form->addRow(tr("Value"), m_value);
    form->addRow(tr("Package"), m_package);
    if (m_quantity)
        form->addRow(tr("Quantity"), m_quantity);
    auto *minimum = new QHBoxLayout;
    minimum->addWidget(m_alert);
    minimum->addWidget(m_minStock, 1);
    form->addRow(tr("Minimum stock"), minimum);
    form->addRow(tr("Notes"), m_notes);
    form->addRow(buttons);

    // Name is required.
    m_ok = buttons->button(QDialogButtonBox::Ok);
    updateOk();
    connect(m_name, &QLineEdit::textChanged, this, &ItemDialog::updateOk);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QWidget *ItemDialog::buildOccurrences(const QList<ItemLocation> &occurrences)
{
    m_occurrences = new QTableWidget(occurrences.size(), 2);
    m_occurrences->setHorizontalHeaderLabels({tr("Drawer"), tr("Quantity")});
    m_occurrences->verticalHeader()->hide();
    m_occurrences->setSelectionMode(QAbstractItemView::NoSelection);
    m_occurrences->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_occurrences->horizontalHeader()->setStretchLastSection(true);
    m_occurrences->setMinimumHeight(30 + 28 * qMin(occurrences.size(), 6));

    for (int row = 0; row < occurrences.size(); ++row) {
        const ItemLocation &location = occurrences[row];
        m_occurrenceIds.append(location.itemId);
        m_occurrences->setItem(
            row, 0, new QTableWidgetItem(QString("%1/%2").arg(location.group, location.drawerLabel)));
        auto *spin = new QSpinBox;
        spin->setRange(0, 1000000);
        spin->setValue(location.quantity);
        m_occurrences->setCellWidget(row, 1, spin);
    }
    return m_occurrences;
}

Item ItemDialog::item() const
{
    Item result = m_item;
    if (m_quantity) {
        result.drawerId = m_drawerId;
        result.quantity = m_quantity->value();
    }
    result.name = m_name->text().trimmed();
    result.category = m_category->text().trimmed();
    result.value = m_value->text().trimmed();
    result.package = m_package->text().trimmed();
    result.minStock = m_alert->isChecked() ? std::optional<int>(m_minStock->value()) : std::nullopt;
    result.notes = m_notes->text().trimmed();
    return result;
}

QList<QPair<int, int>> ItemDialog::quantities() const
{
    QList<QPair<int, int>> result;
    for (int row = 0; row < m_occurrenceIds.size(); ++row) {
        const auto *spin = qobject_cast<QSpinBox *>(m_occurrences->cellWidget(row, 1));
        result.append({m_occurrenceIds[row], spin->value()});
    }
    return result;
}

bool ItemDialog::run()
{
    static QPointer<ItemDialog> open;
    if (open) {
        open->raise();
        open->activateWindow();
        return false;
    }
    open = this;
    setModal(false);
    // Sit over the item dock, leaving the cabinet on the left uncovered.
    if (QWidget *main = parentWidget() ? parentWidget()->window() : nullptr) {
        adjustSize();
        move(main->geometry().right() - width() - 20, main->geometry().top() + 80);
    }
    QEventLoop loop;
    connect(this, &QDialog::finished, &loop, &QEventLoop::quit);
    show();
    loop.exec();
    return result() == QDialog::Accepted;
}
