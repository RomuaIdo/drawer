#include "SmdCode.h"

#include <QRegularExpression>
#include <cmath>

namespace SmdCode {

namespace {

const QList<int> kE96 = {
    100, 102, 105, 107, 110, 113, 115, 118, 121, 124, 127, 130, 133, 137, 140, 143,
    147, 150, 154, 158, 162, 165, 169, 174, 178, 182, 187, 191, 196, 200, 205, 210,
    215, 221, 226, 232, 237, 243, 249, 255, 261, 267, 274, 280, 287, 294, 301, 309,
    316, 324, 332, 340, 348, 357, 365, 374, 383, 392, 402, 412, 422, 432, 442, 453,
    464, 475, 487, 499, 511, 523, 536, 549, 562, 576, 590, 604, 619, 634, 649, 665,
    681, 698, 715, 732, 750, 768, 787, 806, 825, 845, 866, 887, 909, 931, 953, 976};

// Letter -> multiplier. Two letters share a value in the standard (Y/R, X/S, B/H).
std::optional<double> multiplierOf(QChar letter)
{
    switch (letter.unicode()) {
    case 'Z': return 0.001;
    case 'Y': case 'R': return 0.01;
    case 'X': case 'S': return 0.1;
    case 'A': return 1;
    case 'B': case 'H': return 10;
    case 'C': return 100;
    case 'D': return 1000;
    case 'E': return 10000;
    case 'F': return 100000;
    default: return std::nullopt;
    }
}

} // namespace

const QList<int> &e96Values()
{
    return kE96;
}

std::optional<double> decodeEia96(const QString &marking)
{
    static const QRegularExpression pattern(R"(^(\d{2})([A-Z])$)");
    const auto match = pattern.match(marking.trimmed().toUpper());
    if (!match.hasMatch())
        return std::nullopt;

    const int index = match.captured(1).toInt();
    const auto multiplier = multiplierOf(match.captured(2)[0]);
    if (index < 1 || index > kE96.size() || !multiplier)
        return std::nullopt;
    return kE96[index - 1] * *multiplier;
}

std::optional<double> decode(const QString &marking)
{
    const QString text = marking.trimmed().toUpper();

    static const QRegularExpression rNotation(R"(^(\d*)R(\d+)$)");
    if (auto m = rNotation.match(text); m.hasMatch())
        return (m.captured(1) + "." + m.captured(2)).toDouble();

    if (const auto eia = decodeEia96(text))
        return eia;

    static const QRegularExpression digits(R"(^\d{3,4}$)");
    if (!digits.match(text).hasMatch())
        return std::nullopt;

    const int mantissaDigits = text.size() - 1;
    const int multiplier = text.back().digitValue();
    return text.left(mantissaDigits).toInt() * std::pow(10.0, multiplier);
}

} // namespace SmdCode
