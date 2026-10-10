#pragma once

#include <QWidget>

#include "ResistorCode.h"

// Draws a resistor body with its color bands.
class ColorBandStrip : public QWidget
{
    Q_OBJECT

public:
    explicit ColorBandStrip(QWidget *parent = nullptr);

    void setBands(const QList<ResistorCode::Color> &bands);
    QSize sizeHint() const override { return {320, 70}; }

    static QColor qcolor(ResistorCode::Color color);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QList<ResistorCode::Color> m_bands;
};
