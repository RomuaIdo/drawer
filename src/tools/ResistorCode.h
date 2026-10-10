#pragma once

#include <QList>
#include <optional>

// Resistor color code (IEC 60062) decoding and encoding.
namespace ResistorCode {

enum class Color {
    Black, Brown, Red, Orange, Yellow, Green, Blue, Violet, Grey, White, Gold, Silver, None
};

struct Resistance {
    double ohms = 0;
    double tolerancePercent = 20;
    std::optional<int> tempcoPpm; // Only present on 6-band resistors.
};

// Valid band counts are 4 (2 digits), 5 and 6 (3 digits).
bool isValidBandCount(int bands);

// Colors allowed in each role.
QList<Color> digitColors();
QList<Color> multiplierColors();
QList<Color> toleranceColors();
QList<Color> tempcoColors();

QString name(Color color);
std::optional<double> toleranceOf(Color color);

// Decodes a full band sequence; nullopt if the length or any color is invalid for its role.
std::optional<Resistance> decode(const QList<Color> &bands);

// Finds the bands for a value; nullopt if it is not representable with `bandCount` bands
// or the tolerance has no color. `tempcoPpm` is only used for 6 bands.
std::optional<QList<Color>> encode(double ohms, double tolerancePercent, int bandCount,
                                   int tempcoPpm = 100);

} // namespace ResistorCode
