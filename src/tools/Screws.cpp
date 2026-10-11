#include "Screws.h"

namespace Screws {

const QList<Screw> &all()
{
    //                name    d    pitch tap  close normal loose capD capH cbD  cbH  coneD sinkD
    static const QList<Screw> screws = {
        {"M2",   2.0, 0.40, 1.60, 2.2, 2.4, 2.6, 3.8, 2.0, 4.5, 2.2, 3.8, 4.4},
        {"M2.5", 2.5, 0.45, 2.05, 2.7, 2.9, 3.1, 4.5, 2.5, 5.5, 2.7, 4.7, 5.5},
        {"M3",   3.0, 0.50, 2.50, 3.2, 3.4, 3.6, 5.5, 3.0, 6.5, 3.4, 5.5, 6.3},
        {"M4",   4.0, 0.70, 3.30, 4.3, 4.5, 4.8, 7.0, 4.0, 8.0, 4.4, 8.4, 9.4},
    };
    return screws;
}

std::optional<Screw> find(const QString &name)
{
    for (const Screw &screw : all()) {
        if (screw.name.compare(name, Qt::CaseInsensitive) == 0)
            return screw;
    }
    return std::nullopt;
}

} // namespace Screws
