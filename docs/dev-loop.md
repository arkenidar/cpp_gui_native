# PC dev loop

- Launch app once (`./tools/dev/run_pc.ps1`)
- Edit [assets/scripts/main.lua](assets/scripts/main.lua) while app runs
- Save file and observe automatic polling-based reload
- For instant reload confirmation, change `status_tag` (for example `v1` -> `v2`) and verify text updates on screen
- If reload fails, check terminal output for Lua error details

## Known good workflow
- Runtime iteration: launch once, edit [assets/scripts/main.lua](assets/scripts/main.lua), save, verify `status_tag` changes on screen
- Debug run (F5): use `Run gui_pc (F5)` (builds `build-debug` then launches debugger)
- Release run: use [tools/dev/run_pc.ps1](tools/dev/run_pc.ps1) or run `./build/gui_pc`
- Fast recovery: kill stale terminals/debug sessions, run build task `Configure+Build gui_pc (Debug)`, then F5 again

## Current controls
- Without mouse drag, circle runs orbit animation
- Hold left mouse button inside window to drag circle
- Release to return to orbit animation
- Dedicated button widget above edit box toggles circle fill on click
- Button has independent hover/press/flash styling and click count
- Rectangle is now a tiny Lua-driven edit box (hover/focus color states)
- Click edit box to focus, type text, use Backspace to delete, Enter to defocus
- Input is capped at 32 chars; debug text shows `(len/32)` and `MAX` at cap
- Edit text is rendered inside the edit box with left padding
- Divider line under edit box reflects focus (muted when unfocused, accent when focused)
- Press `T` to toggle dark/light theme
- Debug text shows `status_tag` and live edit-box text content

## Visual style notes
- Theme tokens live in [assets/scripts/main.lua](assets/scripts/main.lua) (`theme` table)
- Palette is stabilized (reduced color animation) for cleaner debug UI readability
- Layout is mobile-first for `360x640`: top status text, central accent circle, bottom edit box

## Add a new widget type (Lua-only)
Current rendering is command-driven through `state.draw_commands`, so new widget types are defined in Lua and do not require new C++ fields.

### Pattern
1. Add widget data under `state.widgets`.
2. Add/update helper function for the widget type (`update_*`) that returns render payload + updated widget state.
3. In `update(dt, input)`, call the helper and append draw commands (`cmd_rect`, `cmd_line`, `cmd_text`, `cmd_circle`) into `state.draw_commands`.

### Example: new widget type `badge`
Add a lightweight label badge type that is purely visual.

```lua
state.widgets.badge = {
	x = 16.0,
	y = 398.0,
	w = 110.0,
	h = 26.0,
	text = "BETA",
}

local function update_badge(badge, theme)
	return {
		rect = cmd_rect(badge.x, badge.y, badge.w, badge.h, theme.focus_r, theme.focus_g, theme.focus_b, true),
		text = cmd_text(badge.x + 8.0, badge.y + 17.0, badge.text, theme.text_r, theme.text_g, theme.text_b),
	}
end
```

### Add one widget instance of that type
Inside `update(dt, input)`, render it by adding its commands to `state.draw_commands`:

```lua
local badge_render = update_badge(state.widgets.badge, theme)

state.draw_commands = {
	-- existing commands...
	badge_render.rect,
	badge_render.text,
}
```

### Notes
- To add another badge, copy only `state.widgets.badge` into `badge2` with different `x/y/text`, then append one more `update_badge(...)` output.
- Interactive widget types follow the same pattern, but helper takes `input`, computes hover/click/focus state, and returns draw commands.

## Layout system (current)
Layout is now computed in Lua each frame by `apply_auto_layout(state.widgets, state.layout)` in [assets/scripts/main.lua](assets/scripts/main.lua).

### Authoritative knobs
These live in `state.design` and should be your first stop for visual tuning:
- `side_pad`: horizontal content margin
- `bottom_pad`: distance from bottom edge to last control
- `gap`: vertical spacing between stacked controls
- `edit_h`: textbox height
- `button_h`: button height
- `chip_h`: toggle chip height
- `edit_label_top`: label offset above textbox top (also used as reserved headroom in layout)
- `edit_line_top`: accent-line offset above textbox top
- `edit_text_y`: textbox text baseline offset from textbox top
- `control_text_nudge`: vertical centering tweak for button/chip text

### Stack order and no-overlap invariant
The vertical stack order is fixed and computed bottom-up:
1. `toggle_chip`
2. `button`
3. `editbox2` (`Notes`)
4. `editbox` (`Name`)

No-overlap behavior is enforced by construction:
- each control `y` is derived from the previous control + `gap`
- `edit_label_top` is subtracted when placing the next textbox above, reserving label headroom

### Quick tuning checklist (1 minute)
1. Change `gap` for denser/looser vertical rhythm.
2. Change `bottom_pad` to move entire stack up/down.
3. Change `edit_label_top` if label-line separation needs more space.
4. Change `edit_text_y` if textbox text appears too high/low for current font.
5. Save and verify live via hot reload.
