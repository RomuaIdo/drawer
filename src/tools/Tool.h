#pragma once

#include <QString>
#include <QWidget>

// An electronics utility shown as a tab in the tools dialog.
class ITool
{
public:
    virtual ~ITool() = default;
    virtual QString name() const = 0;
    virtual QWidget *createWidget(QWidget *parent) const = 0;
};

// Adapts any QWidget subclass with a (QWidget *parent) constructor into a tool.
template <typename Widget>
class WidgetTool : public ITool
{
public:
    explicit WidgetTool(QString name) : m_name(std::move(name)) {}

    QString name() const override { return m_name; }
    QWidget *createWidget(QWidget *parent) const override { return new Widget(parent); }

private:
    QString m_name;
};
