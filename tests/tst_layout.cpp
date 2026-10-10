#include <QtTest>

#include "core/Layout.h"

namespace {

DrawerGroup group(int id, double x, double y, int rows, int cols)
{
    DrawerGroup g;
    g.id = id;
    g.posX = x;
    g.posY = y;
    g.rows = rows;
    g.cols = cols;
    return g;
}

} // namespace

class LayoutTest : public QObject
{
    Q_OBJECT

private slots:
    void overlapDetection()
    {
        QVERIFY(Layout::overlaps(group(1, 0, 0, 4, 4), group(2, 3, 3, 2, 2)));
        QVERIFY(!Layout::overlaps(group(1, 0, 0, 4, 4), group(2, 4, 0, 2, 2))); // Adjacent.
        QVERIFY(!Layout::overlaps(group(1, 0, 0, 4, 4), group(2, 0, 4, 1, 1)));
    }

    void collidesIgnoresSelf()
    {
        const QList<DrawerGroup> groups = {group(1, 0, 0, 3, 3)};
        QVERIFY(!Layout::collides(group(1, 0, 0, 3, 3), groups));
        QVERIFY(Layout::collides(group(2, 1, 1, 1, 1), groups));
    }

    void suggestStartsAtOrigin()
    {
        const auto pos = Layout::suggestPosition({}, 4, 4);
        QCOMPARE(pos.x, 0.0);
        QCOMPARE(pos.y, 0.0);
    }

    void suggestFillsGapsBeforeGrowing()
    {
        // A 4x4 block with a 2x2 beside it leaves a 2x2 gap below the small block.
        const QList<DrawerGroup> groups = {group(1, 0, 0, 4, 4), group(2, 4, 0, 2, 2)};
        const auto pos = Layout::suggestPosition(groups, 2, 2);
        QCOMPARE(pos.x, 4.0);
        QCOMPARE(pos.y, 2.0);
    }

    void suggestBalancesInsteadOfStackingInOneRow()
    {
        // Three 4x4 blocks fit a landscape area best as a 2x2 arrangement.
        QList<DrawerGroup> groups = {group(1, 0, 0, 4, 4), group(2, 4, 0, 4, 4)};
        const auto pos = Layout::suggestPosition(groups, 4, 4);
        QCOMPARE(pos.x, 0.0);
        QCOMPARE(pos.y, 4.0);
    }

    void suggestNeverOverlaps()
    {
        QList<DrawerGroup> groups;
        for (int i = 0; i < 12; ++i) {
            const auto pos = Layout::suggestPosition(groups, 3, 3);
            DrawerGroup g = group(i + 1, 0, 0, 3, 3);
            g.posX = pos.x;
            g.posY = pos.y;
            QVERIFY(!Layout::collides(g, groups));
            groups.append(g);
        }
    }

    void dragSnapsToGridByDefault()
    {
        const auto pos = Layout::dragTarget({1, 1}, 2.4, 0.6, true);
        QCOMPARE(pos.x, 3.0);
        QCOMPARE(pos.y, 2.0);
    }

    void dragFreeKeepsFractions()
    {
        const auto pos = Layout::dragTarget({1, 1}, 2.4, 0.6, false);
        QVERIFY(qFuzzyCompare(pos.x, 3.4));
        QVERIFY(qFuzzyCompare(pos.y, 1.6));
    }

    void dragIsClampedToOrigin()
    {
        const auto pos = Layout::dragTarget({1, 1}, -5, -0.2, false);
        QCOMPARE(pos.x, 0.0);
        QVERIFY(qFuzzyCompare(pos.y, 0.8));
    }

    void growingPushesRightNeighbor()
    {
        DrawerGroup resized = group(1, 0, 0, 2, 2);
        const QList<DrawerGroup> all = {resized, group(2, 2, 0, 2, 2)};
        resized.cols = 4; // 2x2 -> 2x4
        const auto moved = Layout::displacedByResize(all, resized, 2, 2);
        QCOMPARE(moved.size(), 1);
        QCOMPARE(moved[0].id, 2);
        QCOMPARE(moved[0].posX, 4.0);
        QCOMPARE(moved[0].posY, 0.0);
    }

    void growingPushesBottomNeighborDown()
    {
        DrawerGroup resized = group(1, 0, 0, 2, 2);
        const QList<DrawerGroup> all = {resized, group(2, 0, 2, 2, 2)};
        resized.rows = 5;
        const auto moved = Layout::displacedByResize(all, resized, 2, 2);
        QCOMPARE(moved.size(), 1);
        QCOMPARE(moved[0].posY, 5.0);
        QCOMPARE(moved[0].posX, 0.0);
    }

    void pushCascadesAndIgnoresUnaffectedGroups()
    {
        DrawerGroup resized = group(1, 0, 0, 2, 2);
        // 2 sits right of the block, 3 right of 2, 4 far away below.
        const QList<DrawerGroup> all = {resized, group(2, 2, 0, 2, 2), group(3, 4, 0, 2, 2),
                                        group(4, 0, 10, 2, 2)};
        resized.cols = 3;
        const auto moved = Layout::displacedByResize(all, resized, 2, 2);
        QCOMPARE(moved.size(), 2);
        for (const DrawerGroup &g : moved) {
            if (g.id == 2) QCOMPARE(g.posX, 3.0);
            if (g.id == 3) QCOMPARE(g.posX, 5.0);
        }
    }

    void resultNeverOverlaps()
    {
        DrawerGroup resized = group(1, 0, 0, 2, 2);
        QList<DrawerGroup> all = {resized, group(2, 2, 0, 2, 2), group(3, 0, 2, 2, 2),
                                  group(4, 2, 2, 2, 2)};
        resized.rows = 5;
        resized.cols = 5;
        const auto moved = Layout::displacedByResize(all, resized, 2, 2);
        for (const DrawerGroup &m : moved) {
            for (DrawerGroup &g : all) {
                if (g.id == m.id)
                    g = m;
            }
        }
        all[0] = resized;
        for (int i = 0; i < all.size(); ++i) {
            for (int j = i + 1; j < all.size(); ++j)
                QVERIFY2(!Layout::overlaps(all[i], all[j]), qPrintable(QString("%1/%2").arg(i).arg(j)));
        }
    }

    void shrinkingMovesNothing()
    {
        DrawerGroup resized = group(1, 0, 0, 4, 4);
        const QList<DrawerGroup> all = {resized, group(2, 4, 0, 2, 2)};
        resized.cols = 2;
        QVERIFY(Layout::displacedByResize(all, resized, 4, 4).isEmpty());
    }

    void fractionalPositionsOverlap()
    {
        DrawerGroup a = group(1, 0, 0, 2, 2);
        DrawerGroup b = group(2, 0, 0, 2, 2);
        b.posX = 1.5;
        QVERIFY(Layout::overlaps(a, b));
        b.posX = 2.0;
        QVERIFY(!Layout::overlaps(a, b));
    }
};

QTEST_MAIN(LayoutTest)
#include "tst_layout.moc"
