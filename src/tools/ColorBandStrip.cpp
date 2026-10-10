#include "ColorBandStrip.h"

#include <QPainter>

using ResistorCode::Color;

ColorBandStrip::ColorBandStrip(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(kDesignHeight);
}

QColor ColorBandStrip::qcolor(Color color)
{
    switch (color) {
    case Color::Black: return QColor(0x10, 0x10, 0x10);
    case Color::Brown: return QColor(0x7b, 0x3f, 0x00);
    case Color::Red: return QColor(0xd3, 0x20, 0x20);
    case Color::Orange: return QColor(0xff, 0x8c, 0x00);
    case Color::Yellow: return QColor(0xf5, 0xd5, 0x00);
    case Color::Green: return QColor(0x2e, 0x9e, 0x3e);
    case Color::Blue: return QColor(0x1f, 0x4e, 0xd8);
    case Color::Violet: return QColor(0x80, 0x3a, 0xb8);
    case Color::Grey: return QColor(0x80, 0x80, 0x80);
    case Color::White: return QColor(0xf8, 0xf8, 0xf8);
    case Color::Gold: return QColor(0xc9, 0xa2, 0x27);
    case Color::Silver: return QColor(0xc0, 0xc0, 0xc8);
    case Color::None: return QColor(0xd9, 0xc3, 0x9a); // Body color.
    }
    return Qt::white;
}

void ColorBandStrip::setBands(const QList<Color> &bands)
{
    m_bands = bands;
    update();
}

void ColorBandStrip::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Keep the design size, centered; shrink proportionally only if it does not fit.
    const double scale = std::min({1.0, double(width()) / kDesignWidth,
                                   double(height()) / kDesignHeight});
    p.translate(width() / 2.0, height() / 2.0);
    p.scale(scale, scale);
    p.translate(-kDesignWidth / 2.0, -kDesignHeight / 2.0);

    const QRect body(30, kDesignHeight / 2 - 22, kDesignWidth - 60, 44);
    p.setPen(QPen(Qt::darkGray, 3));
    // Leads end with the design width, so the whole part keeps one size.
    p.drawLine(QPointF(0, kDesignHeight / 2.0), QPointF(body.left(), kDesignHeight / 2.0));
    p.drawLine(QPointF(body.right(), kDesignHeight / 2.0),
               QPointF(kDesignWidth, kDesignHeight / 2.0));

    p.setPen(QPen(Qt::black, 1));
    p.setBrush(qcolor(Color::None));
    p.drawRoundedRect(body, 14, 14);

    // Tolerance and tempco bands sit apart from the value bands, as on real parts.
    const int count = m_bands.size();
    const int bandWidth = 14;
    for (int i = 0; i < count; ++i) {
        const bool separated = i >= count - (count == 6 ? 2 : 1);
        const double slot = double(body.width() - 40) / std::max(1, count);
        const int x = body.left() + 20 + int(slot * i) + (separated ? 12 : 0);
        // "None" tolerance means no band is printed.
        if (m_bands[i] == Color::None)
            continue;
        p.setPen(Qt::NoPen);
        p.setBrush(qcolor(m_bands[i]));
        p.drawRect(x, body.top(), bandWidth, body.height());
    }
}
