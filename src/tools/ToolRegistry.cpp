#include "ToolRegistry.h"

#include <QObject>

#include "ResistorWidget.h"

ToolRegistry ToolRegistry::withDefaultTools()
{
    ToolRegistry registry;
    registry.add(std::make_unique<WidgetTool<ResistorWidget>>(QObject::tr("Resistor colors")));
    return registry;
}
