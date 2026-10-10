#include <QtTest>

#include "core/Layout.h"

namespace {

DrawerGroup group(int id, int x, int y, int rows, int cols)
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

    void nextFreeRow()
    {
        QCOMPARE(Layout::nextFreeRow({}), 0);
        QCOMPARE(Layout::nextFreeRow({group(1, 0, 0, 3, 3), group(2, 4, 1, 4, 1)}), 5);
    }
};

QTEST_MAIN(LayoutTest)
#include "tst_layout.moc"
