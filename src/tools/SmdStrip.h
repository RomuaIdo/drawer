#pragma once

#include <QWidget>

// Draws an SMD resistor chip printed with its marking.
class SmdStrip : public QWidget
{
    Q_OBJECT

public:
    explicit SmdStrip(QWidget *parent = nullptr);

    // Text printed on the chip; empty leaves it blank.
    void setMarking(const QString &marking);
    QString marking() const { return m_marking; }
    QSize sizeHint() const override { return {320, 90}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString m_marking;
};
