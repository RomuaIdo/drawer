#pragma once

#include <QString>
#include <optional>

// Parsing and formatting of engineering notation (4.7k, 4k7, 100nF, 2u2).
namespace Engineering {

// Accepts SI prefixes (p n u µ m k M G), optional trailing unit (Ω, ohm, R, F)
// and RKM-style infix notation. Returns nullopt if the text is not a number.
std::optional<double> parse(const QString &text);

// Formats with an SI prefix, e.g. format(4700, "Ω") == "4.7 kΩ".
QString format(double value, const QString &unit);

} // namespace Engineering
