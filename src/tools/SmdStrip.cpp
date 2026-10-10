#include "SmdStrip.h"

#include <QPainter>

SmdStrip::SmdStrip(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(90);
}

void SmdStrip::setMarking(const QString &marking)
{
    m_marking = marking;
    update();
}

void SmdStrip::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 2:1 chip that always fits inside the widget.
    const double chipHeight = std::min(height() * 0.8, std::min(width() * 0.6, 220.0) * 0.5);
    const double chipWidth = chipHeight * 2;
    const QRectF chip((width() - chipWidth) / 2, (height() - chipHeight) / 2, chipWidth, chipHeight);
    const double terminalWidth = chipWidth * 0.14;

    // Black body, then silver terminals wrapping both ends.
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x1c, 0x1c, 0x20));
    p.drawRoundedRect(chip, 3, 3);

    p.setBrush(QColor(0xc4, 0xc6, 0xcc));
    p.drawRect(QRectF(chip.left(), chip.top(), terminalWidth, chip.height()));
    p.drawRect(QRectF(chip.right() - terminalWidth, chip.top(), terminalWidth, chip.height()));

    if (m_marking.isEmpty())
        return;

    // Shrink the text until the marking fits between the terminals.
    const QRectF printable = chip.adjusted(terminalWidth, 0, -terminalWidth, 0);
    QFont font = this->font();
    font.setPixelSize(int(chip.height() * 0.5));
    while (font.pixelSize() > 8
           && QFontMetricsF(font).horizontalAdvance(m_marking) > printable.width() * 0.9)
        font.setPixelSize(font.pixelSize() - 1);
    p.setFont(font);
    p.setPen(QColor(0xf0, 0xf0, 0xf0));
    p.drawText(printable, Qt::AlignCenter, m_marking);
}
