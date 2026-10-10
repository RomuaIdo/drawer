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
                Qt::MouseButtons buttons)
{
    QMouseEvent e(type, pos, w->mapToGlobal(pos), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(w, &e);
}

} // namespace

class CabinetViewTest : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_view = std::make_unique<CabinetView>();
        m_view->resize(800, 600);
        m_view->setLayouts({makeLayout(1, 0, 0, 2, 2), makeLayout(2, 4, 0, 2, 2)});
    }

    void dragSnapsToGrid()
    {
        QSignalSpy spy(m_view.get(), &CabinetView::groupMoved);
        drag({48, 38}, {48 + 72 * 2 + 20, 38 + 52 + 10}); // About 2.3 cells right, 1.2 down.
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][1].toDouble(), 2.0);
        QCOMPARE(spy[0][2].toDouble(), 1.0);
    }

    void freeDragKeepsFractions()
    {
        m_view->setFreeMove(true);
        QSignalSpy spy(m_view.get(), &CabinetView::groupMoved);
        drag({48, 38}, {48 + 36, 38}); // Half a cell right.
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][1].toDouble(), 0.5);
    }

    void overlappingDropIsRejected()
    {
        QSignalSpy spy(m_view.get(), &CabinetView::groupMoved);
        drag({48, 38}, {48 + 72 * 4, 38}); // Lands on the second block.
        QCOMPARE(spy.count(), 0);
    }

    void tinyMovementIsAClickNotADrag()
    {
        QSignalSpy moved(m_view.get(), &CabinetView::groupMoved);
        QSignalSpy selected(m_view.get(), &CabinetView::drawerSelected);
        drag({48, 38}, {49, 38});
        QCOMPARE(moved.count(), 0);
        QCOMPARE(selected.count(), 1);
    }

private:
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
