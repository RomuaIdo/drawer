#pragma once

#include <memory>
#include <vector>

#include "Tool.h"

class ToolRegistry
{
public:
    // Registry pre-filled with every built-in tool.
    static ToolRegistry withDefaultTools();

    void add(std::unique_ptr<ITool> tool) { m_tools.push_back(std::move(tool)); }
    const std::vector<std::unique_ptr<ITool>> &tools() const { return m_tools; }

private:
    std::vector<std::unique_ptr<ITool>> m_tools;
};
