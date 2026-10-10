#include "ToolRegistry.h"

#include <QObject>

#include "CapacitorWidget.h"
#include "FormulaWidgets.h"
#include "ResistorWidget.h"

ToolRegistry ToolRegistry::withDefaultTools()
{
    ToolRegistry registry;
    registry.add(std::make_unique<WidgetTool<ResistorWidget>>(QObject::tr("Resistor colors")));
    registry.add(std::make_unique<WidgetTool<CapacitorWidget>>(QObject::tr("Ceramic capacitors")));
    registry.add(std::make_unique<WidgetTool<SmdWidget>>(QObject::tr("SMD resistors")));
    registry.add(std::make_unique<WidgetTool<OhmWidget>>(QObject::tr("Ohm's law")));
    registry.add(std::make_unique<WidgetTool<DividerWidget>>(QObject::tr("Voltage divider")));
    registry.add(std::make_unique<WidgetTool<LedWidget>>(QObject::tr("LED resistor")));
    return registry;
}
