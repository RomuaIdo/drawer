#include "ToolsDialog.h"

#include <QTabWidget>
#include <QVBoxLayout>

#include "ToolRegistry.h"

ToolsDialog::ToolsDialog(const ToolRegistry &registry, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Electronics tools"));

    auto *tabs = new QTabWidget;
    for (const auto &tool : registry.tools())
        tabs->addTab(tool->createWidget(tabs), tool->name());

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
}
