#include "Engineering.h"

#include <QRegularExpression>
#include <cmath>

namespace Engineering {

namespace {

struct Prefix {
    QChar symbol;
    int exponent;
};

const Prefix kPrefixes[] = {
    {'p', -12}, {'n', -9}, {'u', -6}, {QChar(0x00b5), -6}, {QChar(0x03bc), -6}, {'m', -3},
    {'k', 3}, {'K', 3}, {'M', 6}, {'G', 9}, {'R', 0},
};

std::optional<int> exponentOf(const QString &symbol)
{
    if (symbol.isEmpty())
        return 0;
    for (const Prefix &p : kPrefixes) {
        if (symbol.at(0) == p.symbol)
            return p.exponent;
    }
    return std::nullopt;
}

} // namespace

std::optional<double> parse(const QString &text)
{
    const QString t = QString(text).remove(' ');

    // Infix notation: 4k7, 4R7, R47.
    static const QRegularExpression infix(R"(^(\d*)([pnuµμmkKMGR])(\d+)$)");
    if (auto m = infix.match(t); m.hasMatch()) {
        const double mantissa = (m.captured(1) + "." + m.captured(3)).toDouble();
        return mantissa * std::pow(10.0, *exponentOf(m.captured(2)));
    }

    static const QRegularExpression plain(
        R"(^(\d+\.?\d*|\.\d+)([pnuµμmkKMGR]?)(?:Ω|ohms?|F|f)?$)",
        QRegularExpression::CaseInsensitiveOption);
    // Prefix case matters (m vs M), so re-check it against the original text.
    if (auto m = plain.match(t); m.hasMatch()) {
        const auto exp = exponentOf(m.captured(2));
        if (!exp)
            return std::nullopt;
        return m.captured(1).toDouble() * std::pow(10.0, *exp);
    }
    return std::nullopt;
}

QString format(double value, const QString &unit)
{
    if (value == 0.0)
        return "0 " + unit;

    struct Scale {
        int exponent;
        const char *symbol;
    };
    static const Scale scales[] = {{9, "G"}, {6, "M"}, {3, "k"}, {0, ""},
                                   {-3, "m"}, {-6, "µ"}, {-9, "n"}, {-12, "p"}};
    for (const Scale &s : scales) {
        const double scaled = value / std::pow(10.0, s.exponent);
        if (std::abs(scaled) >= 1.0 || s.exponent == -12)
            return QString::number(scaled, 'g', 4) + " " + QString::fromUtf8(s.symbol) + unit;
    }
    return QString::number(value) + " " + unit;
}

} // namespace Engineering
