#include "Formulas.h"

#include <QList>
#include <cmath>

namespace Formulas {

namespace {

const QList<double> kE6 = {1.0, 1.5, 2.2, 3.3, 4.7, 6.8};
const QList<double> kE12 = {1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.9, 4.7, 5.6, 6.8, 8.2};
const QList<double> kE24 = {1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0,
                            3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1};

// E48 and E96 follow the 10^(n/N) rule rounded to 3 digits.
QList<double> computedSeries(int perDecade)
{
    QList<double> values;
    for (int i = 0; i < perDecade; ++i)
        values.append(std::round(std::pow(10.0, double(i) / perDecade) * 100) / 100);
    return values;
}

} // namespace

std::optional<Ohm> solveOhm(std::optional<double> v, std::optional<double> i,
                            std::optional<double> r, std::optional<double> p)
{
    const int known = int(v.has_value()) + int(i.has_value()) + int(r.has_value()) + int(p.has_value());
    if (known < 2)
        return std::nullopt;

    // Derive from the first usable pair; extra inputs are ignored.
    if (v && i) {
        if (*i == 0)
            return std::nullopt;
        r = *v / *i;
    } else if (v && r) {
        if (*r == 0)
            return std::nullopt;
        i = *v / *r;
    } else if (i && r) {
        v = *i * *r;
    } else if (v && p) {
        if (*v == 0 || *p == 0)
            return std::nullopt;
        i = *p / *v;
        r = *v / *i;
    } else if (i && p) {
        if (*i == 0)
            return std::nullopt;
        v = *p / *i;
        r = *v / *i;
    } else if (r && p) {
        if (*r <= 0 || *p < 0)
            return std::nullopt;
        i = std::sqrt(*p / *r);
        v = *i * *r;
    }

    Ohm result;
    result.volts = *v;
    result.amps = *i;
    result.ohms = *r;
    result.watts = result.volts * result.amps;
    return result;
}

std::optional<double> dividerOutput(double vin, double r1, double r2)
{
    if (r1 + r2 == 0)
        return std::nullopt;
    return vin * r2 / (r1 + r2);
}

std::optional<LedResistor> ledResistor(double supplyVolts, double forwardVolts, double amps)
{
    if (amps <= 0 || supplyVolts <= forwardVolts)
        return std::nullopt;
    const double drop = supplyVolts - forwardVolts;
    return LedResistor{drop / amps, drop * amps};
}

std::optional<double> nearestStandard(double ohms, int series)
{
    QList<double> base;
    switch (series) {
    case 6: base = kE6; break;
    case 12: base = kE12; break;
    case 24: base = kE24; break;
    case 48:
    case 96: base = computedSeries(series); break;
    default: return std::nullopt;
    }
    if (ohms <= 0)
        return std::nullopt;

    const int decade = int(std::floor(std::log10(ohms)));
    const double scale = std::pow(10.0, decade);
    for (double value : base) {
        if (value * scale >= ohms * (1 - 1e-9))
            return value * scale;
    }
    return base.first() * scale * 10; // Roll over to the next decade.
}

} // namespace Formulas
