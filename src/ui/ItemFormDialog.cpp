#include "ItemFormDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>

ItemFormDialog::ItemFormDialog(const Item &item, QWidget *parent)
    : QDialog(parent)
    , m_item(item)
    , m_name(new QLineEdit(item.name))
    , m_category(new QLineEdit(item.category))
    , m_value(new QLineEdit(item.value))
    , m_package(new QLineEdit(item.package))
    , m_alert(new QCheckBox(tr("Alert at or below")))
    , m_minStock(new QSpinBox)
    , m_notes(new QLineEdit(item.notes))
{
    m_minStock->setRange(0, 1000000);
    m_minStock->setValue(item.minStock.value_or(0));
    m_alert->setChecked(item.minStock.has_value());
    m_minStock->setEnabled(item.minStock.has_value());
    connect(m_alert, &QCheckBox::toggled, m_minStock, &QWidget::setEnabled);
    m_alert->setToolTip(tr("Applies to the item as a whole: its total over every drawer of the cabinet."));
    m_value->setPlaceholderText(tr("e.g. 4.7k, 100nF"));
}

void ItemFormDialog::buildForm(const Row &lead, const Row &extra)
{
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_ok = buttons->button(QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_name, &QLineEdit::textChanged, this, &ItemFormDialog::updateOk);

    m_form = new QFormLayout(this);
    if (lead.widget)
        m_form->addRow(lead.label, lead.widget);
    m_form->addRow(tr("Name"), m_name);
    m_form->addRow(tr("Category"), m_category);
    m_form->addRow(tr("Value"), m_value);
    m_form->addRow(tr("Package"), m_package);
    if (extra.widget)
        m_form->addRow(extra.label, extra.widget);
    auto *minimum = new QHBoxLayout;
    minimum->addWidget(m_alert);
    minimum->addWidget(m_minStock, 1);
    m_form->addRow(tr("Minimum stock"), minimum);
    m_form->addRow(tr("Notes"), m_notes);
    m_form->addRow(buttons);
    updateOk();
}

void ItemFormDialog::updateOk()
{
    m_ok->setEnabled(!m_name->text().trimmed().isEmpty() && isComplete());
}

void ItemFormDialog::fillDetails(const Item &source)
{
    m_category->setText(source.category);
    m_value->setText(source.value);
    m_package->setText(source.package);
    m_alert->setChecked(source.minStock.has_value());
    m_minStock->setValue(source.minStock.value_or(0));
    m_notes->setText(source.notes);
}

Item ItemFormDialog::item() const
{
    Item result = m_item;
    result.name = m_name->text().trimmed();
    result.category = m_category->text().trimmed();
    result.value = m_value->text().trimmed();
    result.package = m_package->text().trimmed();
    result.minStock = m_alert->isChecked() ? std::optional<int>(m_minStock->value()) : std::nullopt;
    result.notes = m_notes->text().trimmed();
    return result;
}

bool ItemFormDialog::run()
{
    static QPointer<ItemFormDialog> open;
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
