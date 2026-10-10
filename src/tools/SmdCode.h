#pragma once

#include <QString>
#include <optional>

// SMD resistor markings: 3 digits (472), 4 digits (1002) and R notation (4R7).
namespace SmdCode {

// Returns ohms; nullopt for unsupported markings (EIA-96 codes are not handled).
std::optional<double> decode(const QString &marking);

} // namespace SmdCode
