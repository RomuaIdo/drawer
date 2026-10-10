#pragma once

#include <QString>
#include <QWidget>

enum class ToolCategory {
    ComponentReading, // Decode or encode component markings.
    Auxiliary,        // General calculators.
};

// An electronics utility shown as a tab in the tools dialog.
class ITool
{
public:
    virtual ~ITool() = default;
    virtual QString name() const = 0;
    virtual ToolCategory category() const = 0;
    virtual QWidget *createWidget(QWidget *parent) const = 0;
};

// Adapts any QWidget subclass with a (QWidget *parent) constructor into a tool.
template <typename Widget>
class WidgetTool : public ITool
{
public:
    WidgetTool(QString name, ToolCategory category)
        : m_name(std::move(name)), m_category(category) {}

    QString name() const override { return m_name; }
    ToolCategory category() const override { return m_category; }
    QWidget *createWidget(QWidget *parent) const override { return new Widget(parent); }

private:
    QString m_name;
    ToolCategory m_category;
};
