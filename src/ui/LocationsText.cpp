#include "LocationsText.h"

#include <QCoreApplication>
#include <QStringList>

namespace LocationsText {

QString tooltip(const QList<ItemLocation> &locations, int maxOthers)
{
    QStringList lines;
    for (int i = 0; i < locations.size() && i <= maxOthers; ++i)
        lines << QString("%1/%2 (%3)")
                     .arg(locations[i].group, locations[i].drawerLabel)
                     .arg(locations[i].quantity);
    const int more = locations.size() - 1 - maxOthers;
    if (more > 0)
        lines << QCoreApplication::translate("LocationsText", "+%n more", "", more);
    return lines.join('\n');
}

} // namespace LocationsText
