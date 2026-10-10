#pragma once

#include <QWidget>

// Draws a ceramic disc capacitor printed with its marking.
class CapacitorStrip : public QWidget
{
    Q_OBJECT

public:
    explicit CapacitorStrip(QWidget *parent = nullptr);

    // Text printed on the body; an empty text shows a blank capacitor.
    void setMarking(const QString &marking);
    QString marking() const { return m_marking; }
    QSize sizeHint() const override { return {320, 130}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString m_marking;
};
