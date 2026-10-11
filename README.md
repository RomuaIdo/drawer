# Drawer

Desktop app to organize electronic components and other objects in drawer cabinets.

- Define cabinets, drawer blocks (e.g. 4x4, 3x3) or single drawers, and drawer sizes.
- Map items to drawers, search them and export the mapping to CSV.
- Keep pick and restock lists that are applied to the stock in one step.
- Built-in electronics and mechanical tools.

## Build

Requires Qt 6 (Widgets, Sql with SQLite driver, Test), CMake 3.21+ and a C++17 compiler.

```
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/drawer
```

Data is stored in `drawer.db` under the application data directory (`~/.local/share/Drawer` on Linux).

## Usage

### Cabinets and drawers

- Create a cabinet, then add drawer groups: a 4x4 or 3x3 block, or a 1x1 group for a single drawer.
- Double click a drawer to edit its label and size (small, medium, large); click it to manage its items.
- Right click a block and choose Edit to rename it or change its rows and columns. Growing a block pushes its neighbors out of the way; shrinking asks for confirmation if drawers with items would be removed.
- Edit a block to give it a custom color (the name tag follows it, darker). "By size" goes back to green, yellow and red for small, medium and large drawers.

### Moving blocks

- Drag a block to move it. It snaps to the grid and turns red where it would overlap; Esc cancels the drag.
- Right click a block and enable "Move freely" to drop it anywhere.
- Select several blocks with Ctrl+click or by dragging a rectangle on empty space (Ctrl+A selects all, Esc clears), then drag any of them to move the whole selection.

### Items

- The items panel has a "Selected drawer" tab with the items of the chosen drawer, and an "All items" tab (see below).
- The - and + buttons below the list take or add one to the selected items (several rows can be selected).
- Drag items onto any drawer of the cabinet to move them; hold Ctrl while dropping to copy instead.
- In every Add and Edit item dialog the drawer is chosen by clicking it on the cabinet already open on the left. The dialog does not block the window, so the cabinet stays clickable, and it shows the chosen drawer.

#### Minimum stock

An item can have an optional minimum stock.

- It applies to the item as a whole: entries with the same name, category, value and package (ignoring case) in the same cabinet are one item, and their quantities are added up and compared with the minimum.
- At or below the minimum, the rows are tinted and their drawers get a red badge on the cabinet view.
- An item stored in several drawers shows its quantity as `this drawer/total` (e.g. `4/10`). Hovering over it lists the drawers: this drawer first, then up to five others as `Group/R1C1 (4)`.

#### All items tab

- Lists every item of the cabinet with its location. Picking a row selects its drawer, shows it darker and scrolls to it.
- It has its own Add, Edit and Remove buttons, and - / + buttons for the selected entry (off while duplicates are grouped).
- "Group duplicates" (off by default) puts an item stored in several drawers on one row, with its drawers as `R1C1 (10), R2C3 (4)` and the total quantity, and darkens all of those drawers.
- Editing a grouped row changes the common fields of every entry and shows a table with each drawer's quantity so each one can be changed. Remove deletes every entry of the item.

### Pick and restock lists

- Right click items (in the Selected drawer or All items tabs) and choose Add to pick list or Add to restock list. Adding the same entry again raises its quantity by one; double click the quantity to edit it.
- Pick list shows what is in stock and what will be left, and tints lines asking for more than the drawer holds. "Take all from stock" subtracts every line from the stock (never below zero) and empties the list.
- Restock list works the other way: "Restock all" adds every line to the stock.
- The Restock list also has an Add button for components that are not in the database yet. It creates the item with no stock and puts it on the list with the quantity to add.
- Selecting a line darkens its drawer. Both lists can be undone.

### Search

- The search box darkens every drawer holding a matching item. Text matches name, category, value, package and notes. Typing a value such as `1uF` also finds items whose value is written `1u`, `1000nF` or `1µF`.
- The category list next to it keeps only items of one category, both for the darkened drawers (on its own it darkens every drawer with that category) and for the All items tab.
- While a name is typed in the search box or in an Add item dialog, matching items already in stock are suggested, like a search engine. Each suggestion shows the name with its value, package and category. In the search box, choosing one puts its name in the box; in an Add dialog it also fills the category, value, package, minimum stock and notes.

### Editing shortcuts

The Edit menu and the usual shortcuts work.

| Shortcut | Action |
| --- | --- |
| Ctrl+Z | Undo |
| Ctrl+Y, Ctrl+Shift+Z | Redo |
| Ctrl+X | Cut the selected items (pasting then moves them) |
| Ctrl+C | Copy the selected items |
| Ctrl+V | Paste into the selected drawer, as new items |
| Ctrl+A | Select all items when the list has the focus, otherwise all blocks |
| Delete | Remove the selected items when the list has the focus, otherwise the selected drawer |

- Undo and redo cover any change to cabinets, blocks, drawers, items and lists, one user action at a time. The history lasts until the app closes.
- Copy and paste are also in the right click menus (paste on a drawer).
- Deleting a drawer leaves a hole in its block and deletes its items, after a confirmation. Deleting the last drawer of a block removes the block.

### Files

- File > Export mapping writes the full mapping, including empty drawers, to a CSV file.
- File > Export database saves cabinets, blocks, drawers and items to a JSON file.
- File > Import database adds the cabinets from such a file to the current data. Existing cabinets are kept, and an invalid file changes nothing.

### Tools

Tools > Electronics tools opens the calculators.

- **Component reading:** resistor color code, capacitor markings and values, SMD resistor code (including EIA-96).
- **Auxiliary:** Ohm's law, voltage divider, LED resistor, and a metric screw reference (M2 to M4) with the nominal diameter, tap drill and clearance holes for cylindrical and countersunk heads, plus a labeled drawing of each measure.
