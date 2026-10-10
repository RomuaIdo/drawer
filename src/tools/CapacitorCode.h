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

// Letter -> tolerance, nullopt for unknown letters.
std::optional<double> tolerancePercentOf(QChar letter);

} // namespace CapacitorCode
