#pragma once

#include <QString>
#include <optional>

// SMD resistor markings: 3 digits (472), 4 digits (1002), R notation (4R7)
// and EIA-96 (two digits plus a multiplier letter, e.g. 01C = 10 kOhm).
namespace SmdCode {

// Returns ohms; nullopt if the marking is not recognized.
std::optional<double> decode(const QString &marking);

// EIA-96 only: the two-digit code is an index (01-96) into the E96 series.
std::optional<double> decodeEia96(const QString &marking);

// The 96 E96 values (100..976) in code order.
const QList<int> &e96Values();

} // namespace SmdCode
