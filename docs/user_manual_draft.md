## Controls
If host DAW shortcuts are not working, open "GUI Settings" menu and disable "Keyboard Navigation".

### Global Keyboard Navigation
- **Navigate:** Use `Tab` and `Shift + Tab` to move focus sequentially through all interactive controls.
- **Escape Context:** Press `Esc` to close popups, dropdown menus, or back out of focused areas.
- **Focus Memory:** If you switch to another window (e.g., `Alt + Tab`) and return, the plugin will automatically restore keyboard focus to your last used control.

### Knob
- **Adjust:** Click and drag vertically, scroll the mouse wheel, or use the `Up`/`Down`/`Left`/`Right` arrow keys.
- **Fine-Tune:** Hold `Shift` while dragging, scrolling, or using arrow keys for high-precision adjustments.
- **Snap & Step:** Middle-click to jump to the next predefined snap point. Use `Home`/`End` (or `Cmd/Ctrl + Arrow`) to cycle through them via keyboard. Holding `Shift` while Middle-clicking rounds the value to the nearest integer.
- **Exact Entry:** Double-click or press `Enter`/`Space` to open a text box and type a specific value.
- **Reset:** `Cmd/Ctrl + Click` or `Cmd/Ctrl + Backspace/Delete` restores the parameter to its default value.
- **Host Menu:** Right-click or press `Shift + F10` to open the DAW's context menu (for MIDI learn, automation, etc.).

### XY Pad
- **Adjust:** Click and drag the crosshair. Alternatively, use `Left`/`Right` arrows for the X-axis and `Up`/`Down` arrows for the Y-axis. Mouse wheel adjusts X; holding `Shift` adjusts Y.
- **Axis Lock:** Hold `Middle-click` while dragging to lock the Y-axis (changes X only). Add `Shift` to lock the X-axis instead.
- **Snap & Grid:** Click the small square toggle next to the axis labels to enable/disable grid snapping. When enabled, use `PageUp`/`PageDown` to cycle X-axis snaps, and `Home`/`End` for Y-axis snaps.
- **Reset:** `Cmd/Ctrl + Click` or `Cmd/Ctrl + Backspace/Delete` resets both axes to their default values.
- **Host Menu:** Right-click the left/right half of the pad, or press `Shift + F10` (add `Cmd/Ctrl` for the Y-axis) to open the DAW context menu for the respective parameter.

### Dropdown Menu (ComboBox)
- **Select:** Click or press `Enter`/`Space` to open the full dropdown menu.
- **Quick Cycle:** Scroll with the mouse wheel, or use the arrow keys to rapidly change the selection without opening the menu.
- **Reset:** `Cmd/Ctrl + Click` or `Cmd/Ctrl + Backspace/Delete` restores the default option.
- **Host Menu:** Right-click or press `Shift + F10` to open the DAW context menu.

### Button (Action, Toggle, Momentary)
- **Interact:** Left-click, or press `Enter`/`Space` to trigger the button.
- **Host Menu:** Right-click or press `Shift + F10` to open the DAW context menu (available on toggle and momentary buttons).

### Parameter Lock
Prevents specific parameters from being altered when using the global "Randomize" function.

- **Toggle Lock:** Click the parameter's text label, or press `Enter`/`Space` while the label is focused. Locked parameters appear visually dimmed to indicate they are protected.

### Tab View
- **Navigate:** Click a tab header to switch views. You can also scroll the mouse wheel while hovering over the headers, or use the `Left`/`Right` arrow keys when the tab header is focused.

### Preset Manager
The Preset Manager is an interface to save, load, and browse patches.

- **Browse:** Click the preset name to open the file browser. Click folder names to navigate deeper, or `.. (Back)` to return to the parent folder.
- **Cycle:** Use the `<` and `>` arrows to cycle through all presets in the library sequentially.
- **Refresh:** To manually add files to the preset folder via operating system, select `Refresh` from the menu to update the list.

#### Potential Failure Mode
- **Stale Cache (External Changes):** Since the Next/Prev list is cached in memory (`presetCache`), if a user adds or renames a file using the OS File Explorer while the plugin is open, the "Next" button won't see it until they click `Refresh` or save a new preset.
- **Deleted Files:** If a file in the cache is deleted externally, clicking "Next" will attempt to load a path that no longer exists. The code handles this safely (it simply won't load), but the UI might feel unresponsive (staying on the current preset) or the cycle might skip unexpectedly.
- **Network Drives:** Accessing the file system is done on the Message Thread (GUI thread). If the preset folder is on a slow network drive or a sleeping external disk, the UI will freeze briefly during the first "Next" click (cache build) or when opening a directory in the menu.

### Plugin Information & License
Displays version info, credits, and licensing details.

- **View:** Click the "Info" button (or press `Enter`/`Space`) to open the overlay.
- **Navigate:** Use `Tab` to switch between the Information and License pages. Arrow keys can be used to scroll the text.
- **Copy:** Text can be highlighted and copied to the clipboard (`Cmd/Ctrl + C`).
- **Close:** Click the dark margins at the top/bottom of the popup, or press `Esc`.

---

To close this message, click the margin outside of tab view.

--------
Controls
--------

NOTE: If DAW shortcuts fail, disable "Keyboard Navigation" in GUI Settings.

-- Keyboard Navigation --
Navigate        : Tab | Shift + Tab
Interact        : Space | Enter
Close | Back    : Esc
Focus Memory    : Auto-restores last control after Alt + Tab

-- Knob --
Adjust          : Vertical Drag | Scroll | Arrows
Fine-Tune       : Shift + Adjust
Snap Next       : Mid-Click
Snap Cycle      : Home | End | @MOD@ + Arrow
Snap Integer    : Shift + Mid-Click
Exact Value     : Dbl-Click | Enter | Space
Reset           : @MOD@ + Click | Backspace | Del
Host Menu       : Right-Click | Shift + F10

-- XY Pad --
Adjust Both     : Drag | Arrows (X = Left/Right, Y = Up/Down)
Adjust X | Y    : Scroll (X) | Shift + Scroll (Y)
Move X Only     : Mid-Click + Drag
Move Y Only     : Shift + Mid-Click + Drag
Toggle Grid     : Click square toggle
Snap Cycle X    : PgUp | PgDn
Snap Cycle Y    : Home | End
Reset Both      : @MOD@ + Click | Backspace | Del
Host Menu X     : Right-Click L-half | Shift + F10
Host Menu Y     : Right-Click R-half | @MOD@ + Shift + F10

-- Combo Box --
Open Menu       : Click | Enter | Space
Quick Cycle     : Scroll | Arrows
Reset           : @MOD@ + Click | Backspace | Del
Host Menu       : Right-Click | Shift + F10

-- Button --
Trigger         : Click | Enter | Space
Host Menu       : Right-Click | Shift + F10

-- Parameter Lock --
Toggle Lock     : Click Label | Enter | Space (Dimmed = Locked)

Locks parameter from the "Randomize" button.

-- Tab View --
Switch View     : Click Header | Scroll on Header | Arrows

-- Preset Manager --
Browse Menu     : Click Preset Name (Center)
Cycle Presets   : Click or Keyboard Arrows
Refresh         : Select in Menu

External file changes require a Refresh. Slow network drives may cause UI pauses.

-- Info & License --
View Info       : Click "Info" | Enter | Space
Pages | Scroll  : Tab | Arrows
Copy Text       : @MOD@ + C
Close           : Click margins | Esc
