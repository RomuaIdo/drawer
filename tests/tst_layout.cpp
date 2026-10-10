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
