#pragma once

#include <optional>

// Basic circuit formulas. Inputs use base units (V, A, ohm, W).
namespace Formulas {

struct Ohm {
    double volts = 0;
    double amps = 0;
    double ohms = 0;
    double watts = 0;
};

// Solves V, I, R, P from any two known values; nullopt if fewer than two are given,
// or if the given pair is inconsistent with a division by zero.
std::optional<Ohm> solveOhm(std::optional<double> volts, std::optional<double> amps,
                            std::optional<double> ohms, std::optional<double> watts);

// Vout = Vin * R2 / (R1 + R2); nullopt if R1 + R2 is zero.
std::optional<double> dividerOutput(double vin, double r1, double r2);

struct LedResistor {
    double ohms = 0;
    double watts = 0; // Power dissipated in the resistor.
};

// Series resistor for an LED; nullopt if the supply cannot forward-bias the LED
// or the current is not positive.
std::optional<LedResistor> ledResistor(double supplyVolts, double forwardVolts, double amps);

// Next value at or above `ohms` in an E-series (6, 12, 24, 48 or 96); nullopt if unsupported.
std::optional<double> nearestStandard(double ohms, int series);

} // namespace Formulas
