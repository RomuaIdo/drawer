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
    setMinimumHeight(130);
}

void CapacitorStrip::setType(CapacitorType type)
{
    if (type == m_type)
        return;
    m_type = type;
    update();
}

void CapacitorStrip::setPrinted(const QStringList &lines)
{
    if (lines == m_printed)
        return;
    m_printed = lines;
    update();
}

void CapacitorStrip::setScale(double scale)
{
    if (qFuzzyCompare(scale, m_scale))
        return;
    m_scale = scale;
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

void CapacitorStrip::drawText(QPainter &p, const QRectF &area, const QColor &color) const
{
    if (m_printed.isEmpty())
        return;
    const QString text = m_printed.join('\n');

    QFont font = this->font();
    font.setBold(true);
    font.setPixelSize(int(area.height() / m_printed.size() * 0.7));
    // Shrink until the widest line and all lines fit.
    while (font.pixelSize() > 7) {
        const QRectF needed = QFontMetricsF(font).boundingRect(QRectF(0, 0, 10000, 10000),
                                                               Qt::AlignLeft, text);
        if (needed.width() <= area.width() && needed.height() <= area.height())
            break;
        font.setPixelSize(font.pixelSize() - 1);
    }
    p.save();
    p.setFont(font);
    p.setPen(color);
    p.drawText(area, Qt::AlignCenter, text);
    p.restore();
}

// Flat orange disc with a soft light spot, on two pins; carries the three-digit code.
void CapacitorStrip::drawDisc(QPainter &p, const QPointF &c) const
{
    const double radius = 30 * m_scale;
    const QPointF body(c.x(), c.y() - 14);
    drawPins(p, QPointF(body.x(), body.y() + radius * 0.8), radius * 0.8, 24 + radius * 0.2);

    QRadialGradient gradient(body - QPointF(radius * 0.35, radius * 0.4), radius * 1.6);
    gradient.setColorAt(0.0, QColor(0xf2, 0xa9, 0x4e));
    gradient.setColorAt(1.0, QColor(0xe0, 0x8a, 0x2b));
    p.setPen(QPen(QColor(0xc4, 0x75, 0x20), 1.5));
    p.setBrush(gradient);
    const QRectF rect(body.x() - radius, body.y() - radius, 2 * radius, 2 * radius * 0.95);
    p.drawEllipse(rect);
    drawText(p, rect.adjusted(radius * 0.3, radius * 0.3, -radius * 0.3, -radius * 0.3),
             QColor(0x4a, 0x2a, 0x08));
}

// Small tan block with silver ends; chips are not marked.
void CapacitorStrip::drawChip(QPainter &p, const QPointF &c) const
{
    const double w = 104 * m_scale;
    const double h = 52 * m_scale;
    const QRectF body(c.x() - w / 2, c.y() - h / 2, w, h);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xb9, 0x8b, 0x5e));
    p.drawRoundedRect(body, 3, 3);
    p.setBrush(kTerminal);
    p.drawRect(QRectF(body.left(), body.top(), 18, body.height()));
    p.drawRect(QRectF(body.right() - 18, body.top(), 18, body.height()));
}

// Blue box on two pins; carries the value and the voltage.
void CapacitorStrip::drawFilm(QPainter &p, const QPointF &c) const
{
    const double w = 92 * m_scale;
    const double h = 56 * m_scale;
    const QRectF body(c.x() - w / 2, c.y() - h * 0.62, w, h);
    drawPins(p, QPointF(c.x(), body.bottom() - 2), w * 0.6, 22);

    p.setPen(QPen(QColor(0x2f, 0x6f, 0xa3), 1.5));
    p.setBrush(QColor(0x3f, 0x8a, 0xc4));
    p.drawRoundedRect(body, 6, 6);
    drawText(p, body.adjusted(8, 5, -8, -5), Qt::white);
}

// Dark can with a light stripe marking the negative side; the shorter pin is the negative
// one. Text runs along the can, as printed on real parts.
void CapacitorStrip::drawElectrolytic(QPainter &p, const QPointF &c) const
{
    const double w = 56 * m_scale;
    const double h = 66 * m_scale;
    const QRectF body(c.x() - w / 2, c.y() - h * 0.6, w, h);
    drawPins(p, QPointF(c.x(), body.bottom() - 2), w * 0.4, 22, 8);

    p.setPen(QPen(QColor(0x14, 0x2b, 0x52), 1.5));
    p.setBrush(QColor(0x1f, 0x3d, 0x73));
    p.drawRoundedRect(body, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xcf, 0xd8, 0xe6));
    const QRectF stripe(body.left() + 4, body.top() + 8, 10, body.height() - 16);
    p.drawRect(stripe);
    p.setBrush(QColor(255, 255, 255, 40));
    p.drawEllipse(QRectF(body.left() + w * 0.32, body.top() + 3, w * 0.5, 7)); // Top rim.

    // Vertical text on the free part of the can.
    const QRectF textArea(stripe.right() + 4, body.top() + 12, body.right() - stripe.right() - 8,
                          body.height() - 24);
    p.save();
    p.translate(textArea.center());
    p.rotate(-90);
    drawText(p, QRectF(-textArea.height() / 2, -textArea.width() / 2, textArea.height(),
                       textArea.width()),
             Qt::white);
    p.restore();
}

// Orange molded block, darker band at the positive end, silver terminals; carries the value
// and the voltage.
void CapacitorStrip::drawTantalum(QPainter &p, const QPointF &c) const
{
    const double w = 104 * m_scale;
    const double h = 56 * m_scale;
    const QRectF body(c.x() - w / 2, c.y() - h / 2, w, h);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xe6, 0xa1, 0x34));
    p.drawRoundedRect(body, 4, 4);
    p.setBrush(QColor(0xb5, 0x73, 0x14));
    p.drawRect(QRectF(body.left() + 22, body.top(), 10, body.height())); // Polarity band.
    p.setBrush(kTerminal);
    p.drawRect(QRectF(body.left(), body.top() + 6, 12, body.height() - 12));
    p.drawRect(QRectF(body.right() - 12, body.top() + 6, 12, body.height() - 12));
    drawText(p, QRectF(body.left() + 36, body.top() + 4, body.width() - 52, body.height() - 8),
             QColor(0x4a, 0x2a, 0x08));
}
