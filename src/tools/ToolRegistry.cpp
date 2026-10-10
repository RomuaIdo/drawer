#include "ToolRegistry.h"

#include <QObject>

#include "CapacitorWidget.h"
#include "ResistorWidget.h"

ToolRegistry ToolRegistry::withDefaultTools()
{
    ToolRegistry registry;
    registry.add(std::make_unique<WidgetTool<ResistorWidget>>(QObject::tr("Resistor colors")));
    registry.add(std::make_unique<WidgetTool<CapacitorWidget>>(QObject::tr("Ceramic capacitors")));
    return registry;
}
