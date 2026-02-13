state = {
    status_tag = "v1",
    theme_dark = false,
    bg_r = 0.07,
    bg_g = 0.09,
    bg_b = 0.12,
    x = 180.0,
    y = 170.0,
    radius = 24.0,
    circle_r = 0.43,
    circle_g = 0.73,
    circle_b = 1.00,
    circle_filled = true,
    prev_mouse_down = false,
    t = 0.0,
}

state.layout = {
    w = 360.0,
    h = 640.0,
}

state.design = {
    side_pad = 18.0,
    bottom_pad = 48.0,
    gap = 14.0,
    edit_h = 44.0,
    button_h = 40.0,
    chip_h = 30.0,
    label_offset = 18.0,
    edit_line_top = 6.0,
    text_font_px = 16.0,
    renderer_text_nudge = 4.0,
}

themes = {
    dark = {
        bg_r = 0.07,
        bg_g = 0.09,
        bg_b = 0.12,
        circle_r = 0.43,
        circle_g = 0.73,
        circle_b = 1.00,
        surface_r = 0.20,
        surface_g = 0.24,
        surface_b = 0.30,
        hover_r = 0.24,
        hover_g = 0.30,
        hover_b = 0.38,
        focus_r = 0.29,
        focus_g = 0.43,
        focus_b = 0.58,
        accent_r = 0.33,
        accent_g = 0.55,
        accent_b = 0.78,
        line_muted_r = 0.20,
        line_muted_g = 0.27,
        line_muted_b = 0.35,
        text_r = 1.00,
        text_g = 1.00,
        text_b = 1.00,
    },
    light = {
        bg_r = 0.92,
        bg_g = 0.94,
        bg_b = 0.97,
        circle_r = 0.16,
        circle_g = 0.44,
        circle_b = 0.78,
        surface_r = 0.98,
        surface_g = 0.99,
        surface_b = 1.00,
        hover_r = 0.90,
        hover_g = 0.95,
        hover_b = 1.00,
        focus_r = 0.79,
        focus_g = 0.89,
        focus_b = 0.98,
        accent_r = 0.16,
        accent_g = 0.44,
        accent_b = 0.78,
        line_muted_r = 0.62,
        line_muted_g = 0.68,
        line_muted_b = 0.76,
        text_r = 0.08,
        text_g = 0.10,
        text_b = 0.14,
    },
}

state.widgets = {
    editbox = {
        x = 16.0,
        y = 362.0,
        w = 328.0,
        h = 44.0,
        input_type = "text",
        text = "type here",
        focused = false,
        cursor_t = 0.0,
        max_len = 32,
        empty_hint = "<type here>",
        label = "Name",
    },
    editbox2 = {
        x = 16.0,
        y = 432.0,
        w = 328.0,
        h = 44.0,
        input_type = "password",
        text = "",
        reveal_password = false,
        focused = false,
        cursor_t = 0.0,
        max_len = 32,
        empty_hint = "<password>",
        label = "Password",
    },
    button = {
        x = 16.0,
        y = 502.0,
        w = 328.0,
        h = 40.0,
        label = "Toggle Circle Fill",
        clicks = 0,
        armed = false,
        flash_t = 0.0,
    },
    toggle_chip = {
        x = 16.0,
        y = 554.0,
        w = 328.0,
        h = 30.0,
        label = "Snap",
        on = false,
        armed = false,
    },
}

local function point_in_rect(px, py, x, y, w, h)
    return px >= x and px <= (x + w) and py >= y and py <= (y + h)
end

local function mix(a, b, t)
    return a + (b - a) * t
end

local function cmd_rect(x, y, w, h, r, g, b, filled)
    return { kind = "rect", x = x, y = y, w = w, h = h, r = r, g = g, b = b, filled = filled }
end

local function cmd_line(x1, y1, x2, y2, r, g, b)
    return { kind = "line", x = x1, y = y1, x2 = x2, y2 = y2, r = r, g = g, b = b }
end

local function cmd_text(x, y, text, r, g, b)
    return { kind = "text", x = x, y = y, text = text, r = r, g = g, b = b }
end

local function cmd_circle(x, y, radius, r, g, b, filled)
    return { kind = "circle", x = x, y = y, radius = radius, r = r, g = g, b = b, filled = filled }
end

local function text_y_centered_in_box(box_y, box_h, design)
    return box_y + ((box_h - design.text_font_px) * 0.5) + design.renderer_text_nudge
end

local function any_text_widget_focused(widgets)
    for _, widget in pairs(widgets) do
        if type(widget) == "table" then
            local is_text_like = (widget.text ~= nil) or (widget.max_len ~= nil) or (widget.empty_hint ~= nil)
            if is_text_like and widget.focused == true then
                return true
            end
        end
    end
    return false
end

local function mask_password(text)
    return string.rep("•", #text)
end

local function apply_auto_layout(widgets, layout, design)
    local side_pad = design.side_pad
    local control_w = layout.w - (side_pad * 2.0)

    local chip = widgets.toggle_chip
    chip.x = side_pad
    chip.w = control_w
    chip.h = design.chip_h
    chip.y = layout.h - design.bottom_pad - chip.h

    local button = widgets.button
    button.x = side_pad
    button.w = control_w
    button.h = design.button_h
    button.y = chip.y - design.gap - button.h

    local edit2 = widgets.editbox2
    edit2.x = side_pad
    edit2.w = control_w
    edit2.h = design.edit_h
    edit2.y = button.y - design.gap - edit2.h - design.label_offset

    local edit1 = widgets.editbox
    edit1.x = side_pad
    edit1.w = control_w
    edit1.h = design.edit_h
    edit1.y = edit2.y - design.gap - edit1.h - design.label_offset
end

local function update_button(button, dt, input, theme, design, down_edge, up_edge, over_button, on_click)
    if down_edge and over_button then
        button.armed = true
    end

    if up_edge then
        if button.armed and over_button then
            button.clicks = button.clicks + 1
            button.flash_t = 0.15
            if on_click ~= nil then
                on_click()
            end
        end
        button.armed = false
    end

    local button_r = theme.surface_r
    local button_g = theme.surface_g
    local button_b = theme.surface_b

    if input.mouse_down and button.armed then
        button_r = theme.focus_r
        button_g = theme.focus_g
        button_b = theme.focus_b
    end

    if button.flash_t > 0.0 then
        button.flash_t = math.max(0.0, button.flash_t - dt)
        button_r = theme.accent_r
        button_g = theme.accent_g
        button_b = theme.accent_b
    end

    return {
        r = button_r,
        g = button_g,
        b = button_b,
        text = string.format("%s (%d)", button.label, button.clicks),
        text_x = button.x + 8.0,
        text_y = text_y_centered_in_box(button.y, button.h, design),
        text_r = theme.text_r,
        text_g = theme.text_g,
        text_b = theme.text_b,
    }
end

local function update_editbox(editbox, input, theme, design, down_edge, over_edit)
    local is_password = editbox.input_type == "password"

    if is_password and editbox.reveal_password == nil then
        editbox.reveal_password = false
    end

    local eye_w = 28.0
    local eye_x = editbox.x + editbox.w - eye_w
    local eye_over = is_password and point_in_rect(input.mouse_x, input.mouse_y, eye_x, editbox.y, eye_w, editbox.h)

    if is_password and down_edge and eye_over then
        editbox.reveal_password = not editbox.reveal_password
        editbox.focused = true
    end

    if down_edge then
        editbox.focused = over_edit
    end

    if editbox.focused then
        if input.backspace_pressed and #editbox.text > 0 then
            editbox.text = string.sub(editbox.text, 1, #editbox.text - 1)
        end

        if input.text_input and #input.text_input > 0 then
            editbox.text = string.sub(editbox.text .. input.text_input, 1, editbox.max_len)
        end

        if input.enter_pressed then
            editbox.focused = false
        end
    end

    local rect_r = theme.surface_r
    local rect_g = theme.surface_g
    local rect_b = theme.surface_b

    if editbox.focused then
        rect_r = theme.focus_r
        rect_g = theme.focus_g
        rect_b = theme.focus_b
    end

    local line_r = theme.line_muted_r
    local line_g = theme.line_muted_g
    local line_b = theme.line_muted_b
    if editbox.focused then
        line_r = theme.accent_r
        line_g = theme.accent_g
        line_b = theme.accent_b
    end

    local caret = ""
    if editbox.focused and (math.floor(editbox.cursor_t * 2.0) % 2 == 0) then
        caret = "|"
    end

    local shown = editbox.text
    if shown == "" then
        shown = editbox.empty_hint
    elseif is_password and (not editbox.reveal_password) then
        shown = mask_password(shown)
    end

    local eye_text = ""
    local eye_r = mix(theme.text_r, theme.line_muted_r, 0.20)
    local eye_g = mix(theme.text_g, theme.line_muted_g, 0.20)
    local eye_b = mix(theme.text_b, theme.line_muted_b, 0.20)
    if is_password then
        eye_text = editbox.reveal_password and "[-]" or "[+]"
    end

    return {
        rect_x = editbox.x,
        rect_y = editbox.y,
        rect_w = editbox.w,
        rect_h = editbox.h,
        rect_r = rect_r,
        rect_g = rect_g,
        rect_b = rect_b,
        line_x1 = editbox.x,
        line_y1 = editbox.y - design.edit_line_top,
        line_x2 = editbox.x + editbox.w,
        line_y2 = editbox.y - design.edit_line_top,
        line_r = line_r,
        line_g = line_g,
        line_b = line_b,
        text = string.format("%s%s", shown, caret),
        text_x = editbox.x + 8.0,
        text_y = text_y_centered_in_box(editbox.y, editbox.h, design),
        text_r = theme.text_r,
        text_g = theme.text_g,
        text_b = theme.text_b,
        label = editbox.label,
        label_x = editbox.x,
        label_y = editbox.y - design.label_offset,
        label_r = mix(theme.text_r, theme.line_muted_r, 0.35),
        label_g = mix(theme.text_g, theme.line_muted_g, 0.35),
        label_b = mix(theme.text_b, theme.line_muted_b, 0.35),
        eye_text = eye_text,
        eye_text_x = eye_x + 6.0,
        eye_text_y = text_y_centered_in_box(editbox.y, editbox.h, design),
        eye_text_r = eye_r,
        eye_text_g = eye_g,
        eye_text_b = eye_b,
    }
end

local function update_toggle_chip(chip, input, theme, design, down_edge, up_edge, over_chip)
    if down_edge and over_chip then
        chip.armed = true
    end

    if up_edge then
        if chip.armed and over_chip then
            chip.on = not chip.on
        end
        chip.armed = false
    end

    local bg_r = theme.surface_r
    local bg_g = theme.surface_g
    local bg_b = theme.surface_b
    if chip.on then
        bg_r = theme.focus_r
        bg_g = theme.focus_g
        bg_b = theme.focus_b
    end

    local tag = chip.on and "ON" or "OFF"
    return {
        rect_x = chip.x,
        rect_y = chip.y,
        rect_w = chip.w,
        rect_h = chip.h,
        rect_r = bg_r,
        rect_g = bg_g,
        rect_b = bg_b,
        text = string.format("%s: %s", chip.label, tag),
        text_x = chip.x + 8.0,
        text_y = text_y_centered_in_box(chip.y, chip.h, design),
        text_r = mix(theme.text_r, theme.line_muted_r, 0.25),
        text_g = mix(theme.text_g, theme.line_muted_g, 0.25),
        text_b = mix(theme.text_b, theme.line_muted_b, 0.25),
    }
end

function update(dt, input)
    state.t = state.t + dt

    local editbox = state.widgets.editbox
    local editbox2 = state.widgets.editbox2
    local button = state.widgets.button
    local toggle_chip = state.widgets.toggle_chip
    editbox.cursor_t = editbox.cursor_t + dt
    editbox2.cursor_t = editbox2.cursor_t + dt

    state.text_input_active = any_text_widget_focused(state.widgets)

    apply_auto_layout(state.widgets, state.layout, state.design)

    local theme = state.theme_dark and themes.dark or themes.light

    local mouse_valid = input.mouse_x > 0 and input.mouse_y > 0
    local over_edit = mouse_valid and
        point_in_rect(input.mouse_x, input.mouse_y, editbox.x, editbox.y, editbox.w, editbox.h)
    local over_edit2 = mouse_valid and point_in_rect(input.mouse_x, input.mouse_y, editbox2.x, editbox2.y, editbox2.w,
        editbox2.h)
    local over_button = mouse_valid and
        point_in_rect(input.mouse_x, input.mouse_y, button.x, button.y, button.w, button.h)
    local over_chip = mouse_valid and
        point_in_rect(input.mouse_x, input.mouse_y, toggle_chip.x, toggle_chip.y, toggle_chip.w, toggle_chip.h)

    local down_edge = input.mouse_down and (not state.prev_mouse_down)
    local up_edge = (not input.mouse_down) and state.prev_mouse_down

    local edit_render = update_editbox(editbox, input, theme, state.design, down_edge, over_edit)
    local edit2_render = update_editbox(editbox2, input, theme, state.design, down_edge, over_edit2)
    local button_render = update_button(button, dt, input, theme, state.design, down_edge, up_edge, over_button,
        function()
            state.circle_filled = not state.circle_filled
        end)
    local chip_render = update_toggle_chip(toggle_chip, input, theme, state.design, down_edge, up_edge, over_chip)

    state.theme_dark = toggle_chip.on

    local can_drag = input.mouse_down and mouse_valid and (not over_edit) and (not over_edit2) and (not over_button) and
        (not over_chip) and
        (not editbox.focused) and (not editbox2.focused)

    if can_drag then
        state.x = input.mouse_x
        state.y = input.mouse_y
    else
        state.x = 260.0 + math.sin(state.t * 1.2) * 64.0
        state.y = 180.0 + math.cos(state.t * 1.0) * 44.0
    end

    state.bg_r = theme.bg_r
    state.bg_g = theme.bg_g
    state.bg_b = theme.bg_b

    state.circle_r = theme.circle_r
    state.circle_g = theme.circle_g
    state.circle_b = theme.circle_b

    state.draw_commands = {
        cmd_circle(state.x, state.y, state.radius, state.circle_r, state.circle_g, state.circle_b, state.circle_filled),
        cmd_rect(edit_render.rect_x, edit_render.rect_y, edit_render.rect_w, edit_render.rect_h, edit_render.rect_r,
            edit_render.rect_g, edit_render.rect_b, true),
        cmd_rect(edit_render.rect_x, edit_render.rect_y, edit_render.rect_w, edit_render.rect_h, 0.0, 0.0, 0.0, false),
        cmd_line(edit_render.line_x1, edit_render.line_y1, edit_render.line_x2, edit_render.line_y2, edit_render.line_r,
            edit_render.line_g, edit_render.line_b),

        cmd_rect(edit2_render.rect_x, edit2_render.rect_y, edit2_render.rect_w, edit2_render.rect_h, edit2_render.rect_r,
            edit2_render.rect_g, edit2_render.rect_b, true),
        cmd_rect(edit2_render.rect_x, edit2_render.rect_y, edit2_render.rect_w, edit2_render.rect_h, 0.0, 0.0, 0.0, false),
        cmd_line(edit2_render.line_x1, edit2_render.line_y1, edit2_render.line_x2, edit2_render.line_y2,
            edit2_render.line_r, edit2_render.line_g, edit2_render.line_b),

        cmd_rect(button.x, button.y, button.w, button.h, button_render.r, button_render.g, button_render.b, true),
        cmd_rect(button.x, button.y, button.w, button.h, 0.0, 0.0, 0.0, false),
        cmd_rect(chip_render.rect_x, chip_render.rect_y, chip_render.rect_w, chip_render.rect_h, chip_render.rect_r,
            chip_render.rect_g, chip_render.rect_b, true),
        cmd_rect(chip_render.rect_x, chip_render.rect_y, chip_render.rect_w, chip_render.rect_h, 0.0, 0.0, 0.0, false),
        cmd_text(edit_render.label_x, edit_render.label_y, edit_render.label, edit_render.label_r, edit_render.label_g,
            edit_render.label_b),
        cmd_text(edit2_render.label_x, edit2_render.label_y, edit2_render.label, edit2_render.label_r,
            edit2_render.label_g,
            edit2_render.label_b),
        cmd_text(edit_render.text_x, edit_render.text_y, edit_render.text, edit_render.text_r, edit_render.text_g,
            edit_render.text_b),
        cmd_text(edit2_render.text_x, edit2_render.text_y, edit2_render.text, edit2_render.text_r, edit2_render.text_g,
            edit2_render.text_b),
        cmd_text(edit_render.eye_text_x, edit_render.eye_text_y, edit_render.eye_text, edit_render.eye_text_r,
            edit_render.eye_text_g, edit_render.eye_text_b),
        cmd_text(edit2_render.eye_text_x, edit2_render.eye_text_y, edit2_render.eye_text, edit2_render.eye_text_r,
            edit2_render.eye_text_g, edit2_render.eye_text_b),
        cmd_text(button_render.text_x, button_render.text_y, button_render.text, button_render.text_r,
            button_render.text_g,
            button_render.text_b),
        cmd_text(chip_render.text_x, chip_render.text_y, chip_render.text, chip_render.text_r, chip_render.text_g,
            chip_render.text_b),
    }

    state.prev_mouse_down = input.mouse_down

    return state
end
