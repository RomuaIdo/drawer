#include "CapacitorStrip.h"

#include <QPainter>
#include <QRadialGradient>

using CapacitorCode::CapacitorType;

namespace {

const QColor kPin(0xa8, 0xa8, 0xb0);
const QColor kTerminal(0xc4, 0xc6, 0xcc);

void drawPins(QPainter &p, const QPointF &top, double spacing, double length, double secondShorter = 0)
{
    p.setPen(QPen(kPin, 3, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(top.x() - spacing / 2, top.y()), QPointF(top.x() - spacing / 2, top.y() + length));
    p.drawLine(QPointF(top.x() + spacing / 2, top.y()),
               QPointF(top.x() + spacing / 2, top.y() + length - secondShorter));
}

} // namespace

CapacitorStrip::CapacitorStrip(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(110);
}

void CapacitorStrip::setType(CapacitorType type)
{
    if (type == m_type)
        return;
    m_type = type;
    update();
}

void CapacitorStrip::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPointF center(width() / 2.0, height() / 2.0);
    switch (m_type) {
    case CapacitorType::CeramicDisc: drawDisc(p, center); break;
    case CapacitorType::CeramicChip: drawChip(p, center); break;
    case CapacitorType::Film: drawFilm(p, center); break;
    case CapacitorType::Electrolytic: drawElectrolytic(p, center); break;
    case CapacitorType::Tantalum: drawTantalum(p, center); break;
    }
}

// Flat orange disc with a soft light spot, on two pins.
void CapacitorStrip::drawDisc(QPainter &p, const QPointF &c) const
{
    const double radius = 34;
    const QPointF body(c.x(), c.y() - 12);
    drawPins(p, QPointF(body.x(), body.y() + radius * 0.8), radius * 0.8, 42);

    QRadialGradient gradient(body - QPointF(radius * 0.35, radius * 0.4), radius * 1.6);
    gradient.setColorAt(0.0, QColor(0xf2, 0xa9, 0x4e));
    gradient.setColorAt(1.0, QColor(0xe0, 0x8a, 0x2b));
    p.setPen(QPen(QColor(0xc4, 0x75, 0x20), 1.5));
    p.setBrush(gradient);
    p.drawEllipse(QRectF(body.x() - radius, body.y() - radius, 2 * radius, 2 * radius * 0.95));
}

// Small tan block with silver ends.
void CapacitorStrip::drawChip(QPainter &p, const QPointF &c) const
{
    const QRectF body(c.x() - 52, c.y() - 26, 104, 52);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xb9, 0x8b, 0x5e));
    p.drawRoundedRect(body, 3, 3);
    p.setBrush(kTerminal);
    p.drawRect(QRectF(body.left(), body.top(), 18, body.height()));
    p.drawRect(QRectF(body.right() - 18, body.top(), 18, body.height()));
}

// Blue box with a light edge, on two pins.
void CapacitorStrip::drawFilm(QPainter &p, const QPointF &c) const
{
    const QRectF body(c.x() - 46, c.y() - 36, 92, 62);
    drawPins(p, QPointF(c.x(), body.bottom() - 2), 56, 24);

    p.setPen(QPen(QColor(0x2f, 0x6f, 0xa3), 1.5));
    p.setBrush(QColor(0x3f, 0x8a, 0xc4));
    p.drawRoundedRect(body, 6, 6);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 45));
    p.drawRoundedRect(QRectF(body.left() + 6, body.top() + 6, 12, body.height() - 12), 4, 4);
}

// Dark can with a light stripe marking the negative side; the shorter pin is the negative one.
void CapacitorStrip::drawElectrolytic(QPainter &p, const QPointF &c) const
{
    const QRectF body(c.x() - 28, c.y() - 40, 56, 66);
    drawPins(p, QPointF(c.x(), body.bottom() - 2), 22, 26, 8);

    p.setPen(QPen(QColor(0x14, 0x2b, 0x52), 1.5));
    p.setBrush(QColor(0x1f, 0x3d, 0x73));
    p.drawRoundedRect(body, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xcf, 0xd8, 0xe6));
    p.drawRect(QRectF(body.left() + 4, body.top() + 8, 10, body.height() - 16)); // Negative stripe.
    p.setBrush(QColor(255, 255, 255, 40));
    p.drawEllipse(QRectF(body.left() + 18, body.top() + 3, 28, 7)); // Top rim.
}

// Orange molded block, darker band at the positive end, silver terminals.
void CapacitorStrip::drawTantalum(QPainter &p, const QPointF &c) const
{
    const QRectF body(c.x() - 52, c.y() - 28, 104, 56);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xe6, 0xa1, 0x34));
    p.drawRoundedRect(body, 4, 4);
    p.setBrush(QColor(0xb5, 0x73, 0x14));
    p.drawRect(QRectF(body.left() + 22, body.top(), 10, body.height())); // Polarity band.
    p.setBrush(kTerminal);
    p.drawRect(QRectF(body.left(), body.top() + 6, 12, body.height() - 12));
    p.drawRect(QRectF(body.right() - 12, body.top() + 6, 12, body.height() - 12));
}
