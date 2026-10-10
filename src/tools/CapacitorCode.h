#pragma once

#include <QString>
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

// What was typed: a marking such as "104" or "104K", or a value such as "100n" or "4u7".
struct Reading {
    Capacitance capacitance;
    QString marking; // Three-digit marking (with the tolerance letter if given); empty if none fits.
};
std::optional<Reading> read(const QString &input);

// Letter -> tolerance, nullopt for unknown letters.
std::optional<double> tolerancePercentOf(QChar letter);

} // namespace CapacitorCode
