#include "CapacitorWidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include "CapacitorCode.h"
#include "CapacitorStrip.h"
#include "Engineering.h"
#include "ResultLabel.h"

using namespace CapacitorCode;

namespace {
constexpr int kAutomatic = -1;
}

CapacitorWidget::CapacitorWidget(QWidget *parent)
    : QWidget(parent)
    , m_strip(new CapacitorStrip)
    , m_input(new QLineEdit)
    , m_voltage(new QLineEdit)
    , m_type(new QComboBox)
    , m_result(makeResultLabel())
{
    m_input->setPlaceholderText(tr("e.g. 104, 223K, 4R7, 100n, 4u7"));
    m_voltage->setPlaceholderText(tr("optional, e.g. 50, 6.3V, 1kV"));
    m_type->addItem(tr("Automatic"), kAutomatic);
    for (CapacitorType type : {CapacitorType::CeramicDisc, CapacitorType::CeramicChip,
                               CapacitorType::Film, CapacitorType::Electrolytic,
                               CapacitorType::Tantalum})
        m_type->addItem(name(type), int(type));
    m_type->setToolTip(tr("Automatic picks the usual kind of capacitor for the value"));

    auto *form = new QFormLayout;
    form->addRow(tr("Marking or value"), m_input);
    form->addRow(tr("Voltage"), m_voltage);
    form->addRow(tr("Type"), m_type);
    form->addRow(m_result);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_strip);
    layout->addLayout(form);
    layout->addStretch();

    connect(m_input, &QLineEdit::textChanged, this, &CapacitorWidget::refresh);
    connect(m_voltage, &QLineEdit::textChanged, this, &CapacitorWidget::refresh);
    connect(m_type, &QComboBox::currentIndexChanged, this, &CapacitorWidget::refresh);
    refresh();
}

void CapacitorWidget::refresh()
{
    const QString text = m_input->text().trimmed();
    const auto reading = read(text);
    const bool automatic = m_type->currentData().toInt() == kAutomatic;

    // The drawing follows the value unless the type was fixed by hand.
    CapacitorType type = CapacitorType::CeramicDisc;
    if (!automatic)
        type = static_cast<CapacitorType>(m_type->currentData().toInt());
    else if (reading)
        type = defaultTypeFor(reading->capacitance.picofarads);
    m_strip->setType(type);

    // The voltage is optional; an unreadable one is reported but does not stop the rest.
    const QString voltageText = m_voltage->text().trimmed();
    const auto volts = parseVoltage(voltageText);
    m_strip->setScale(voltageScale(volts));
    m_strip->setPrinted(reading ? printedLines(type, *reading, volts) : QStringList());

    if (text.isEmpty()) {
        m_result->clear();
        return;
    }
    if (!reading) {
        m_result->setText(tr("Not a valid marking or value"));
        return;
    }

    const Capacitance &c = reading->capacitance;
    QString tolerance = tr("not given");
    if (c.tolerancePercent)
        tolerance = QString("±%1%").arg(*c.tolerancePercent);
    else if (c.tolerancePf)
        tolerance = QString("±%1 pF").arg(*c.tolerancePf);

    m_result->setText(
        tr("Value: %1\nTolerance: %2\nMarking: %3\nVoltage: %4\nDrawn as: %5%6")
            .arg(Engineering::format(c.farads(), "F"), tolerance,
                 reading->marking.isEmpty() ? tr("no 3-digit code") : reading->marking,
                 !volts ? (voltageText.isEmpty() ? tr("not given") : tr("not valid"))
                        : formatVoltage(*volts),
                 name(type), automatic ? tr(" (automatic)") : QString()));
}
