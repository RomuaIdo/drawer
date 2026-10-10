#pragma once

#include <QDialog>

class ToolRegistry;

// Tabbed window with one tab per registered tool.
class ToolsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ToolsDialog(const ToolRegistry &registry, QWidget *parent = nullptr);
};
