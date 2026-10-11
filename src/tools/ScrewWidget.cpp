#include "ScrewWidget.h"

#include <QComboBox>
#include <QFormLayout>

#include "ResultLabel.h"
#include "ScrewDiagram.h"
#include "Screws.h"

namespace {

QString mm(double value)
{
    return QString::number(value, 'g', 3) + " mm";
}

} // namespace

ScrewWidget::ScrewWidget(QWidget *parent)
    : QWidget(parent)
    , m_size(new QComboBox)
    , m_result(makeResultLabel())
{
    for (const Screws::Screw &screw : Screws::all())
        m_size->addItem(screw.name);

    auto *form = new QFormLayout(this);
    form->addRow(tr("Screw"), m_size);
    form->addRow(new ScrewDiagram);
    form->addRow(m_result);
    connect(m_size, &QComboBox::currentTextChanged, this, &ScrewWidget::showScrew);
    showScrew();
}

void ScrewWidget::showScrew()
{
    const auto s = Screws::find(m_size->currentText());
    if (!s)
        return;
    m_result->setText(
        tr("Nominal diameter: %1 (pitch %2)\n"
           "Tap drill (to cut the thread): %3\n"
           "\n"
           "Clearance hole (screw passes through)\n"
           "  Close: %4   Normal: %5   Loose: %6\n"
           "\n"
           "Cylindrical head (socket cap)\n"
           "  Head: %7 x %8 high\n"
           "  Counterbore: %9, %10 deep\n"
           "\n"
           "Conical head (countersunk)\n"
           "  Head: %11\n"
           "  Countersink: %12")
            .arg(mm(s->diameter), mm(s->pitch), mm(s->tapDrill), mm(s->clearanceClose),
                 mm(s->clearanceNormal), mm(s->clearanceLoose))
            .arg(mm(s->cylinderHeadDiameter), mm(s->cylinderHeadHeight), mm(s->counterboreDiameter),
                 mm(s->counterboreDepth), mm(s->coneHeadDiameter), mm(s->countersinkDiameter)));
}
