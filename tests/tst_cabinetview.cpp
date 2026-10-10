#include <QSignalSpy>
#include <QtTest>

#include "ui/CabinetView.h"

namespace {

CabinetView::GroupLayout makeLayout(int id, double x, double y, int rows, int cols)
{
    CabinetView::GroupLayout l;
    l.group.id = id;
    l.group.rows = rows;
    l.group.cols = cols;
    l.group.posX = x;
    l.group.posY = y;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c)
            l.drawers.append({id * 100 + r * cols + c, id, r, c, DrawerSize::Medium, "d"});
    }
    return l;
}

void mouseEvent(QWidget *w, QEvent::Type type, QPoint pos, Qt::MouseButton button,
                Qt::MouseButtons buttons, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    QMouseEvent e(type, pos, w->mapToGlobal(pos), button, buttons, mods);
    QApplication::sendEvent(w, &e);
}

// Pixel at the middle of cell (col, row) of the view, in the default geometry.
QPoint cellCenter(double col, double row) { return {int(12 + col * 72 + 36), int(12 + row * 52 + 26)}; }

QList<DrawerGroup> movedGroups(const QSignalSpy &spy) { return spy.at(0).at(0).value<QList<DrawerGroup>>(); }

} // namespace

class CabinetViewTest : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_view = std::make_unique<CabinetView>();
        m_view->resize(800, 600);
        // Two blocks side by side and one below, all 2x2.
        m_view->setLayouts({makeLayout(1, 0, 0, 2, 2), makeLayout(2, 4, 0, 2, 2),
                            makeLayout(3, 0, 4, 2, 2)});
    }

    void dragSnapsToGrid()
    {
        QSignalSpy spy(m_view.get(), &CabinetView::groupsMoved);
        drag(cellCenter(0, 0), cellCenter(0, 0) + QPoint(72 * 2 + 20, 52 + 10)); // ~2.3 right, 1.2 down.
        QCOMPARE(spy.count(), 1);
        const auto moved = movedGroups(spy);
        QCOMPARE(moved.size(), 1);
        QCOMPARE(moved[0].posX, 2.0);
        QCOMPARE(moved[0].posY, 1.0);
    }

    void freeDragKeepsFractions()
    {
        m_view->setFreeMove(true);
        QSignalSpy spy(m_view.get(), &CabinetView::groupsMoved);
        drag(cellCenter(0, 0), cellCenter(0, 0) + QPoint(36, 0)); // Half a cell right.
        QCOMPARE(spy.count(), 1);
        QCOMPARE(movedGroups(spy)[0].posX, 0.5);
    }

    void overlappingDropIsRejected()
    {
        QSignalSpy spy(m_view.get(), &CabinetView::groupsMoved);
        drag(cellCenter(0, 0), cellCenter(0, 0) + QPoint(72 * 4, 0)); // Lands on the second block.
        QCOMPARE(spy.count(), 0);
    }

    void tinyMovementIsAClickNotADrag()
    {
        QSignalSpy moved(m_view.get(), &CabinetView::groupsMoved);
        QSignalSpy selected(m_view.get(), &CabinetView::drawerSelected);
        drag(cellCenter(0, 0), cellCenter(0, 0) + QPoint(1, 0));
        QCOMPARE(moved.count(), 0);
        QCOMPARE(selected.count(), 1);
    }

    void ctrlClickBuildsASelection()
    {
        click(cellCenter(0, 0));
        QCOMPARE(m_view->selectedGroups(), (QSet<int>{1}));
        click(cellCenter(4, 0), Qt::ControlModifier);
        QCOMPARE(m_view->selectedGroups(), (QSet<int>{1, 2}));
        click(cellCenter(0, 0), Qt::ControlModifier); // Toggles off.
        QCOMPARE(m_view->selectedGroups(), (QSet<int>{2}));
    }

    void plainClickOnASelectedBlockCollapsesTheSelection()
    {
        click(cellCenter(0, 0));
        click(cellCenter(4, 0), Qt::ControlModifier);
        click(cellCenter(4, 0));
        QCOMPARE(m_view->selectedGroups(), (QSet<int>{2}));
    }

    void rubberBandSelectsIntersectingBlocks()
    {
        // From empty space at the right, over blocks 1 and 2 but not 3.
        const QPoint start(700, 150);
        mouseEvent(m_view.get(), QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        mouseEvent(m_view.get(), QEvent::MouseMove, QPoint(20, 20), Qt::NoButton, Qt::LeftButton);
        mouseEvent(m_view.get(), QEvent::MouseButtonRelease, QPoint(20, 20), Qt::LeftButton,
                   Qt::NoButton);
        QCOMPARE(m_view->selectedGroups(), (QSet<int>{1, 2}));

        click(QPoint(700, 500)); // Empty space clears it.
        QVERIFY(m_view->selectedGroups().isEmpty());
    }

    void rightButtonBandSelectsToo()
    {
        mouseEvent(m_view.get(), QEvent::MouseButtonPress, QPoint(700, 150), Qt::RightButton,
                   Qt::RightButton);
        mouseEvent(m_view.get(), QEvent::MouseMove, QPoint(20, 20), Qt::NoButton, Qt::RightButton);
        mouseEvent(m_view.get(), QEvent::MouseButtonRelease, QPoint(20, 20), Qt::RightButton,
                   Qt::NoButton);
        QCOMPARE(m_view->selectedGroups(), (QSet<int>{1, 2}));
    }

    void bandIsRepaintedOnEveryMove()
    {
        m_view->show();
        QVERIFY(QTest::qWaitForWindowExposed(m_view.get()));
        mouseEvent(m_view.get(), QEvent::MouseButtonPress, QPoint(700, 500), Qt::LeftButton,
                   Qt::LeftButton);
        // Moves over empty space, where the selection does not change, must still repaint.
        const auto bandPixels = [&](QPoint corner) {
            mouseEvent(m_view.get(), QEvent::MouseMove, corner, Qt::NoButton, Qt::LeftButton);
            QCoreApplication::sendPostedEvents(m_view.get(), QEvent::UpdateRequest);
            return m_view->grab().toImage();
        };
        const QImage small = bandPixels(QPoint(650, 450));
        const QImage large = bandPixels(QPoint(560, 380));
        QVERIFY(small != large);
        mouseEvent(m_view.get(), QEvent::MouseButtonRelease, QPoint(560, 380), Qt::LeftButton,
                   Qt::NoButton);
    }

    void selectedDrawerOutlineIsNotCoveredByTheBlockOutline()
    {
        // The first column sits right under the block outline; the blue line must stay on top.
        m_view->setSelectedDrawer(100 + 100); // Block 2, first drawer.
        const QImage image = m_view->grab().toImage();
        const int left = 12 + 4 * 72 + 2; // Left edge of the padded cell.
        const QColor edge = image.pixelColor(left, 12 + 26);
        QVERIFY2(edge.blue() > 200 && edge.red() < 80, qPrintable(edge.name()));
    }

    void groupColorOverridesTheSizeColor()
    {
        auto layouts = QList<CabinetView::GroupLayout>{makeLayout(1, 0, 0, 1, 1)};
        layouts[0].group.color = "#3366cc";
        m_view->setLayouts(layouts);
        const QImage image = m_view->grab().toImage();
        // Middle of the drawer, away from its text: the custom color, not the size color.
        const QColor fill = image.pixelColor(12 + 10, 12 + 40);
        QCOMPARE(fill, QColor("#3366cc"));
    }

    void draggingASelectionMovesEveryBlock()
    {
        click(cellCenter(0, 0));
        click(cellCenter(4, 0), Qt::ControlModifier);

        QSignalSpy spy(m_view.get(), &CabinetView::groupsMoved);
        drag(cellCenter(4, 0), cellCenter(4, 0) + QPoint(0, 52 * 2)); // Two cells down.
        QCOMPARE(spy.count(), 1);
        const auto moved = movedGroups(spy);
        QCOMPARE(moved.size(), 2);
        for (const DrawerGroup &g : moved)
            QCOMPARE(g.posY, 2.0);
        QCOMPARE(moved[0].posX + 4, moved[1].posX); // Offset between them is kept.
    }

    void selectionDragIsRejectedIfAnyBlockWouldOverlap()
    {
        click(cellCenter(0, 0));
        click(cellCenter(4, 0), Qt::ControlModifier);

        QSignalSpy spy(m_view.get(), &CabinetView::groupsMoved);
        // Four cells down puts block 1 on top of the unselected block 3.
        drag(cellCenter(4, 0), cellCenter(4, 0) + QPoint(0, 52 * 4));
        QCOMPARE(spy.count(), 0);
    }

    void dragStopsAtTheEdgeForTheWholeSelection()
    {
        click(cellCenter(0, 0));
        click(cellCenter(4, 0), Qt::ControlModifier);

        QSignalSpy spy(m_view.get(), &CabinetView::groupsMoved);
        // Block 1 is at x=0, so moving left is impossible for both.
        drag(cellCenter(4, 0), cellCenter(4, 0) + QPoint(-72 * 2, 52));
        QCOMPARE(spy.count(), 1);
        const auto moved = movedGroups(spy);
        QCOMPARE(moved[0].posX, 0.0);
        QCOMPARE(moved[1].posX, 4.0);
    }

    void escapeClearsTheSelection()
    {
        click(cellCenter(0, 0));
        QTest::keyClick(m_view.get(), Qt::Key_Escape);
        QVERIFY(m_view->selectedGroups().isEmpty());
        QTest::keyClick(m_view.get(), Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(m_view->selectedGroups().size(), 3);
    }

private:
    void click(QPoint pos, Qt::KeyboardModifiers mods = Qt::NoModifier)
    {
        mouseEvent(m_view.get(), QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, mods);
        mouseEvent(m_view.get(), QEvent::MouseButtonRelease, pos, Qt::LeftButton, Qt::NoButton, mods);
    }

    void drag(QPoint from, QPoint to)
    {
        mouseEvent(m_view.get(), QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
        mouseEvent(m_view.get(), QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton);
        mouseEvent(m_view.get(), QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
    }

    std::unique_ptr<CabinetView> m_view;
};

QTEST_MAIN(CabinetViewTest)
#include "tst_cabinetview.moc"
