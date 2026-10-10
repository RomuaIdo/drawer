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
    QSize sizeHint() const override { return {kDesignWidth, kDesignHeight}; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;

    static QColor qcolor(ResistorCode::Color color);

    // The drawing is laid out in these units and scaled uniformly to fit.
    static constexpr int kDesignWidth = 320;
    static constexpr int kDesignHeight = 70;

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QList<ResistorCode::Color> m_bands;
};
