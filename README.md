# Drawer

Desktop app to organize electronic components and other objects in drawer cabinets.

- Define cabinets, drawer blocks (e.g. 4x4, 3x3) or single drawers, and drawer sizes.
- Map items to drawers and export the mapping to CSV.
- Built-in electronics tools (resistor color code, capacitor markings and values, SMD resistor code including EIA-96, Ohm's law, voltage divider, LED resistor, and a metric screw reference: nominal diameter, tap drill and clearance holes for cylindrical and countersunk heads for M2 to M4, with a labeled drawing of each measure).

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
- The - and + buttons below the item list take or add one to the selected items (several rows can be selected). An item can have an optional minimum stock. It applies to the item as a whole: entries with the same name, category, value and package (ignoring case) in the same cabinet are one item, and their quantities are added up and compared with the minimum. At or below it, the rows are tinted and their drawers get a red badge on the cabinet view. An item stored in several drawers shows its quantity as `this drawer/total` (e.g. `4/10`); hovering over it lists the drawers (this drawer first, then up to five others as `Group/R1C1 (4)`).
- Drag items from the list onto any drawer of the cabinet to move them; hold Ctrl while dropping to copy instead. Ctrl+C copies the selected items (or use the right click menu), and Ctrl+V or the right click menu on a drawer pastes them there as new items.
- The Edit menu and the usual shortcuts work: Ctrl+Z undoes and Ctrl+Y (or Ctrl+Shift+Z) redoes any change to cabinets, blocks, drawers and items, one user action at a time; history lasts until the app closes. Ctrl+X cuts the selected items (pasting then moves them), Ctrl+C copies, Ctrl+V pastes into the selected drawer, and Ctrl+A selects all items when the list has the focus, otherwise all blocks.
- The Delete key (or Edit > Delete) removes the selected items when the list has the focus, otherwise the selected drawer, which leaves a hole in its block (the drawer's items are deleted with it, after a confirmation). Deleting the last drawer of a block removes the block. All of it can be undone.
- The items panel has two tabs. "Selected drawer" lists the items of the chosen drawer. "All items" lists every item of the cabinet with its location; picking a row selects its drawer and shows it darker (and scrolls to it). The tab has its own Add, Edit and Remove buttons and - / + buttons for the selected entry (the - / + buttons are off while duplicates are grouped). "Group duplicates" (off by default) puts an item stored in several drawers on one row, with its drawers as `R1C1 (10), R2C3 (4)` and the total quantity, and darkens all of those drawers. Editing such a grouped row changes the common fields of every entry and shows a table with each drawer's quantity so each one can be changed; Remove deletes every entry of the item.
- The Pick list and Restock list tabs hold working lists. Right click items (in the Selected drawer or All items tabs) and choose Add to pick list or Add to restock list; adding the same entry again raises its quantity by one, and the quantity can be edited by double clicking it. Pick list shows what is in stock and what will be left, and tints lines asking for more than the drawer holds. "Take all from stock" subtracts every line from the stock in one go (never below zero) and empties the list; "Restock all" does the same in the other direction. Selecting a line darkens its drawer. Both can be undone. The Restock list also has an Add button for components that are not in the database yet: it creates the item with no stock and puts it on the list with the quantity to add. In every Add and Edit item dialog the drawer is chosen by clicking it on the cabinet already open on the left of the main window; the dialog does not block the window, so the cabinet stays clickable, and it shows the chosen drawer.
- The search box darkens every drawer holding a matching item. Text matches name, category, value, package and notes; typing a value such as `1uF` also finds items whose value is written `1u`, `1000nF` or `1µF`. The category list next to it keeps only items of one category, both for the darkened drawers (on its own it darkens every drawer with that category) and for the All items tab.
- File > Export mapping writes the full mapping, including empty drawers, to a CSV file.
- File > Export database saves cabinets, blocks, drawers and items to a JSON file; File > Import database adds the cabinets from such a file to the current data (existing cabinets are kept, and an invalid file changes nothing).
- Tools > Electronics tools opens the calculators.

Data is stored in `drawer.db` under the application data directory (`~/.local/share/Drawer` on Linux).
