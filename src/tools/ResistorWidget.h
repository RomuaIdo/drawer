#pragma once

#include <QWidget>
#include <optional>

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
    struct Value {
        double ohms;
        double tolerancePercent;
        int tempcoPpm;
    };

    void rebuildBandSelectors();
    void onBandEdited();
    void onTypeChanged();
    void decodeBands();
    void encodeValue();
    // The value to keep when the band count changes: the typed one, else what the bands say.
    std::optional<Value> currentValue() const;
    // Sets the selectors to the bands for `value`; false if the band count cannot express it.
    bool applyValue(const Value &value);

    QComboBox *m_bandCount;
    QComboBox *m_type;
    QWidget *m_selectorsHost;
    QList<QComboBox *> m_selectors;
    ColorBandStrip *m_strip;
    QLabel *m_result;
    QLineEdit *m_valueInput;
    QComboBox *m_toleranceInput;
    QLabel *m_encodeStatus;
    bool m_applyingBands = false; // True while selectors are set from code, not by the user.
};
