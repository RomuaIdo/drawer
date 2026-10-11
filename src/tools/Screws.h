#pragma once

#include <QList>
#include <QString>
#include <optional>

// Metric screw data (ISO coarse thread). All lengths are in millimeters.
namespace Screws {

struct Screw {
    QString name;            // e.g. "M3".
    double diameter;         // Nominal (major) diameter.
    double pitch;            // Coarse pitch.
    double tapDrill;         // Drill for cutting the thread.
    double clearanceClose;   // Clearance hole, close fit (ISO 273 fine).
    double clearanceNormal;  // Clearance hole, normal fit (ISO 273 medium).
    double clearanceLoose;   // Clearance hole, loose fit (ISO 273 coarse).
    double cylinderHeadDiameter; // Socket cap head (ISO 4762).
    double cylinderHeadHeight;
    double counterboreDiameter;  // Recess that sinks a socket cap head.
    double counterboreDepth;
    double coneHeadDiameter;     // Countersunk head (ISO 7046).
    double countersinkDiameter;  // Opening of the countersink (ISO 15065).
};

const QList<Screw> &all();
std::optional<Screw> find(const QString &name);

} // namespace Screws
