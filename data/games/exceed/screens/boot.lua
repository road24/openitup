-- Exceed Boot Screen
-- Displays logo BGA and transitions to title

local timer = 0.0
local BOOT_DURATION = 3.0
local logo_bga = nil

function on_enter(params)
    timer = 0.0
    -- Load logo BGA from game package assets
    logo_bga = lua_render.load_bga(game.resolve("animations/boot/logo.bgaj"))
end

function update(dt)
    timer = timer + dt

    if lua_input.is_pressed("START") then
        lua_scene.replace("title")
        return
    end

    if timer >= BOOT_DURATION then
        lua_scene.replace("title")
    end
end

function render()
    if logo_bga then
        lua_render.draw_bga(game.resolve("animations/boot/logo.bgaj"), timer * 60.0)
    end

    -- Fallback text if no BGA
    lua_draw.rect(0, 0, 640, 480, 0, 0, 0, 255)
    lua_render.draw_text("openitup", 280, 220)
    lua_render.draw_text("- Exceed -", 270, 250)
end

function on_exit()
    logo_bga = nil
end
