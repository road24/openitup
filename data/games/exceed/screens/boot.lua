-- Exceed Boot Screen
-- Displays logo and transitions to title

local timer = 0.0
local BOOT_DURATION = 2.0

function on_enter(params)
    timer = 0.0
    -- Logo BGA would be loaded here if available
end

function update(dt)
    timer = timer + dt

    -- Skip boot screen with START button
    if input.is_pressed(PadInput.START) then
        scene.replace("title")
        return
    end

    -- Auto-transition after BOOT_DURATION
    if timer >= BOOT_DURATION then
        scene.replace("title")
    end
end

function render()
    -- Placeholder: render logo BGA when available
    -- renderer.draw_bga("logo.bgaj", 0, 0, timer)

    renderer.draw_text("openitup - Exceed", 320, 240)
end

function on_exit()
    -- Cleanup
end
