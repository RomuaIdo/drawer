#pragma once

#include <QWidget>

class QComboBox;
class QLabel;

// Metric screw reference: nominal diameter, tap drill and clearance holes for the heads.
class ScrewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScrewWidget(QWidget *parent = nullptr);

private:
    void showScrew();

    QComboBox *m_size;
    QLabel *m_result;
};
