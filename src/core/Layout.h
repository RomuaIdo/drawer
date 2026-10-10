#pragma once

#include <QList>

#include "Models.h"

// Grid placement rules for drawer groups inside a cabinet.
namespace Layout {

// True if the two groups share at least one grid cell.
bool overlaps(const DrawerGroup &a, const DrawerGroup &b);

// True if `group` overlaps any other group (the group itself is ignored by id).
bool collides(const DrawerGroup &group, const QList<DrawerGroup> &others);

// First grid row below every existing group.
int nextFreeRow(const QList<DrawerGroup> &groups);

} // namespace Layout
