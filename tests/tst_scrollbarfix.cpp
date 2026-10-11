#include <QScrollBar>
#include <QtTest>

#include "ui/ScrollBarHoverFix.h"

namespace {

// Counts the leave events it receives.
class WatchedBar : public QScrollBar
{
public:
    int leaves = 0;

protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::Leave)
            ++leaves;
        return QScrollBar::event(e);
    }
};

} // namespace

class ScrollBarFixTest : public QObject
{
    Q_OBJECT

private slots:
    void releasingOutsideTheBarSendsALeave()
    {
        ScrollBarHoverFix fix;
        qApp->installEventFilter(&fix);
        WatchedBar bar;
        bar.setOrientation(Qt::Horizontal);
        bar.setGeometry(300, 300, 200, 16); // The cursor is not over it.
        bar.show();

        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(5, 5), QPointF(305, 305),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&bar, &release);
        QTRY_COMPARE(bar.leaves, 1);
        qApp->removeEventFilter(&fix);
    }
};

QTEST_MAIN(ScrollBarFixTest)
#include "tst_scrollbarfix.moc"
