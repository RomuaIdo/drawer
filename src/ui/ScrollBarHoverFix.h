#pragma once

#include <QObject>

// Some styles keep a scroll bar highlighted after it was dragged with the mouse released
// outside it, because no leave event arrives. This filter, installed on the application,
// sends that leave event.
class ScrollBarHoverFix : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};
