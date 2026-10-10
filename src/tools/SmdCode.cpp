#include "SmdCode.h"

#include <QRegularExpression>
#include <cmath>

namespace SmdCode {

std::optional<double> decode(const QString &marking)
{
    const QString text = marking.trimmed().toUpper();

    static const QRegularExpression rNotation(R"(^(\d*)R(\d+)$)");
    if (auto m = rNotation.match(text); m.hasMatch())
        return (m.captured(1) + "." + m.captured(2)).toDouble();

    static const QRegularExpression digits(R"(^\d{3,4}$)");
    if (!digits.match(text).hasMatch())
        return std::nullopt;

    const int mantissaDigits = text.size() - 1;
    const int multiplier = text.back().digitValue();
    return text.left(mantissaDigits).toInt() * std::pow(10.0, multiplier);
}

} // namespace SmdCode
