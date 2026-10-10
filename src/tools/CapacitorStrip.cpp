#include "CapacitorStrip.h"

#include <QPainter>
#include <QRadialGradient>

CapacitorStrip::CapacitorStrip(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(110);
}

void CapacitorStrip::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPointF center(width() / 2.0, height() * 0.4);
    const double radius = std::min(width() / 4.0, height() * 0.34);

    // Pins.
    const double pinOffset = radius * 0.4;
    p.setPen(QPen(QColor(0xa8, 0xa8, 0xb0), 3, Qt::SolidLine, Qt::RoundCap));
    for (double side : {-1.0, 1.0}) {
        const double x = center.x() + side * pinOffset;
        p.drawLine(QPointF(x, center.y() + radius * 0.8), QPointF(x, height() - 4));
    }

    // Body: flat orange with a soft light spot as the only detail.
    QRadialGradient gradient(center - QPointF(radius * 0.35, radius * 0.4), radius * 1.6);
    gradient.setColorAt(0.0, QColor(0xf2, 0xa9, 0x4e));
    gradient.setColorAt(1.0, QColor(0xe0, 0x8a, 0x2b));
    p.setPen(QPen(QColor(0xc4, 0x75, 0x20), 1.5));
    p.setBrush(gradient);
    p.drawEllipse(QRectF(center.x() - radius, center.y() - radius, 2 * radius, 2 * radius * 0.95));
}
