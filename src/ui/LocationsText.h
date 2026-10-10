#pragma once

#include <QList>
#include <QString>

#include "core/Models.h"

namespace LocationsText {

// One "Group/R1C1 (4)" line per drawer: the first location (the item's own drawer), then at
// most `maxOthers` more, and a "+N more" line for the rest.
QString tooltip(const QList<ItemLocation> &locations, int maxOthers = 5);

} // namespace LocationsText
