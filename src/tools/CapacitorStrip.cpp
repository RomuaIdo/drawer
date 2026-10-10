#include "CapacitorStrip.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>

CapacitorStrip::CapacitorStrip(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(130);
}

void CapacitorStrip::setMarking(const QString &marking)
{
    m_marking = marking;
    update();
}

void CapacitorStrip::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPointF center(width() / 2.0, height() * 0.42);
    const double radius = std::min(width() / 4.0, height() * 0.36);

    // Leads: two wires leaving the bottom of the disc.
    const double leadOffset = radius * 0.45;
    p.setPen(QPen(QColor(0x9a, 0x9a, 0xa4), 3, Qt::SolidLine, Qt::RoundCap));
    for (double side : {-1.0, 1.0}) {
        const QPointF top(center.x() + side * leadOffset, center.y() + radius * 0.8);
        p.drawLine(top, QPointF(top.x(), height() - 4));
    }

    // Body: slightly flattened disc with a darker rim and a highlight.
    const QRectF body(center.x() - radius, center.y() - radius, 2 * radius, 2 * radius * 0.95);
    p.setPen(QPen(QColor(0x9a, 0x5b, 0x1f), 2));
    p.setBrush(QColor(0xe8, 0x9a, 0x3c));
    p.drawEllipse(body);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 50));
    p.drawEllipse(body.adjusted(radius * 0.25, radius * 0.15, -radius * 0.9, -radius * 1.0));

    if (m_marking.isEmpty())
        return;

    // Shrink the text until the marking fits inside the disc.
    QFont font = this->font();
    font.setBold(true);
    font.setPixelSize(int(radius * 0.6));
    const double maxWidth = radius * 1.5;
    while (font.pixelSize() > 8 && QFontMetricsF(font).horizontalAdvance(m_marking) > maxWidth)
        font.setPixelSize(font.pixelSize() - 1);
    p.setFont(font);
    p.setPen(QColor(0x2b, 0x1a, 0x06));
    p.drawText(body, Qt::AlignCenter, m_marking);
}
