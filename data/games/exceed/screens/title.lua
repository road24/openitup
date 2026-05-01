-- Exceed Title Screen
-- Attract mode loop with "Press START" prompt

local timer = 0.0
local attract_timer = 0.0
local ATTRACT_DELAY = 30.0  -- Enter attract mode after 30 seconds of inactivity

function on_enter(params)
    timer = 0.0
    attract_timer = 0.0
    -- Title BGM would start here
end

function update(dt)
    timer = timer + dt
    attract_timer = attract_timer + dt

    if input.is_pressed(PadInput.START) then
        scene.push("mode_select")
    end

    -- Future: trigger attract mode demo after ATTRACT_DELAY
end

function render()
    -- Placeholder: render title BGA when available
    -- renderer.draw_bga("title.bgaj", 0, 0, timer)

    renderer.draw_text("EXCEED", 320, 180)

    -- Blinking "Press START" prompt
    if math.floor(timer * 2) % 2 == 0 then
        renderer.draw_text("Press START", 320, 300)
    end
end

function on_exit()
    -- Stop title BGM
end
