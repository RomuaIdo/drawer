#include "Layout.h"

#include <algorithm>
#include <QSet>
#include <cmath>
#include <optional>

namespace Layout {

namespace {

constexpr double kEpsilon = 1e-9;
constexpr double kTargetAspect = 1.5; // Width / height of the area to fill.

struct Candidate {
    Position position;
    double area;
    double longestSide;
};

bool better(const Candidate &a, const Candidate &b)
{
    if (std::abs(a.area - b.area) > kEpsilon)
        return a.area < b.area;
    if (std::abs(a.longestSide - b.longestSide) > kEpsilon)
        return a.longestSide < b.longestSide;
    if (std::abs(a.position.y - b.position.y) > kEpsilon)
        return a.position.y < b.position.y;
    return a.position.x < b.position.x;
}

// Values worth trying on one axis: every whole cell up to the layout edge,
// plus the exact edges of existing groups (they may be fractional).
QList<double> axisCandidates(double extent, const QList<double> &edges)
{
    QList<double> values = edges;
    for (int v = 0; v <= int(std::ceil(extent)); ++v)
        values.append(v);
    return values;
}

} // namespace

bool overlaps(const DrawerGroup &a, const DrawerGroup &b)
{
    return a.posX < b.posX + b.cols - kEpsilon && b.posX < a.posX + a.cols - kEpsilon
        && a.posY < b.posY + b.rows - kEpsilon && b.posY < a.posY + a.rows - kEpsilon;
}

bool collides(const DrawerGroup &group, const QList<DrawerGroup> &others)
{
    return std::any_of(others.begin(), others.end(), [&](const DrawerGroup &other) {
        return other.id != group.id && overlaps(group, other);
    });
}

Position suggestPosition(const QList<DrawerGroup> &groups, int rows, int cols)
{
    double width = 0;
    double height = 0;
    QList<double> xEdges{0};
    QList<double> yEdges{0};
    for (const DrawerGroup &g : groups) {
        width = std::max(width, g.posX + g.cols);
        height = std::max(height, g.posY + g.rows);
        xEdges.append(g.posX + g.cols);
        yEdges.append(g.posY + g.rows);
    }

    DrawerGroup probe;
    probe.rows = rows;
    probe.cols = cols;

    std::optional<Candidate> best;
    for (double x : axisCandidates(width, xEdges)) {
        for (double y : axisCandidates(height, yEdges)) {
            probe.posX = x;
            probe.posY = y;
            if (collides(probe, groups))
                continue;

            const double w = std::max(width, x + cols);
            const double h = std::max(height, y + rows);
            // Smallest landscape box that contains the layout.
            const double boxW = std::max(w, h * kTargetAspect);
            const double boxH = std::max(h, w / kTargetAspect);
            const Candidate candidate{{x, y}, boxW * boxH, std::max(w, h)};
            if (!best || better(candidate, *best))
                best = candidate;
        }
    }
    return best ? best->position : Position{0, height};
}

Position dragTarget(Position start, double dx, double dy, bool snap)
{
    double x = start.x + dx;
    double y = start.y + dy;
    if (snap) {
        x = std::round(x);
        y = std::round(y);
    }
    return {std::max(0.0, x), std::max(0.0, y)};
}

QList<DrawerGroup> displacedByResize(const QList<DrawerGroup> &groups, const DrawerGroup &resized,
                                     int oldRows, int oldCols)
{
    enum class Push { Right, Down };

    QList<DrawerGroup> current = groups;
    QSet<int> moved;

    struct Mover {
        DrawerGroup group;
        std::optional<Push> direction; // Unset for the resized group itself.
    };
    QList<Mover> queue{{resized, std::nullopt}};
    const double oldRight = resized.posX + oldCols;
    const double oldBottom = resized.posY + oldRows;

    while (!queue.isEmpty()) {
        const Mover mover = queue.takeFirst();
        for (DrawerGroup &other : current) {
            if (other.id == resized.id || other.id == mover.group.id
                || !overlaps(mover.group, other))
                continue;

            Push push;
            if (mover.direction) {
                push = *mover.direction;
            } else {
                const double right = mover.group.posX + mover.group.cols - other.posX;
                const double down = mover.group.posY + mover.group.rows - other.posY;
                const bool beyondRight = other.posX >= oldRight - kEpsilon;
                const bool beyondBottom = other.posY >= oldBottom - kEpsilon;
                if (beyondRight != beyondBottom)
                    push = beyondRight ? Push::Right : Push::Down;
                else
                    push = right <= down ? Push::Right : Push::Down;
            }

            if (push == Push::Right)
                other.posX = mover.group.posX + mover.group.cols;
            else
                other.posY = mover.group.posY + mover.group.rows;
            moved.insert(other.id);
            queue.append({other, push});
        }
    }

    QList<DrawerGroup> result;
    for (const DrawerGroup &g : current) {
        if (moved.contains(g.id))
            result.append(g);
    }
    return result;
}

} // namespace Layout
