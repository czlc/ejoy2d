#include "png.h"
#include "texture.h"
#include "array.h"
#include "render.h"

#include <lua.h>
#include <lauxlib.h>
#include "stb_image.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct png {
	int width;
	int height;
	int comp;
	uint8_t *buffer;
};

static int
loadpng_from_file(FILE *rgba, size_t size, struct png *png) {
	png->buffer = stbi_load_from_file(rgba, &png->width, &png->height, &png->comp, 0);
	return 1;
}

static int
loadtexture(lua_State *L) {
	int id = (int)luaL_checkinteger(L,1);	// gtexid
	size_t sz = 0;
	const char * filename = luaL_checklstring(L, 2, &sz);
	ARRAY(char, tmp, sz + 5);
	sprintf(tmp, "%s.png", filename);
	FILE *rgba = fopen(tmp, "rb");
	if (rgba == NULL) {
		return luaL_error(L, "Can't open %s.png)", filename);
	}
	struct png png;

	fseek(rgba, 0, SEEK_END);
	size_t size = ftell(rgba);
	fseek(rgba, 0, SEEK_SET);
	int ok = loadpng_from_file(rgba, size, &png);
	
	if (rgba) {
		fclose(rgba);
	}
	if (!ok) {
		if (png.buffer) {
			free(png.buffer);
		}
		luaL_error(L, "Invalid file %s", filename);
	}

	int type = 0;
	switch (png.comp)
	{
	case 3:
		type = TEXTURE_RGB;
		break;
	case 4:
		type = TEXTURE_RGBA8;
		break;
	case 1:
		type = TEXTURE_A8;
		break;
	default:
		assert(NULL);
	}
 	const char * err = texture_load(id, (enum TEXTURE_FORMAT)type, png.width, png.height, png.buffer, lua_toboolean(L, 3));
 	free(png.buffer);
 	if (err) {
 		return luaL_error(L, "%s", err);
 	}

	return 0;
}

int 
ejoy2d_png(lua_State *L) {
	luaL_Reg l[] = {
		{ "texture", loadtexture },
		{ NULL, NULL },
	};

	luaL_newlib(L,l);

	return 1;
}
