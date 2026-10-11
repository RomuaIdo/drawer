#include "ToolRegistry.h"

#include <QObject>

#include "CapacitorWidget.h"
#include "FormulaWidgets.h"
#include "ResistorWidget.h"
#include "ScrewWidget.h"

ToolRegistry ToolRegistry::withDefaultTools()
{
    ToolRegistry registry;
    registry.add(std::make_unique<WidgetTool<ResistorWidget>>(QObject::tr("Resistor colors"),
                                                              ToolCategory::ComponentReading));
    registry.add(std::make_unique<WidgetTool<CapacitorWidget>>(QObject::tr("Ceramic capacitors"),
                                                              ToolCategory::ComponentReading));
    registry.add(std::make_unique<WidgetTool<SmdWidget>>(QObject::tr("SMD resistors"),
                                                              ToolCategory::ComponentReading));
    registry.add(std::make_unique<WidgetTool<OhmWidget>>(QObject::tr("Ohm's law"),
                                                           ToolCategory::Auxiliary));
    registry.add(std::make_unique<WidgetTool<DividerWidget>>(QObject::tr("Voltage divider"),
                                                              ToolCategory::Auxiliary));
    registry.add(std::make_unique<WidgetTool<LedWidget>>(QObject::tr("LED resistor"),
                                                              ToolCategory::Auxiliary));
    registry.add(std::make_unique<WidgetTool<ScrewWidget>>(QObject::tr("Metric screws"),
                                                              ToolCategory::Auxiliary));
    return registry;
}

std::vector<const ITool *> ToolRegistry::toolsIn(ToolCategory category) const
{
    std::vector<const ITool *> result;
    for (const auto &tool : m_tools) {
        if (tool->category() == category)
            result.push_back(tool.get());
    }
    return result;
}
