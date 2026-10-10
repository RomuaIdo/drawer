#include "CapacitorCode.h"

#include <QObject>
#include <QRegularExpression>
#include <cmath>

#include "Engineering.h"

namespace CapacitorCode {

namespace {

// B, C, D are absolute (pF); the rest are percent.
std::optional<double> absoluteToleranceOf(QChar letter)
{
    switch (letter.toUpper().unicode()) {
    case 'B': return 0.1;
    case 'C': return 0.25;
    case 'D': return 0.5;
    default: return std::nullopt;
    }
}

} // namespace

std::optional<double> tolerancePercentOf(QChar letter)
{
    switch (letter.toUpper().unicode()) {
    case 'F': return 1;
    case 'G': return 2;
    case 'H': return 3;
    case 'J': return 5;
    case 'K': return 10;
    case 'M': return 20;
    case 'Z': return 80; // +80 / -20
    default: return std::nullopt;
    }
}

std::optional<Capacitance> decode(const QString &marking)
{
    const QString text = marking.trimmed().toUpper();
    static const QRegularExpression pattern(R"(^(\d{2,3}|\dR\d)([A-Z])?$)");
    const auto match = pattern.match(text);
    if (!match.hasMatch())
        return std::nullopt;

    Capacitance result;
    const QString code = match.captured(1);
    if (code.size() == 3 && code[1] == 'R') {
        result.picofarads = QString(code).replace('R', '.').toDouble(); // 4R7 -> 4.7 pF
    } else if (code.size() == 3) {
        const int multiplier = code[2].digitValue();
        if (multiplier == 7) // Unused; 8 and 9 are fractional multipliers.
            return std::nullopt;
        const double factor = multiplier == 8 ? 0.01 : multiplier == 9 ? 0.1
                                                                        : std::pow(10.0, multiplier);
        result.picofarads = code.left(2).toInt() * factor;
    } else {
        result.picofarads = code.toInt(); // Two digits: plain picofarads.
    }

    const QString letter = match.captured(2);
    if (!letter.isEmpty()) {
        result.tolerancePercent = tolerancePercentOf(letter[0]);
        result.tolerancePf = absoluteToleranceOf(letter[0]);
        if (!result.tolerancePercent && !result.tolerancePf)
            return std::nullopt;
    }
    return result;
}

std::optional<QString> encode(double picofarads)
{
    if (picofarads < 0)
        return std::nullopt;
    if (picofarads < 10) {
        // Below 10 pF the value is written directly, e.g. 4R7.
        const double tenths = std::round(picofarads * 10);
        if (std::abs(picofarads * 10 - tenths) > 1e-6)
            return std::nullopt;
        return QString("%1R%2").arg(int(tenths) / 10).arg(int(tenths) % 10);
    }
    for (int exponent = 0; exponent <= 6; ++exponent) {
        const double scaled = picofarads / std::pow(10.0, exponent);
        const double rounded = std::round(scaled);
        if (scaled >= 10 && scaled < 100 && std::abs(scaled - rounded) < 1e-6 * scaled)
            return QString("%1%2").arg(int(rounded)).arg(exponent);
    }
    return std::nullopt;
}

QString name(CapacitorType type)
{
    switch (type) {
    case CapacitorType::CeramicDisc: return QObject::tr("Ceramic disc");
    case CapacitorType::CeramicChip: return QObject::tr("Ceramic chip (SMD)");
    case CapacitorType::Film: return QObject::tr("Film (box)");
    case CapacitorType::Electrolytic: return QObject::tr("Electrolytic");
    case CapacitorType::Tantalum: return QObject::tr("Tantalum (SMD)");
    }
    return {};
}

CapacitorType defaultTypeFor(double picofarads)
{
    constexpr double kEpsilon = 1 + 1e-9; // 100n parsed as 100000.00000000001 pF is still 100 nF.
    if (picofarads <= 100e3 * kEpsilon)
        return CapacitorType::CeramicDisc;
    if (picofarads <= 1e6 * kEpsilon)
        return CapacitorType::Film;
    return CapacitorType::Electrolytic;
}

std::optional<Reading> read(const QString &input)
{
    const QString text = input.trimmed();
    if (text.isEmpty())
        return std::nullopt;

    // A marking first: "100F" is 10 pF with an F tolerance, not 100 farads.
    if (const auto marked = decode(text)) {
        Reading reading{*marked, QString()};
        const QString letter = text.back().isLetter() && text.size() > 1 && text.at(text.size() - 2).isDigit()
            ? QString(text.back().toUpper())
            : QString();
        if (const auto code = encode(marked->picofarads))
            reading.marking = *code + letter;
        return reading;
    }

    // Otherwise a value with an optional prefix and unit.
    const auto farads = Engineering::parse(text);
    if (!farads || *farads <= 0)
        return std::nullopt;
    Reading reading;
    reading.capacitance.picofarads = *farads * 1e12;
    if (const auto code = encode(reading.capacitance.picofarads))
        reading.marking = *code;
    return reading;
}

std::optional<double> parseVoltage(const QString &text)
{
    QString t = text.trimmed();
    if (t.endsWith('V') || t.endsWith('v'))
        t.chop(1);
    const auto volts = Engineering::parse(t);
    if (!volts || *volts <= 0)
        return std::nullopt;
    return volts;
}

QString formatVoltage(double volts)
{
    // Whole volts without decimals, thousands as kV.
    if (volts >= 1000)
        return QString::number(volts / 1000, 'g', 3) + "kV";
    return QString::number(volts, 'g', 3) + "V";
}

QString compactValue(double picofarads, bool withUnit)
{
    // Micro goes up to 10 mF so that large electrolytics read 1000u, 4700u as printed.
    double divisor = 1;
    QString symbol = "p";
    if (picofarads >= 1e10) {
        divisor = 1e9;
        symbol = "m";
    } else if (picofarads >= 1e6) {
        divisor = 1e6;
        symbol = QString::fromUtf8("\u00b5");
    } else if (picofarads >= 1e3) {
        divisor = 1e3;
        symbol = "n";
    }
    return QString::number(picofarads / divisor, 'g', 4) + symbol + (withUnit ? "F" : "");
}

QStringList printedLines(CapacitorType type, const Reading &reading, std::optional<double> volts)
{
    const double pf = reading.capacitance.picofarads;
    QStringList lines;
    switch (type) {
    case CapacitorType::CeramicChip:
        return {}; // Chips are not marked.
    case CapacitorType::CeramicDisc:
        lines << (reading.marking.isEmpty() ? compactValue(pf, false) : reading.marking);
        break;
    case CapacitorType::Film:
        lines << compactValue(pf, false);
        break;
    case CapacitorType::Electrolytic:
        lines << compactValue(pf, true);
        break;
    case CapacitorType::Tantalum:
        lines << compactValue(pf, false);
        break;
    }
    if (volts)
        lines << formatVoltage(*volts);
    return lines;
}

double voltageScale(std::optional<double> volts)
{
    if (!volts)
        return 1.0;
    if (*volts <= 10)
        return 0.85;
    if (*volts <= 25)
        return 1.0;
    if (*volts <= 63)
        return 1.12;
    if (*volts <= 160)
        return 1.22;
    return 1.3;
}

} // namespace CapacitorCode
