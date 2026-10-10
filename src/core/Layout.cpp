#include "Layout.h"

#include <algorithm>

namespace Layout {

bool overlaps(const DrawerGroup &a, const DrawerGroup &b)
{
    return a.posX < b.posX + b.cols && b.posX < a.posX + a.cols
        && a.posY < b.posY + b.rows && b.posY < a.posY + a.rows;
}

bool collides(const DrawerGroup &group, const QList<DrawerGroup> &others)
{
    return std::any_of(others.begin(), others.end(), [&](const DrawerGroup &other) {
        return other.id != group.id && overlaps(group, other);
    });
}

int nextFreeRow(const QList<DrawerGroup> &groups)
{
    int row = 0;
    for (const DrawerGroup &g : groups)
        row = std::max(row, g.posY + g.rows);
    return row;
}

} // namespace Layout
