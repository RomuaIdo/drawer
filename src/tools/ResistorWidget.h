#pragma once

#include <QWidget>

class ColorBandStrip;
class QComboBox;
class QLabel;
class QLineEdit;

// Resistor color code calculator: bands to value and value to bands.
class ResistorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ResistorWidget(QWidget *parent = nullptr);

private:
    void rebuildBandSelectors();
    void decodeBands();
    void encodeValue();

    QComboBox *m_bandCount;
    QWidget *m_selectorsHost;
    QList<QComboBox *> m_selectors;
    ColorBandStrip *m_strip;
    QLabel *m_result;
    QLineEdit *m_valueInput;
    QComboBox *m_toleranceInput;
    QLabel *m_encodeStatus;
};
