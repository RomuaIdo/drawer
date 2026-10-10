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
- Select several blocks with Ctrl+click or by dragging a rectangle on empty space  (Ctrl+A selects all, Esc clears), then drag any of them to move the whole selection together.
- Edit a block to give it a custom color (the name tag follows it, darker); "By size" goes back to green, yellow and red for small, medium and large drawers.
- Drag a block to move it; it snaps to the grid and turns red where it would overlap. Right click a block and enable "Move freely" to drop it anywhere. Esc cancels a drag.
- The - and + buttons below the item list take or add one to the selected items (several rows can be selected). An item can have an optional minimum stock. It applies to the item as a whole: entries with the same name, category, value and package (ignoring case) in the same cabinet are one item, and their quantities are added up and compared with the minimum. At or below it, the rows are tinted and their drawers get a red badge on the cabinet view. Hovering over a quantity shows `drawer/total` and where else the item is stored (this drawer first, then up to five others as `Group/R1C1`).
- Drag items from the list onto any drawer of the cabinet to move them; hold Ctrl while dropping to copy instead. Ctrl+C copies the selected items (or use the right click menu), and Ctrl+V or the right click menu on a drawer pastes them there as new items.
- The Edit menu and the usual shortcuts work: Ctrl+Z undoes and Ctrl+Y (or Ctrl+Shift+Z) redoes any change to cabinets, blocks, drawers and items, one user action at a time; history lasts until the app closes. Ctrl+X cuts the selected items (pasting then moves them), Ctrl+C copies, Ctrl+V pastes into the selected drawer, and Ctrl+A selects all items when the list has the focus, otherwise all blocks.
- The Delete key (or Edit > Delete) removes the selected items when the list has the focus, otherwise the selected drawer, which leaves a hole in its block (the drawer's items are deleted with it, after a confirmation). Deleting the last drawer of a block removes the block. All of it can be undone.
- The search box highlights every drawer holding a matching item.
- File > Export mapping writes the full mapping, including empty drawers, to a CSV file.
- File > Export organization saves cabinets, blocks, drawers and items to a JSON file; File > Import organization adds the cabinets from such a file to the current data (existing cabinets are kept, and an invalid file changes nothing).
- Tools > Electronics tools opens the calculators.

Data is stored in `drawer.db` under the application data directory (`~/.local/share/Drawer` on Linux).
