#include "ResistorWidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QVBoxLayout>

#include "ColorBandStrip.h"
#include "Engineering.h"
#include "ResistorCode.h"
#include "ResultLabel.h"

using namespace ResistorCode;

namespace {

QIcon swatch(Color color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(ColorBandStrip::qcolor(color));
    return QIcon(pixmap);
}

void fillColors(QComboBox *combo, const QList<Color> &colors, bool showTolerance = false)
{
    for (Color c : colors) {
        QString text = name(c);
        if (showTolerance)
            text += QString(" (±%1%)").arg(*toleranceOf(c));
        combo->addItem(swatch(c), text, int(c));
    }
}

} // namespace

ResistorWidget::ResistorWidget(QWidget *parent)
    : QWidget(parent)
    , m_bandCount(new QComboBox)
    , m_selectorsHost(new QWidget)
    , m_strip(new ColorBandStrip)
    , m_result(makeResultLabel())
    , m_valueInput(new QLineEdit)
    , m_toleranceInput(new QComboBox)
    , m_encodeStatus(makeResultLabel())
{
    for (int bands : {4, 5, 6})
        m_bandCount->addItem(tr("%1 bands").arg(bands), bands);

    m_valueInput->setPlaceholderText(tr("e.g. 4.7k, 4k7, 330"));
    for (Color c : toleranceColors())
        m_toleranceInput->addItem(QString("±%1%").arg(*toleranceOf(c)), int(c));
    m_toleranceInput->setCurrentIndex(m_toleranceInput->findData(int(Color::Gold)));

    auto *decodeBox = new QGroupBox(tr("Bands to value"));
    auto *decodeLayout = new QVBoxLayout(decodeBox);
    auto *countRow = new QFormLayout;
    countRow->addRow(tr("Bands"), m_bandCount);
    decodeLayout->addLayout(countRow);
    decodeLayout->addWidget(m_selectorsHost);
    decodeLayout->addWidget(m_result);

    auto *encodeBox = new QGroupBox(tr("Value to bands"));
    auto *encodeLayout = new QFormLayout(encodeBox);
    encodeLayout->addRow(tr("Resistance (ohms)"), m_valueInput);
    encodeLayout->addRow(tr("Tolerance"), m_toleranceInput);
    encodeLayout->addRow(m_encodeStatus);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_strip);
    layout->addWidget(decodeBox);
    layout->addWidget(encodeBox);
    layout->addStretch();

    connect(m_bandCount, &QComboBox::currentIndexChanged, this, &ResistorWidget::rebuildBandSelectors);
    connect(m_valueInput, &QLineEdit::textChanged, this, &ResistorWidget::encodeValue);
    connect(m_toleranceInput, &QComboBox::currentIndexChanged, this, &ResistorWidget::encodeValue);

    m_bandCount->setCurrentIndex(0);
    rebuildBandSelectors();
}

void ResistorWidget::rebuildBandSelectors()
{
    // Dropping the old layout's widgets before installing a new layout.
    delete m_selectorsHost->layout();
    qDeleteAll(m_selectorsHost->findChildren<QWidget *>(Qt::FindDirectChildrenOnly));
    m_selectors.clear();

    const int count = m_bandCount->currentData().toInt();
    const int digits = count == 4 ? 2 : 3;

    auto *form = new QFormLayout(m_selectorsHost);
    form->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < count; ++i) {
        auto *combo = new QComboBox;
        QString label;
        if (i < digits) {
            fillColors(combo, digitColors());
            label = tr("Digit %1").arg(i + 1);
        } else if (i == digits) {
            fillColors(combo, multiplierColors());
            label = tr("Multiplier");
        } else if (i == digits + 1) {
            fillColors(combo, toleranceColors(), true);
            combo->setCurrentIndex(combo->findData(int(Color::Gold)));
            label = tr("Tolerance");
        } else {
            fillColors(combo, tempcoColors());
            combo->setCurrentIndex(combo->findData(int(Color::Brown)));
            label = tr("Temp. coefficient");
        }
        if (i == 0)
            combo->setCurrentIndex(combo->findData(int(Color::Brown)));
        form->addRow(label, combo);
        m_selectors.append(combo);
        connect(combo, &QComboBox::currentIndexChanged, this, &ResistorWidget::decodeBands);
    }
    decodeBands();
}

void ResistorWidget::decodeBands()
{
    QList<Color> bands;
    for (QComboBox *combo : m_selectors)
        bands.append(static_cast<Color>(combo->currentData().toInt()));
    m_strip->setBands(bands);

    const auto value = decode(bands);
    if (!value) {
        m_result->setText(tr("Invalid combination"));
        return;
    }
    QString text = QString("%1  ±%2%")
                       .arg(Engineering::format(value->ohms, QString::fromUtf8("Ω")))
                       .arg(value->tolerancePercent);
    if (value->tempcoPpm)
        text += QString("  %1 ppm/K").arg(*value->tempcoPpm);
    m_result->setText(text);
}

void ResistorWidget::encodeValue()
{
    const QString input = m_valueInput->text().trimmed();
    if (input.isEmpty()) {
        m_encodeStatus->clear();
        return;
    }
    const auto ohms = Engineering::parse(input);
    if (!ohms) {
        m_encodeStatus->setText(tr("Not a valid value"));
        return;
    }
    const Color toleranceColor = static_cast<Color>(m_toleranceInput->currentData().toInt());
    const int count = m_bandCount->currentData().toInt();
    const auto bands = encode(*ohms, *toleranceOf(toleranceColor), count);
    if (!bands) {
        m_encodeStatus->setText(tr("Not representable with %1 bands").arg(count));
        return;
    }
    m_encodeStatus->setText(tr("Bands set from value"));
    for (int i = 0; i < m_selectors.size(); ++i)
        m_selectors[i]->setCurrentIndex(m_selectors[i]->findData(int((*bands)[i])));
}
