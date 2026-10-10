# Drawer

Desktop app to organize electronic components and other objects in drawer cabinets.

- Define cabinets, drawer blocks (e.g. 4x4, 3x3) or single drawers, and drawer sizes.
- Map items to drawers and export the mapping to CSV.
- Built-in electronics tools (resistor color code, ceramic capacitor code, SMD resistor code including EIA-96, Ohm's law, voltage divider, LED resistor).

## Build

Requires Qt 6 (Widgets, Sql with SQLite driver, Test), CMake 3.21+ and a C++17 compiler.

```
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/drawer
```

## Usage

- Create a cabinet, then add drawer groups: a 4x4 or 3x3 block, or a 1x1 group for a single drawer.
- Double click a drawer to edit its label and size (small, medium, large); click it to manage its items.
- Right click a block and choose Edit to rename it or change its rows and columns; growing a block pushes its neighbors out of the way, and shrinking asks for confirmation if drawers with items would be removed.
- Select several blocks with Ctrl+click or by dragging a rectangle on empty space (Ctrl+A selects all, Esc clears), then drag any of them to move the whole selection together.
- Drag a block to move it; it snaps to the grid and turns red where it would overlap. Right click a block and enable "Move freely" to drop it anywhere. Esc cancels a drag.
- The search box highlights every drawer holding a matching item.
- File > Export mapping writes the full mapping, including empty drawers, to a CSV file.
- File > Export organization saves cabinets, blocks, drawers and items to a JSON file; File > Import organization adds the cabinets from such a file to the current data (existing cabinets are kept, and an invalid file changes nothing).
- Tools > Electronics tools opens the calculators.

Data is stored in `drawer.db` under the application data directory (`~/.local/share/Drawer` on Linux).
