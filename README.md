# Drawer

Desktop app to organize electronic components and other objects in drawer cabinets.

- Define cabinets, drawer blocks (e.g. 4x4, 3x3) or single drawers, and drawer sizes.
- Map items to drawers and export the mapping to CSV.
- Built-in electronics tools (resistor color code, ceramic capacitor code, SMD resistor code, Ohm's law, voltage divider, LED resistor).

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
- The search box highlights every drawer holding a matching item.
- File > Export writes the full mapping, including empty drawers, to a CSV file.
- Tools > Electronics tools opens the calculators.

Data is stored in `drawer.db` under the application data directory (`~/.local/share/Drawer` on Linux).
