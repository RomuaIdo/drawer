# Drawer

Desktop app to organize electronic components and other objects in drawer cabinets.

- Define cabinets, drawer blocks (e.g. 4x4, 3x3) or single drawers, and drawer sizes.
- Map items to drawers and export the mapping to CSV.
- Built-in electronics tools (resistor color code, ceramic capacitor code, ...).

## Build

Requires Qt 6 (Widgets, Sql with SQLite driver, Test), CMake 3.21+ and a C++17 compiler.

```
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/drawer
```
