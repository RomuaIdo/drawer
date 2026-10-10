#pragma once

#include <QWidget>

class CapacitorStrip;
class QComboBox;
class QLabel;
class QLineEdit;

// Capacitor calculator: type a marking (104, 104K) or a value (100n, 4u7) and get the other
// form, the tolerance and a drawing of the usual kind of capacitor for that value.
class CapacitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CapacitorWidget(QWidget *parent = nullptr);

private:
    void refresh();

    CapacitorStrip *m_strip;
    QLineEdit *m_input;
    QLineEdit *m_voltage;
    QComboBox *m_type;
    QLabel *m_result;
};
