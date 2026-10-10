#pragma once

#include <QString>
#include <QStringList>
#include <optional>

// Three-digit capacitor marking (e.g. 104 = 100 nF) with optional tolerance letter.
namespace CapacitorCode {

struct Capacitance {
    double picofarads = 0;
    std::optional<double> tolerancePercent; // Set when a tolerance letter was given.
    std::optional<double> tolerancePf;      // Absolute tolerance for letters B, C, D (below 10 pF).
    double farads() const { return picofarads * 1e-12; }
};

// Decodes "104", "104K", "4R7" style or "47" (plain picofarads). Case-insensitive letter.
std::optional<Capacitance> decode(const QString &marking);

// Encodes to a 3-digit code (e.g. 100 nF -> "104"); nullopt if not representable.
std::optional<QString> encode(double picofarads);

// The physical kinds the tool can draw.
enum class CapacitorType { CeramicDisc, CeramicChip, Film, Electrolytic, Tantalum };

QString name(CapacitorType type);

// Usual construction for a capacitance: ceramic disc up to 100 nF, film up to 1 uF,
// electrolytic above. Chip and tantalum are only drawn when chosen by hand.
CapacitorType defaultTypeFor(double picofarads);

// "50", "50V", "6.3v" or "1kV"; nullopt if not a positive voltage.
std::optional<double> parseVoltage(const QString &text);
// "50V", "6.3V", "1kV".
QString formatVoltage(double volts);

// Value as printed on parts: "22p", "100n", "4.7u" (with the micro sign), optionally with "F".
QString compactValue(double picofarads, bool withUnit);

// Relative size of the drawing: higher voltage ratings make bigger parts.
double voltageScale(std::optional<double> volts);

// What was typed: a marking such as "104" or "104K", or a value such as "100n" or "4u7".
struct Reading {
    Capacitance capacitance;
    QString marking; // Three-digit marking (with the tolerance letter if given); empty if none fits.
};
std::optional<Reading> read(const QString &input);

// What a part of this type has printed on it, one entry per line; empty for types that are
// usually left unmarked. The style differs: disc ceramics carry the three-digit code, film
// boxes the value, electrolytic cans value and voltage, tantalum chips value and voltage.
QStringList printedLines(CapacitorType type, const Reading &reading, std::optional<double> volts);

// Letter -> tolerance, nullopt for unknown letters.
std::optional<double> tolerancePercentOf(QChar letter);

} // namespace CapacitorCode
