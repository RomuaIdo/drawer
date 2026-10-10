#include "ResistorCode.h"

#include <QObject>
#include <cmath>

namespace ResistorCode {

namespace {

using C = Color;

int digitOf(Color c)
{
    return int(c) <= int(C::White) ? int(c) : -1;
}

std::optional<double> multiplierOf(Color c)
{
    if (digitOf(c) >= 0)
        return std::pow(10.0, digitOf(c));
    if (c == C::Gold)
        return 0.1;
    if (c == C::Silver)
        return 0.01;
    return std::nullopt;
}

std::optional<int> tempcoOf(Color c)
{
    switch (c) {
    case C::Black: return 250;
    case C::Brown: return 100;
    case C::Red: return 50;
    case C::Orange: return 15;
    case C::Yellow: return 25;
    case C::Green: return 20;
    case C::Blue: return 10;
    case C::Violet: return 5;
    case C::Grey: return 1;
    default: return std::nullopt;
    }
}

Color colorForDigit(int digit)
{
    return static_cast<Color>(digit);
}

Color multiplierColor(int exponent)
{
    if (exponent == -1)
        return C::Gold;
    if (exponent == -2)
        return C::Silver;
    return colorForDigit(exponent);
}

} // namespace

bool isValidBandCount(int bands)
{
    return bands >= 4 && bands <= 6;
}

QList<Color> digitColors()
{
    return {C::Black, C::Brown, C::Red, C::Orange, C::Yellow,
            C::Green, C::Blue, C::Violet, C::Grey, C::White};
}

QList<Color> multiplierColors()
{
    return digitColors() + QList<Color>{C::Gold, C::Silver};
}

QList<Color> toleranceColors()
{
    return {C::Brown, C::Red, C::Green, C::Blue, C::Violet, C::Grey, C::Gold, C::Silver, C::None};
}

QList<Color> tempcoColors()
{
    return {C::Black, C::Brown, C::Red, C::Orange, C::Yellow,
            C::Green, C::Blue, C::Violet, C::Grey};
}

QString name(Color color)
{
    switch (color) {
    case C::Black: return QObject::tr("Black");
    case C::Brown: return QObject::tr("Brown");
    case C::Red: return QObject::tr("Red");
    case C::Orange: return QObject::tr("Orange");
    case C::Yellow: return QObject::tr("Yellow");
    case C::Green: return QObject::tr("Green");
    case C::Blue: return QObject::tr("Blue");
    case C::Violet: return QObject::tr("Violet");
    case C::Grey: return QObject::tr("Grey");
    case C::White: return QObject::tr("White");
    case C::Gold: return QObject::tr("Gold");
    case C::Silver: return QObject::tr("Silver");
    case C::None: return QObject::tr("None");
    }
    return {};
}

std::optional<double> toleranceOf(Color color)
{
    switch (color) {
    case C::Brown: return 1;
    case C::Red: return 2;
    case C::Green: return 0.5;
    case C::Blue: return 0.25;
    case C::Violet: return 0.1;
    case C::Grey: return 0.05;
    case C::Gold: return 5;
    case C::Silver: return 10;
    case C::None: return 20;
    default: return std::nullopt;
    }
}

std::optional<Resistance> decode(const QList<Color> &bands)
{
    if (!isValidBandCount(bands.size()))
        return std::nullopt;

    const int digitCount = bands.size() == 4 ? 2 : 3;
    double significant = 0;
    for (int i = 0; i < digitCount; ++i) {
        const int d = digitOf(bands[i]);
        if (d < 0)
            return std::nullopt;
        significant = significant * 10 + d;
    }

    const auto multiplier = multiplierOf(bands[digitCount]);
    const auto tolerance = toleranceOf(bands[digitCount + 1]);
    if (!multiplier || !tolerance)
        return std::nullopt;

    Resistance result;
    result.ohms = significant * *multiplier;
    result.tolerancePercent = *tolerance;
    if (bands.size() == 6) {
        result.tempcoPpm = tempcoOf(bands[5]);
        if (!result.tempcoPpm)
            return std::nullopt;
    }
    return result;
}

std::optional<QList<Color>> encode(double ohms, double tolerancePercent, int bandCount, int tempcoPpm)
{
    if (!isValidBandCount(bandCount) || ohms < 0)
        return std::nullopt;

    Color toleranceColor = C::None;
    bool toleranceFound = false;
    for (Color c : toleranceColors()) {
        if (std::abs(*toleranceOf(c) - tolerancePercent) < 1e-9) {
            toleranceColor = c;
            toleranceFound = true;
        }
    }
    if (!toleranceFound)
        return std::nullopt;

    const int digitCount = bandCount == 4 ? 2 : 3;
    QList<Color> bands;

    if (ohms == 0) {
        bands = QList<Color>(digitCount + 1, C::Black);
    } else {
        // Find the exponent that leaves exactly `digitCount` integer digits.
        const double low = std::pow(10.0, digitCount - 1);
        const double high = std::pow(10.0, digitCount);
        bool found = false;
        for (int exponent = -2; exponent <= 9 && !found; ++exponent) {
            const double scaled = ohms / std::pow(10.0, exponent);
            const double rounded = std::round(scaled);
            if (scaled < low || scaled >= high || std::abs(scaled - rounded) > 1e-6 * scaled)
                continue;
            int digits = int(rounded);
            QList<Color> digitBands;
            for (int i = 0; i < digitCount; ++i) {
                digitBands.prepend(colorForDigit(digits % 10));
                digits /= 10;
            }
            bands = digitBands + QList<Color>{multiplierColor(exponent)};
            found = true;
        }
        if (!found)
            return std::nullopt;
    }

    bands.append(toleranceColor);
    if (bandCount == 6) {
        bool tempcoFound = false;
        for (Color c : tempcoColors()) {
            if (tempcoOf(c) == tempcoPpm) {
                bands.append(c);
                tempcoFound = true;
            }
        }
        if (!tempcoFound)
            return std::nullopt;
    }
    return bands;
}

} // namespace ResistorCode
