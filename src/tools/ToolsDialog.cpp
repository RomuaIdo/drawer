#include "ToolsDialog.h"

#include <QTabWidget>
#include <QVBoxLayout>

#include "ToolRegistry.h"

ToolsDialog::ToolsDialog(const ToolRegistry &registry, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Electronics tools"));

    // One top-level tab per category, each holding a tab per tool.
    auto *categories = new QTabWidget;
    const std::pair<ToolCategory, QString> groups[] = {
        {ToolCategory::ComponentReading, tr("Component reading")},
        {ToolCategory::Auxiliary, tr("Auxiliary")},
    };
    for (const auto &[category, title] : groups) {
        auto *tabs = new QTabWidget;
        for (const ITool *tool : registry.toolsIn(category))
            tabs->addTab(tool->createWidget(tabs), tool->name());
        categories->addTab(tabs, title);
    }

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(categories);
}
