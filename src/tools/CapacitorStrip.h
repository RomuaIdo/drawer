#pragma once

#include <QWidget>

#include "CapacitorCode.h"

// Draws a capacitor of the chosen type in a minimal style: flat color and pins or terminals.
class CapacitorStrip : public QWidget
{
    Q_OBJECT

public:
    explicit CapacitorStrip(QWidget *parent = nullptr);

    void setType(CapacitorCode::CapacitorType type);
    CapacitorCode::CapacitorType type() const { return m_type; }
    QSize sizeHint() const override { return {320, 110}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void drawDisc(QPainter &p, const QPointF &center) const;
    void drawChip(QPainter &p, const QPointF &center) const;
    void drawFilm(QPainter &p, const QPointF &center) const;
    void drawElectrolytic(QPainter &p, const QPointF &center) const;
    void drawTantalum(QPainter &p, const QPointF &center) const;

    CapacitorCode::CapacitorType m_type = CapacitorCode::CapacitorType::CeramicDisc;
};
