#include "FormulaWidgets.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

#include "Engineering.h"
#include "Formulas.h"
#include "ResultLabel.h"
#include "SmdCode.h"

namespace {

QLineEdit *makeInput(const QString &placeholder)
{
    auto *edit = new QLineEdit;
    edit->setPlaceholderText(placeholder);
    return edit;
}

// Empty text -> nullopt; text that is not a number -> `invalid` set to true.
std::optional<double> parseField(const QLineEdit *edit, bool &invalid)
{
    const QString text = edit->text().trimmed();
    if (text.isEmpty())
        return std::nullopt;
    const auto value = Engineering::parse(text);
    if (!value)
        invalid = true;
    return value;
}

} // namespace

OhmWidget::OhmWidget(QWidget *parent)
    : QWidget(parent)
    , m_volts(makeInput("V"))
    , m_amps(makeInput("A"))
    , m_ohms(makeInput(QString::fromUtf8("Ω")))
    , m_watts(makeInput("W"))
    , m_result(makeResultLabel())
{
    auto *form = new QFormLayout(this);
    form->addRow(tr("Fill any two fields"), new QLabel);
    form->addRow(tr("Voltage"), m_volts);
    form->addRow(tr("Current"), m_amps);
    form->addRow(tr("Resistance"), m_ohms);
    form->addRow(tr("Power"), m_watts);
    form->addRow(m_result);

    for (QLineEdit *edit : {m_volts, m_amps, m_ohms, m_watts})
        connect(edit, &QLineEdit::textChanged, this, &OhmWidget::recalculate);
}

void OhmWidget::recalculate()
{
    bool invalid = false;
    const auto v = parseField(m_volts, invalid);
    const auto i = parseField(m_amps, invalid);
    const auto r = parseField(m_ohms, invalid);
    const auto p = parseField(m_watts, invalid);
    if (invalid) {
        m_result->setText(tr("Not a valid value"));
        return;
    }
    const auto solved = Formulas::solveOhm(v, i, r, p);
    if (!solved) {
        m_result->setText(tr("Need two valid values"));
        return;
    }
    m_result->setText(QString("%1\n%2\n%3\n%4")
                          .arg(Engineering::format(solved->volts, "V"),
                               Engineering::format(solved->amps, "A"),
                               Engineering::format(solved->ohms, QString::fromUtf8("Ω")),
                               Engineering::format(solved->watts, "W")));
}

DividerWidget::DividerWidget(QWidget *parent)
    : QWidget(parent)
    , m_vin(makeInput("V"))
    , m_r1(makeInput(QString::fromUtf8("Ω (top)")))
    , m_r2(makeInput(QString::fromUtf8("Ω (bottom)")))
    , m_result(makeResultLabel())
{
    auto *form = new QFormLayout(this);
    form->addRow(tr("Input voltage"), m_vin);
    form->addRow(tr("R1"), m_r1);
    form->addRow(tr("R2"), m_r2);
    form->addRow(m_result);

    for (QLineEdit *edit : {m_vin, m_r1, m_r2})
        connect(edit, &QLineEdit::textChanged, this, &DividerWidget::recalculate);
}

void DividerWidget::recalculate()
{
    bool invalid = false;
    const auto vin = parseField(m_vin, invalid);
    const auto r1 = parseField(m_r1, invalid);
    const auto r2 = parseField(m_r2, invalid);
    if (invalid || !vin || !r1 || !r2) {
        m_result->setText(invalid ? tr("Not a valid value") : QString());
        return;
    }
    const auto out = Formulas::dividerOutput(*vin, *r1, *r2);
    m_result->setText(out ? tr("Vout = %1").arg(Engineering::format(*out, "V"))
                          : tr("R1 + R2 must not be zero"));
}

LedWidget::LedWidget(QWidget *parent)
    : QWidget(parent)
    , m_supply(makeInput("V"))
    , m_forward(makeInput("V"))
    , m_current(makeInput("e.g. 20m"))
    , m_result(makeResultLabel())
{
    auto *form = new QFormLayout(this);
    form->addRow(tr("Supply voltage"), m_supply);
    form->addRow(tr("LED forward voltage"), m_forward);
    form->addRow(tr("LED current (A)"), m_current);
    form->addRow(m_result);

    for (QLineEdit *edit : {m_supply, m_forward, m_current})
        connect(edit, &QLineEdit::textChanged, this, &LedWidget::recalculate);
}

void LedWidget::recalculate()
{
    bool invalid = false;
    const auto supply = parseField(m_supply, invalid);
    const auto forward = parseField(m_forward, invalid);
    const auto current = parseField(m_current, invalid);
    if (invalid || !supply || !forward || !current) {
        m_result->setText(invalid ? tr("Not a valid value") : QString());
        return;
    }
    const auto led = Formulas::ledResistor(*supply, *forward, *current);
    if (!led) {
        m_result->setText(tr("Supply must exceed the forward voltage, current must be positive"));
        return;
    }
    const auto standard = Formulas::nearestStandard(led->ohms, 24);
    const QString ohm = QString::fromUtf8("Ω");
    m_result->setText(tr("Resistor: %1\nNext E24 value: %2\nPower: %3")
                          .arg(Engineering::format(led->ohms, ohm),
                               standard ? Engineering::format(*standard, ohm) : QString("-"),
                               Engineering::format(led->watts, "W")));
}

SmdWidget::SmdWidget(QWidget *parent)
    : QWidget(parent)
    , m_marking(makeInput("e.g. 472, 1002, 4R7"))
    , m_result(makeResultLabel())
{
    auto *form = new QFormLayout(this);
    form->addRow(tr("SMD marking"), m_marking);
    form->addRow(m_result);
    connect(m_marking, &QLineEdit::textChanged, this, &SmdWidget::recalculate);
}

void SmdWidget::recalculate()
{
    const QString text = m_marking->text().trimmed();
    if (text.isEmpty()) {
        m_result->clear();
        return;
    }
    const auto ohms = SmdCode::decode(text);
    m_result->setText(ohms ? Engineering::format(*ohms, QString::fromUtf8("Ω"))
                           : tr("Invalid marking"));
}
