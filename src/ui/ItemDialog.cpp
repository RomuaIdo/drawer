#include "ItemDialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

#include "ItemCompleter.h"

ItemDialog::ItemDialog(const Item &item, const DrawerLayouts &drawers, CabinetView *cabinet,
                       QWidget *parent)
    : ItemFormDialog(item, parent)
    , m_layouts(drawers)
    , m_drawerText(new QLabel)
    , m_quantity(new QSpinBox)
{
    setWindowTitle(item.id ? tr("Edit item") : tr("New item"));
    m_quantity->setRange(0, 1000000);
    m_quantity->setValue(item.quantity);
    buildForm({tr("Drawer"), m_drawerText}, {tr("Quantity"), m_quantity});
    if (cabinet)
        connect(cabinet, &CabinetView::drawerSelected, this,
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
                text = tr("%1 / %2").arg(l.group.name, d.label);
            }
        }
    }
    m_drawerText->setText(text);
    updateOk();
}

void ItemDialog::setSuggestions(const QList<Item> &items)
{
    auto *completer = new ItemCompleter(this);
    completer->setItems(items);
    m_name->setCompleter(completer);
    connect(completer, &ItemCompleter::itemChosen, this,
            [this](const Item &chosen) { fillDetails(chosen); });
}

void ItemDialog::setRestockMode()
{
    setWindowTitle(tr("Add item to the restock list"));
    m_quantity->setMinimum(1);
    if (m_quantity->value() < 1)
        m_quantity->setValue(1);
    if (auto *label = qobject_cast<QLabel *>(form()->labelForField(m_quantity)))
        label->setText(tr("Quantity to add"));
}

Item ItemDialog::item() const
{
    Item result = ItemFormDialog::item();
    result.drawerId = m_drawerId;
    result.quantity = m_quantity->value();
    return result;
}
