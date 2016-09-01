local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"
local s = require "ejoy2d.shader.c"

--s.blend(0, 0)
pack.load {
	pattern = fw.WorkDir..[[examples/asset/?]],
	"creature",
}


local obj = ej.sprite("creature","raptor")
obj:ps(0.5)
obj.action = "walk"
local game = {}
local screencoord = { x = 512, y = 384, scale = 1.2 }

function game.update()
	obj.frame = obj.frame + 1
end

function game.drawframe()
	ej.clear(0xff808080)	-- clear (0.5,0.5,0.5,1) gray
	obj:draw(screencoord)
--	obj1:draw(screencoord)
end

function game.touch(what, x, y)
end

function game.message(...)
end

function game.handle_error(...)
end

function game.on_resume()
end

function game.on_pause()
end

ej.start(game)

