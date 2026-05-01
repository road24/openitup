-- Exceed Mode Select Screen
-- Single, Double, Co-op, Battle mode selection

local selected_mode = 0  -- 0=Single, 1=Double, 2=Co-op, 3=Battle
local MODE_NAMES = {"Single", "Double", "Co-op", "Battle"}

function on_enter(params)
    selected_mode = 0
end

function update(dt)
    -- Navigate modes
    if input.is_pressed(PadInput.RIGHT) then
        selected_mode = (selected_mode + 1) % 4
        -- Play navigation SFX
    end

    if input.is_pressed(PadInput.LEFT) then
        selected_mode = (selected_mode - 1) % 4
        -- Play navigation SFX
    end

    -- Confirm selection
    if input.is_pressed(PadInput.CENTER) then
        scene.push("song_select", {mode = MODE_NAMES[selected_mode + 1]})
        -- Play confirmation SFX
    end

    -- Go back to title
    if input.is_pressed(PadInput.BACK) then
        scene.pop()
    end
end

function render()
    renderer.draw_text("Select Mode", 320, 100)

    -- Render mode buttons
    for i = 0, 3 do
        local x = 160 + i * 120
        local y = 240

        if i == selected_mode then
            renderer.draw_text(">> " .. MODE_NAMES[i + 1] .. " <<", x, y)
        else
            renderer.draw_text(MODE_NAMES[i + 1], x, y)
        end
    end
end

function on_exit()
end
