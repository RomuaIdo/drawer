#pragma once

#include <QWidget>

// Fixed schematic that names every measure listed by the screw tool: the thread, and a
// section through a cylindrical and a conical (countersunk) head in their holes.
class ScrewDiagram : public QWidget
{
    Q_OBJECT

public:
    explicit ScrewDiagram(QWidget *parent = nullptr);

    QSize sizeHint() const override { return {kThreadWidth + 2 * kHeadWidth, kHeight}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    static constexpr int kThreadWidth = 300;
    static constexpr int kHeadWidth = 250;
    static constexpr int kHeight = 235;
};
