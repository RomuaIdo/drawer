#include "GroupDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>

#include "SizeComboBox.h"

namespace {

QSpinBox *makeSpin(int min, int max, int value)
{
    auto *spin = new QSpinBox;
    spin->setRange(min, max);
    spin->setValue(value);
    return spin;
}

} // namespace

GroupDialog::GroupDialog(const DrawerGroup &group, PlacementCheck isFree, QWidget *parent)
    : QDialog(parent)
    , m_group(group)
    , m_isFree(std::move(isFree))
    , m_name(new QLineEdit(group.name))
    , m_rows(makeSpin(1, 20, group.rows))
    , m_cols(makeSpin(1, 20, group.cols))
    , m_posX(makeSpin(0, 99, group.posX))
    , m_posY(makeSpin(0, 99, group.posY))
    , m_size(new SizeComboBox(group.defaultSize))
{
    const bool editing = group.id != 0;
    setWindowTitle(editing ? tr("Edit drawer group") : tr("New drawer group"));

    m_rows->setEnabled(!editing);
    m_cols->setEnabled(!editing);
    m_size->setToolTip(tr("Size applied to every drawer created in this group"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto *form = new QFormLayout(this);
    form->addRow(tr("Name"), m_name);
    form->addRow(tr("Rows"), m_rows);
    form->addRow(tr("Columns"), m_cols);
    form->addRow(tr("Position X"), m_posX);
    form->addRow(tr("Position Y"), m_posY);
    form->addRow(tr("Default size"), m_size);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &GroupDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

DrawerGroup GroupDialog::group() const
{
    DrawerGroup result = m_group;
    result.name = m_name->text().trimmed();
    result.rows = m_rows->value();
    result.cols = m_cols->value();
    result.posX = m_posX->value();
    result.posY = m_posY->value();
    result.defaultSize = m_size->size();
    return result;
}

void GroupDialog::accept()
{
    if (m_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Name is required."));
        return;
    }
    if (!m_isFree(group())) {
        QMessageBox::warning(this, windowTitle(), tr("The group overlaps another group."));
        return;
    }
    QDialog::accept();
}
