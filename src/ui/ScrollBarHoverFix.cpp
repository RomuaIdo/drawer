#include "ScrollBarHoverFix.h"

#include <QCursor>
#include <QEvent>
#include <QHoverEvent>
#include <QPointer>
#include <QScrollBar>
#include <QApplication>
#include <QTimer>

bool ScrollBarHoverFix::eventFilter(QObject *watched, QEvent *event)
{
    auto *bar = qobject_cast<QScrollBar *>(watched);
    if (bar && event->type() == QEvent::MouseButtonRelease) {
        // After the release has been handled, tell the bar if the mouse is no longer over it.
        QTimer::singleShot(0, bar, [bar = QPointer<QScrollBar>(bar)] {
            if (!bar || bar->rect().contains(bar->mapFromGlobal(QCursor::pos())))
                return;
            QEvent leave(QEvent::Leave);
            QApplication::sendEvent(bar, &leave);
            QHoverEvent hover(QEvent::HoverLeave, QPointF(-1, -1), QCursor::pos(), QPointF(0, 0));
            QApplication::sendEvent(bar, &hover);
            bar->update();
        });
    }
    return QObject::eventFilter(watched, event);
}
