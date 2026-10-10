#pragma once

#include <QList>

#include "Models.h"

// Grid placement rules for drawer groups inside a cabinet.
namespace Layout {

struct Position {
    double x = 0;
    double y = 0;
};

// True if the two groups share any area (touching edges do not count).
bool overlaps(const DrawerGroup &a, const DrawerGroup &b);

// True if `group` overlaps any other group (the group itself is ignored by id).
bool collides(const DrawerGroup &group, const QList<DrawerGroup> &others);

// Free grid position for a rows x cols block that keeps the cabinet compact:
// it minimizes the occupied area, assuming a landscape screen, and prefers
// filling gaps over growing the layout. Ties go to the top-left.
Position suggestPosition(const QList<DrawerGroup> &groups, int rows, int cols);

} // namespace Layout
