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

// Shift to apply to a set of blocks dragged by (dx, dy) grid cells. With `snap` the anchor
// block (one of `origins`) lands on a whole cell and the others keep their offset to it.
// The shift is limited so that no block goes above or left of the origin.
Position dragDelta(const QList<Position> &origins, Position anchor, double dx, double dy,
                   bool snap);

// Groups that must move so that `resized` (already carrying its new size) overlaps nothing.
// Neighbors are pushed right or down, whichever is closer, and the push cascades to
// whatever they bump into. `oldRows`/`oldCols` give the extent before resizing, which
// tells whether a neighbor lies beyond the right or the bottom edge. Only changed
// groups are returned; the resized group itself never moves.
QList<DrawerGroup> displacedByResize(const QList<DrawerGroup> &groups, const DrawerGroup &resized,
                                     int oldRows, int oldCols);

} // namespace Layout
