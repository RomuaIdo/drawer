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
    void setType(ResistorCode::ResistorType type);
    ResistorCode::ResistorType type() const { return m_type; }
    QSize sizeHint() const override { return {kDesignWidth, kDesignHeight}; }

    static QColor qcolor(ResistorCode::Color color);

    // Fixed drawing size; it only shrinks when the widget is smaller than this.
    static constexpr int kDesignWidth = 320;
    static constexpr int kDesignHeight = 70;

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QList<ResistorCode::Color> m_bands;
    ResistorCode::ResistorType m_type = ResistorCode::ResistorType::CarbonFilm;
};
