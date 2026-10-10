#pragma once

#include <QWidget>

// Draws a minimal ceramic disc capacitor: an orange body and two pins.
class CapacitorStrip : public QWidget
{
    Q_OBJECT

public:
    explicit CapacitorStrip(QWidget *parent = nullptr);

    QSize sizeHint() const override { return {320, 110}; }

protected:
    void paintEvent(QPaintEvent *) override;
};
