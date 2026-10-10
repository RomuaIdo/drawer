#include "CapacitorWidget.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include "CapacitorCode.h"
#include "Engineering.h"

namespace {

QLabel *makeResultLabel()
{
    auto *label = new QLabel;
    label->setStyleSheet("font-size: 18px; font-weight: bold;");
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

} // namespace

CapacitorWidget::CapacitorWidget(QWidget *parent)
    : QWidget(parent)
    , m_markingInput(new QLineEdit)
    , m_decoded(makeResultLabel())
    , m_valueInput(new QLineEdit)
    , m_encoded(makeResultLabel())
{
    m_markingInput->setPlaceholderText(tr("e.g. 104, 223K, 4R7"));
    m_valueInput->setPlaceholderText(tr("e.g. 100nF, 4.7u, 22p"));

    auto *decodeBox = new QGroupBox(tr("Marking to value"));
    auto *decodeForm = new QFormLayout(decodeBox);
    decodeForm->addRow(tr("Marking"), m_markingInput);
    decodeForm->addRow(m_decoded);

    auto *encodeBox = new QGroupBox(tr("Value to marking"));
    auto *encodeForm = new QFormLayout(encodeBox);
    encodeForm->addRow(tr("Capacitance"), m_valueInput);
    encodeForm->addRow(m_encoded);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(decodeBox);
    layout->addWidget(encodeBox);
    layout->addStretch();

    connect(m_markingInput, &QLineEdit::textChanged, this, &CapacitorWidget::decodeMarking);
    connect(m_valueInput, &QLineEdit::textChanged, this, &CapacitorWidget::encodeValue);
}

void CapacitorWidget::decodeMarking()
{
    const QString text = m_markingInput->text().trimmed();
    if (text.isEmpty()) {
        m_decoded->clear();
        return;
    }
    const auto value = CapacitorCode::decode(text);
    if (!value) {
        m_decoded->setText(tr("Invalid marking"));
        return;
    }
    QString result = Engineering::format(value->farads(), "F");
    if (value->tolerancePercent)
        result += QString("  ±%1%").arg(*value->tolerancePercent);
    else if (value->tolerancePf)
        result += QString("  ±%1 pF").arg(*value->tolerancePf);
    m_decoded->setText(result);
}

void CapacitorWidget::encodeValue()
{
    const QString text = m_valueInput->text().trimmed();
    if (text.isEmpty()) {
        m_encoded->clear();
        return;
    }
    const auto farads = Engineering::parse(text);
    if (!farads) {
        m_encoded->setText(tr("Not a valid value"));
        return;
    }
    const auto code = CapacitorCode::encode(*farads * 1e12);
    m_encoded->setText(code ? *code : tr("No 3-digit code for this value"));
}
