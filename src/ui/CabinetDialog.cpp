#include "CabinetDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

CabinetDialog::CabinetDialog(const Cabinet &cabinet, QWidget *parent)
    : QDialog(parent)
    , m_cabinet(cabinet)
    , m_name(new QLineEdit(cabinet.name))
    , m_description(new QPlainTextEdit(cabinet.description))
{
    setWindowTitle(cabinet.id ? tr("Edit cabinet") : tr("New cabinet"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto *form = new QFormLayout(this);
    form->addRow(tr("Name"), m_name);
    form->addRow(tr("Description"), m_description);
    form->addRow(buttons);

    // Name is required.
    auto *ok = buttons->button(QDialogButtonBox::Ok);
    ok->setEnabled(!cabinet.name.trimmed().isEmpty());
    connect(m_name, &QLineEdit::textChanged, this,
            [ok](const QString &text) { ok->setEnabled(!text.trimmed().isEmpty()); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

Cabinet CabinetDialog::cabinet() const
{
    Cabinet result = m_cabinet;
    result.name = m_name->text().trimmed();
    result.description = m_description->toPlainText().trimmed();
    return result;
}
