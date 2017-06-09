local s = require "ejoy2d.shader.c"
local c = require "ejoy2d.sprite.c"

local PRECISION = ""
local PRECISION_HIGH = ""
local OPENGLES_VERSION = s.version()

if OPENGLES_VERSION >= 2 then
	-- Opengl ES 2.0 need float precision specifiers
	PRECISION = "#version 100\nprecision lowp float;\n"
	PRECISION_HIGH = "#version 100\nprecision highp float;\n"
end

local sprite_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;
uniform sampler2D texture0;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = tmp.xyz * v_color.xyz;
	gl_FragColor.w = tmp.w;
	gl_FragColor *= v_color.w;
	gl_FragColor.xyz += v_additive.xyz * tmp.w;
}
]]

local sprite_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;
attribute vec4 additive;

varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

void main() {
	gl_Position = position + vec4(-1.0,1.0,0,0);
	v_texcoord = texcoord;
	v_color = color;
	v_additive = additive;
}
]]

local gui_text_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;
attribute vec4 additive;

varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform mat4 inv_pmv;

void main() {
	gl_Position = inv_pmv * position + vec4(-1.0,1.0,0,0);
	v_texcoord = texcoord;
	v_color = color;
	v_additive = additive;
}
]]


local text_fs = {
	[2] = [[
		varying vec2 v_texcoord;
		varying vec4 v_color;
		varying vec4 v_additive;

		uniform sampler2D texture0;

		void main() {
			float c = texture2D(texture0, v_texcoord).w;
			float alpha = clamp(c, 0.0, 0.5) * 2.0;

			gl_FragColor.xyz = (v_color.xyz + v_additive.xyz) * alpha;
			gl_FragColor.w = alpha;
			gl_FragColor *= v_color.w;
		}
	]],

	[3] = [[
		varying vec2 v_texcoord;
		varying vec4 v_color;
		varying vec4 v_additive;

		uniform sampler2D texture0;

		void main() {
			float c = texture2D(texture0, v_texcoord).r;
			float alpha = clamp(c, 0.0, 0.5) * 2.0;

			gl_FragColor.xyz = (v_color.xyz + v_additive.xyz) * alpha;
			gl_FragColor.w = alpha;
			gl_FragColor *= v_color.w;
		}
	]]
}

local text_edge_fs = {
	[2] = [[
		varying vec2 v_texcoord;
		varying vec4 v_color;
		varying vec4 v_additive;

		uniform sampler2D texture0;

		void main() {
			float c = texture2D(texture0, v_texcoord).w;
			float alpha = clamp(c, 0.0, 0.5) * 2.0;
			float color = (clamp(c, 0.5, 1.0) - 0.5) * 2.0;

			gl_FragColor.xyz = (v_color.xyz + v_additive.xyz) * color;
			gl_FragColor.w = alpha;
			gl_FragColor *= v_color.w;
		}
	]],
	[3] = [[
		varying vec2 v_texcoord;
		varying vec4 v_color;
		varying vec4 v_additive;

		uniform sampler2D texture0;

		void main() {
			float c = texture2D(texture0, v_texcoord).r;
			float alpha = clamp(c, 0.0, 0.5) * 2.0;
			float color = (clamp(c, 0.5, 1.0) - 0.5) * 2.0;

			gl_FragColor.xyz = (v_color.xyz + v_additive.xyz) * color;
			gl_FragColor.w = alpha;
			gl_FragColor *= v_color.w;
		}
	]],
}


local gray_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main()
{
	vec4 tmp = texture2D(texture0, v_texcoord);
	vec4 c;
	c.xyz = tmp.xyz * v_color.xyz;
	c.w = tmp.w;
	c *= v_color.w;
	c.xyz += v_additive.xyz * tmp.w;
	float g = dot(c.rgb , vec3(0.299, 0.587, 0.114));
	gl_FragColor = vec4(g,g,g,c.a);
}
]]

local color_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main()
{
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = v_color.xyz * tmp.w;
	gl_FragColor.w = tmp.w;
}
]]


local blend_fs = [[
varying vec2 v_texcoord;
varying vec2 v_mask_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = tmp.xyz * v_color.xyz;
	gl_FragColor.w = tmp.w;
	gl_FragColor *= v_color.w;
	gl_FragColor.xyz += v_additive.xyz * tmp.w;

	vec4 m = texture2D(texture0, v_mask_texcoord);
	gl_FragColor.xyz *= m.xyz;
//	gl_FragColor *= m.w;
}
]]


local blend_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;
attribute vec4 additive;

varying vec2 v_texcoord;
varying vec2 v_mask_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform vec2 mask;

void main() {
	gl_Position = position + vec4(-1,1,0,0);
	v_texcoord = texcoord;
	v_mask_texcoord = texcoord + mask;
	v_color = color;
    v_additive = additive;
}
]]

local renderbuffer_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
uniform sampler2D texture0;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = tmp.xyz * v_color.xyz;
	gl_FragColor.w = tmp.w;
	gl_FragColor *= v_color.w;
}
]]

local renderbuffer_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;

varying vec2 v_texcoord;
varying vec4 v_color;

uniform vec4 st;

void main() {
	gl_Position.x = position.x * st.x + st.z -1.0;
	gl_Position.y = position.y * st.y + st.w +1.0;
	gl_Position.z = position.z;
	gl_Position.w = position.w;
	v_texcoord = texcoord;
	v_color = color;
}
]]

local shader = {}

local shader_name = {
	NORMAL = 0,
	RENDERBUFFER = 1,
	TEXT = 2,
	EDGE = 3,
	GUI_TEXT = 4,
	GUI_EDGE = 5,
	GRAY = 6,
	COLOR = 7,
	BLEND = 8,
}
-- user defined shader (or replace default shader)
local MAX_PROGRAM = 16
local USER_PROGRAM = 9

local uniform_format = {
	float = 1,
	float2 = 2,
	float3 = 3,
	float4 = 4,
	matrix33 = 5,
	matrix44 = 6,
}

local shader_material = {}

shader.draw = s.draw
shader.blend = s.blend
shader.clear = s.clear
shader.texture = s.shader_texture

-- 根据prog的名字得到模板id
function shader.id(name)
	local id = assert(shader_name[name] , "Invalid shader name " .. name)
	return id
end

local uniform_set = s.uniform_set

-- 返回一个table，其中每一项都是一个uniform name 为key的function，通过调用这个
-- 函数可以设置uniform的值，这也就是用户角度看到的shader对象了
local function create_shader(id, uniform)
	if uniform then
		local s = {}
		for index , u in ipairs(uniform) do
			local loc = index-1
			local format = u.type
			s[u.name] = function(...)
				uniform_set(id, loc, format, ...)
			end
		end
		return s
	end
end

local material_setuniform = s.material_setuniform
local material_settexture = s.material_settexture

-- 根据id指定的prog生成一个带__index的的metatable，通过它可以方便设置material的uniform和texture
-- metatable主要给sprite用
-- id 为shader 模板id
local function material_meta(id, arg)
	local uniform = arg.uniform or arg
	local meta
	if uniform then
		local index_table = {}
		meta = { __index = index_table }
		for index , u in ipairs(uniform) do
			local loc = index-1
			index_table[u.name] = function(self, ...)
				material_setuniform(self.__obj, loc, ...) -- __obj是material这个userdata
			end
		end
		if arg.texture then
			index_table.texture = function(self, ...)
				material_settexture(self.__obj, ...)
			end
		end
	end
	shader_material[id] = meta
end

local function create_text_material(id)
	local text_uniform={{name="inv_pmv", type=uniform_format.matrix44}}
	s.uniform_bind(id, text_uniform)
	create_shader(id, text_uniform)
	material_meta(id, text_uniform)
	local meta=shader.material_meta(id)
	local mat = {__obj=c.new_material(id)}
	return setmetatable(mat, meta)
end

function shader.init()
	s.load(shader_name.NORMAL, PRECISION .. sprite_fs, PRECISION .. sprite_vs)
	s.load(shader_name.TEXT, PRECISION .. (text_fs[OPENGLES_VERSION] or text_fs[2]), PRECISION .. sprite_vs)
	s.load(shader_name.EDGE, PRECISION .. (text_edge_fs[OPENGLES_VERSION] or text_edge_fs[2]), PRECISION .. sprite_vs)
	s.load(shader_name.GUI_TEXT, PRECISION .. (text_fs[OPENGLES_VERSION] or text_fs[2]), PRECISION .. gui_text_vs)
	s.load(shader_name.GUI_EDGE, PRECISION .. (text_edge_fs[OPENGLES_VERSION] or text_edge_fs[2]), PRECISION .. gui_text_vs)
	s.load(shader_name.GRAY, PRECISION .. gray_fs, PRECISION .. sprite_vs)
	s.load(shader_name.COLOR, PRECISION .. color_fs, PRECISION .. sprite_vs)
	s.load(shader_name.BLEND, PRECISION .. blend_fs, PRECISION .. blend_vs)
	s.load(shader_name.RENDERBUFFER, PRECISION .. renderbuffer_fs, PRECISION_HIGH .. renderbuffer_vs)
	s.uniform_bind(shader_name.RENDERBUFFER, { { name = "st", type = uniform_format.float4} })	-- st must the first uniform (the type is float4/4)

	shader.gui_text_material = create_text_material(shader_name.GUI_TEXT)
	shader.gui_edge_material = create_text_material(shader_name.GUI_EDGE)
	shader.gui_text_material:inv_pmv(1.0,0,0,0,  0,1.0,0,0, 0,0,1.0,0, 0,0,0,1.0)
	shader.gui_edge_material:inv_pmv(1.0,0,0,0,  0,1.0,0,0, 0,0,1.0,0, 0,0,0,1.0)
end

-- 载入一个用户自定义program
-- arg.name			: shader name
-- arg.vs, arg.fs	: shader 脚本内容
-- arg.texture		: sampler2D name table
-- arg.uniform		:
function shader.define( arg )
	local name = assert(arg.name)
	local id = shader_name[name]
	if id == nil then
		assert(USER_PROGRAM < MAX_PROGRAM)
		id = USER_PROGRAM
		USER_PROGRAM = id + 1
	end

	local vs = PRECISION .. (arg.vs or sprite_vs)
	local fs = PRECISION_HIGH .. (arg.fs or sprite_fs)

	s.load(id, fs, vs, arg.texture)

	local uniform = arg.uniform
	if uniform then
		for _,v in ipairs(uniform) do
			v.type = assert(uniform_format[v.type]) -- 字符串转枚举
		end
		s.uniform_bind(id, uniform)
	end

	local r = create_shader(id, uniform)
	shader_name[name] = id

	material_meta(id, arg)
	return r
end

-- 返回指定prog的material的metatable，通过这个metatable，可以修改material的uniform和texture
function shader.material_meta(prog)
	return shader_material[prog]
end

return shader
