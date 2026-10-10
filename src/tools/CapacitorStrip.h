#pragma once

#include <QStringList>
#include <QWidget>

#include "CapacitorCode.h"

// Draws a capacitor of the chosen type in a minimal style: flat color, pins or terminals,
// and the text printed on the part where that type carries any.
class CapacitorStrip : public QWidget
{
    Q_OBJECT

public:
    explicit CapacitorStrip(QWidget *parent = nullptr);

    void setType(CapacitorCode::CapacitorType type);
    CapacitorCode::CapacitorType type() const { return m_type; }
    // Text printed on the body, one entry per line; empty for an unmarked part.
    void setPrinted(const QStringList &lines);
    QStringList printed() const { return m_printed; }
    // Relative size of the body (see CapacitorCode::voltageScale).
    void setScale(double scale);
    double scale() const { return m_scale; }
    QSize sizeHint() const override { return {320, 130}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void drawDisc(QPainter &p, const QPointF &center) const;
    void drawChip(QPainter &p, const QPointF &center) const;
    void drawFilm(QPainter &p, const QPointF &center) const;
    void drawElectrolytic(QPainter &p, const QPointF &center) const;
    void drawTantalum(QPainter &p, const QPointF &center) const;
    // Prints the lines centered in `area`, shrinking the font until they fit.
    void drawText(QPainter &p, const QRectF &area, const QColor &color) const;

    CapacitorCode::CapacitorType m_type = CapacitorCode::CapacitorType::CeramicDisc;
    QStringList m_printed;
    double m_scale = 1.0;
};
