#include "GroupDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QSpinBox>

#include "SizeComboBox.h"

namespace {

QSpinBox *makeSpin(int min, int max, int value)
{
    auto *spin = new QSpinBox;
    spin->setRange(min, max);
    spin->setValue(value);
    return spin;
}

QDoubleSpinBox *makePositionSpin(double value)
{
    auto *spin = new QDoubleSpinBox;
    spin->setRange(0, 999);
    spin->setDecimals(2);
    spin->setSingleStep(1);
    spin->setValue(value);
    return spin;
}

} // namespace

GroupDialog::GroupDialog(const DrawerGroup &group, PlacementCheck isFree,
                         PositionSuggester suggest, QWidget *parent)
    : QDialog(parent)
    , m_group(group)
    , m_isFree(std::move(isFree))
    , m_name(new QLineEdit(group.name))
    , m_rows(makeSpin(1, 20, group.rows))
    , m_cols(makeSpin(1, 20, group.cols))
    , m_posX(makePositionSpin(group.posX))
    , m_posY(makePositionSpin(group.posY))
    , m_suggest(std::move(suggest))
    , m_size(new SizeComboBox(group.defaultSize))
    , m_colorButton(new QPushButton)
    , m_color(group.color)
{
    const bool editing = group.id != 0;
    setWindowTitle(editing ? tr("Edit drawer group") : tr("New drawer group"));

    m_size->setToolTip(tr("Size applied to drawers created from now on"));
    if (editing) {
        m_rows->setToolTip(tr("Changing the size adds or removes drawers; neighbors make room"));
        m_cols->setToolTip(m_rows->toolTip());
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto *form = new QFormLayout(this);
    form->addRow(tr("Name"), m_name);
    form->addRow(tr("Rows"), m_rows);
    form->addRow(tr("Columns"), m_cols);
    form->addRow(tr("Position X"), m_posX);
    form->addRow(tr("Position Y"), m_posY);
    form->addRow(tr("Default size"), m_size);

    auto *reset = new QPushButton(tr("By size"));
    reset->setToolTip(tr("Color the drawers by their size instead"));
    auto *colorRow = new QHBoxLayout;
    colorRow->addWidget(m_colorButton, 1);
    colorRow->addWidget(reset);
    form->addRow(tr("Color"), colorRow);
    showColor();
    connect(m_colorButton, &QPushButton::clicked, this, &GroupDialog::chooseColor);
    connect(reset, &QPushButton::clicked, this, [this] {
        m_color.clear();
        showColor();
    });
    form->addRow(buttons);

    // New groups follow the suggested position until the user picks one by hand.
    m_autoPosition = !editing && m_suggest;
    if (m_autoPosition) {
        suggestPosition();
        for (QSpinBox *spin : {m_rows, m_cols})
            connect(spin, &QSpinBox::valueChanged, this, [this] {
                if (m_autoPosition)
                    suggestPosition();
            });
        for (QDoubleSpinBox *spin : {m_posX, m_posY})
            connect(spin, &QDoubleSpinBox::valueChanged, this, [this] {
                if (!m_updatingPosition)
                    m_autoPosition = false;
            });
    }

    connect(buttons, &QDialogButtonBox::accepted, this, &GroupDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void GroupDialog::chooseColor()
{
    const QColor chosen = QColorDialog::getColor(QColor(m_color), this, tr("Group color"));
    if (!chosen.isValid())
        return;
    m_color = chosen.name();
    showColor();
}

void GroupDialog::showColor()
{
    const QColor color(m_color);
    if (color.isValid()) {
        m_colorButton->setText(m_color);
        m_colorButton->setStyleSheet(QString("background-color: %1; color: %2;")
                                         .arg(m_color, color.lightness() > 140 ? "black" : "white"));
    } else {
        m_colorButton->setText(tr("Colored by drawer size"));
        m_colorButton->setStyleSheet(QString());
    }
}

void GroupDialog::suggestPosition()
{
    const Layout::Position pos = m_suggest(m_rows->value(), m_cols->value());
    m_updatingPosition = true;
    m_posX->setValue(pos.x);
    m_posY->setValue(pos.y);
    m_updatingPosition = false;
}

DrawerGroup GroupDialog::group() const
{
    DrawerGroup result = m_group;
    result.name = m_name->text().trimmed();
    result.rows = m_rows->value();
    result.cols = m_cols->value();
    result.posX = m_posX->value();
    result.posY = m_posY->value();
    result.defaultSize = m_size->size();
    result.color = m_color;
    return result;
}

void GroupDialog::accept()
{
    if (m_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Name is required."));
        return;
    }
    if (!m_isFree(group())) {
        QMessageBox::warning(this, windowTitle(), tr("The group overlaps another group."));
        return;
    }
    QDialog::accept();
}
