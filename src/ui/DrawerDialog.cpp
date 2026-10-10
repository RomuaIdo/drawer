#include "DrawerDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>

#include "SizeComboBox.h"

DrawerDialog::DrawerDialog(const Drawer &drawer, QWidget *parent)
    : QDialog(parent)
    , m_drawer(drawer)
    , m_label(new QLineEdit(drawer.label))
    , m_size(new SizeComboBox(drawer.size))
{
    setWindowTitle(tr("Edit drawer"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto *form = new QFormLayout(this);
    form->addRow(tr("Label"), m_label);
    form->addRow(tr("Size"), m_size);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

Drawer DrawerDialog::drawer() const
{
    Drawer result = m_drawer;
    result.label = m_label->text().trimmed();
    result.size = m_size->size();
    return result;
}
