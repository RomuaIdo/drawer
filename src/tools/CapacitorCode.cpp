#include "CapacitorCode.h"

#include <QRegularExpression>
#include <cmath>

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

} // namespace CapacitorCode
