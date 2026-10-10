#include <QSignalSpy>
#include <QtTest>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>

#include "ui/CabinetView.h"
#include "ui/ItemMime.h"

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

    void rightButtonDoesNotSelect()
    {
        mouseEvent(m_view.get(), QEvent::MouseButtonPress, QPoint(700, 150), Qt::RightButton,
                   Qt::RightButton);
        mouseEvent(m_view.get(), QEvent::MouseMove, QPoint(20, 20), Qt::NoButton, Qt::RightButton);
        mouseEvent(m_view.get(), QEvent::MouseButtonRelease, QPoint(20, 20), Qt::RightButton,
                   Qt::NoButton);
        QVERIFY(m_view->selectedGroups().isEmpty());
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

    void selectedDrawerOutlineIsNotCoveredAndFollowsTheDrawerColor()
    {
        // The first column sits right under the block outline; the line must stay on top
        // and be a darker version of the (medium, yellow) drawer color.
        m_view->setSelectedDrawer(200); // Block 2, first drawer.
        const QImage image = m_view->grab().toImage();
        const int left = 12 + 4 * 72 + 2; // Left edge of the padded cell.
        QCOMPARE(image.pixelColor(left, 12 + 26).rgb(), QColor(0xff, 0xe0, 0x82).darker(200).rgb());
    }

    void selectedBlockOutlineIsNotCoveredByANeighbor()
    {
        // Blocks 1 and 2 touch; block 2 comes later in the list and has its own outline.
        m_view->setLayouts({makeLayout(1, 0, 0, 2, 2), makeLayout(2, 2, 0, 2, 2)});
        click(cellCenter(0, 0));
        const QImage image = m_view->grab().toImage();
        const QColor highlight = m_view->palette().color(QPalette::Highlight);
        QCOMPARE(image.pixelColor(12 + 2 * 72, 12 + 52 + 26), highlight); // Shared edge.
    }

    void lowStockDrawersGetABadge()
    {
        m_view->setLayouts({makeLayout(1, 0, 0, 1, 1)});
        const QImage before = m_view->grab().toImage();
        m_view->setLowStockDrawers({100});
        const QImage after = m_view->grab().toImage();
        QVERIFY(before != after);
        // Red badge near the top-right corner of the drawer.
        const QColor badge = after.pixelColor(12 + 72 - 2 - 9, 12 + 2 + 3 + 3);
        QVERIFY2(badge.red() > 180 && badge.green() < 80, qPrintable(badge.name()));
    }

    void droppingItemsOnADrawerReportsMoveOrCopy()
    {
        QSignalSpy spy(m_view.get(), &CabinetView::itemsDropped);
        std::unique_ptr<QMimeData> mime(ItemMime::encode({7, 8}));
        const QPoint target = cellCenter(4, 0); // Block 2, first drawer (id 200).

        drop(mime.get(), target, Qt::MoveAction);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].value<QList<int>>(), (QList<int>{7, 8}));
        QCOMPARE(spy[0][1].toInt(), 200);
        QCOMPARE(spy[0][2].toBool(), false);

        drop(mime.get(), target, Qt::CopyAction); // What Qt proposes while Ctrl is held.
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy[1][2].toBool(), true);
    }

    void dropsOutsideDrawersOrWithOtherDataAreIgnored()
    {
        QSignalSpy spy(m_view.get(), &CabinetView::itemsDropped);
        std::unique_ptr<QMimeData> mime(ItemMime::encode({7}));
        drop(mime.get(), QPoint(700, 500), Qt::MoveAction);

        QMimeData other;
        other.setText("not items");
        drop(&other, cellCenter(0, 0), Qt::MoveAction);
        QCOMPARE(spy.count(), 0);
    }

    void pickerModeOnlyChoosesDrawers()
    {
        m_view->setPickerMode(true);
        QSignalSpy selected(m_view.get(), &CabinetView::drawerSelected);
        QSignalSpy moved(m_view.get(), &CabinetView::groupsMoved);

        click(cellCenter(4, 0));
        QCOMPARE(selected.count(), 1);
        QCOMPARE(selected[0][0].value<Drawer>().id, 200);

        // No block selection, no dragging, no band, no drops.
        QVERIFY(m_view->selectedGroups().isEmpty());
        drag(cellCenter(0, 0), cellCenter(0, 0) + QPoint(72 * 2, 52));
        QCOMPARE(moved.count(), 0);
        mouseEvent(m_view.get(), QEvent::MouseButtonPress, QPoint(700, 150), Qt::LeftButton, Qt::LeftButton);
        mouseEvent(m_view.get(), QEvent::MouseMove, QPoint(20, 20), Qt::NoButton, Qt::LeftButton);
        mouseEvent(m_view.get(), QEvent::MouseButtonRelease, QPoint(20, 20), Qt::LeftButton, Qt::NoButton);
        QVERIFY(m_view->selectedGroups().isEmpty());
        QVERIFY(!m_view->acceptDrops());

        click(QPoint(700, 500)); // Empty space chooses nothing.
        QCOMPARE(selected.count(), 2); // Only the drag's press on a drawer counted.
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
    // A full drag session over the view: enter, move, drop.
    void drop(const QMimeData *mime, QPoint pos, Qt::DropAction action)
    {
        QDragEnterEvent enter(pos, action, mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(m_view.get(), &enter);
        QDragMoveEvent move(pos, action, mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(m_view.get(), &move);
        QDropEvent drop(pos, action, mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(m_view.get(), &drop);
    }

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
