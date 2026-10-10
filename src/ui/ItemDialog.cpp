#include "ItemDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

ItemDialog::ItemDialog(const Item &item, const QList<DrawerChoice> &drawers, QWidget *parent)
    : QDialog(parent)
    , m_item(item)
    , m_drawer(new QComboBox)
    , m_name(new QLineEdit(item.name))
    , m_category(new QLineEdit(item.category))
    , m_value(new QLineEdit(item.value))
    , m_package(new QLineEdit(item.package))
    , m_quantity(new QSpinBox)
    , m_alert(new QCheckBox(tr("Alert at or below")))
    , m_minStock(new QSpinBox)
    , m_notes(new QLineEdit(item.notes))
{
    setWindowTitle(item.id ? tr("Edit item") : tr("New item"));

    for (const DrawerChoice &d : drawers)
        m_drawer->addItem(d.text, d.id);
    m_drawer->setCurrentIndex(m_drawer->findData(item.drawerId));
    m_quantity->setRange(0, 1000000);
    m_quantity->setValue(item.quantity);
    m_minStock->setRange(0, 1000000);
    m_minStock->setValue(item.minStock.value_or(0));
    m_alert->setChecked(item.minStock.has_value());
    m_minStock->setEnabled(item.minStock.has_value());
    connect(m_alert, &QCheckBox::toggled, m_minStock, &QWidget::setEnabled);
    m_alert->setToolTip(tr("Applies to the item as a whole: its total over every drawer of the cabinet."));
    m_value->setPlaceholderText(tr("e.g. 4.7k, 100nF"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto *form = new QFormLayout(this);
    form->addRow(tr("Drawer"), m_drawer);
    form->addRow(tr("Name"), m_name);
    form->addRow(tr("Category"), m_category);
    form->addRow(tr("Value"), m_value);
    form->addRow(tr("Package"), m_package);
    form->addRow(tr("Quantity"), m_quantity);
    auto *minimum = new QHBoxLayout;
    minimum->addWidget(m_alert);
    minimum->addWidget(m_minStock, 1);
    form->addRow(tr("Minimum stock"), minimum);
    form->addRow(tr("Notes"), m_notes);
    form->addRow(buttons);

    // Name is required.
    auto *ok = buttons->button(QDialogButtonBox::Ok);
    ok->setEnabled(!item.name.trimmed().isEmpty());
    connect(m_name, &QLineEdit::textChanged, this,
            [ok](const QString &text) { ok->setEnabled(!text.trimmed().isEmpty()); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

Item ItemDialog::item() const
{
    Item result = m_item;
    result.drawerId = m_drawer->currentData().toInt();
    result.name = m_name->text().trimmed();
    result.category = m_category->text().trimmed();
    result.value = m_value->text().trimmed();
    result.package = m_package->text().trimmed();
    result.quantity = m_quantity->value();
    result.minStock = m_alert->isChecked() ? std::optional<int>(m_minStock->value()) : std::nullopt;
    result.notes = m_notes->text().trimmed();
    return result;
}
