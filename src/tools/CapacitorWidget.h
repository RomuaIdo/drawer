#pragma once

#include <QWidget>

class CapacitorStrip;
class QLabel;
class QLineEdit;

// Ceramic capacitor marking calculator: code to value and value to code.
class CapacitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CapacitorWidget(QWidget *parent = nullptr);

private:
    void decodeMarking();
    void encodeValue();

    CapacitorStrip *m_strip;
    QLineEdit *m_markingInput;
    QLabel *m_decoded;
    QLineEdit *m_valueInput;
    QLabel *m_encoded;
};
