#include "png.h"
#include "texture.h"
#include "array.h"
#include "render.h"

#include <lua.h>
#include <lauxlib.h>
#include "libpng/include/png.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct png {
	int type;
	int depth;
	int width;
	int height;
	uint8_t *buffer;
};


static int
loadpng_from_file(FILE *rgba, uint8_t *buffer, size_t size, struct png *png) {

	/* first do a quick check that the file really is a PNG image; could
     * have used slightly more general png_sig_cmp() function instead */
	unsigned char sig[8];
    fread(sig, 1, 8, rgba);
    if (!png_check_sig(sig, 8)) {
        return 0;   /* bad signature */
	}

	/* could pass pointers to user-defined error handlers instead of NULLs: */
	png_structp png_ptr = NULL;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		return 0;
	}
	png_infop info_ptr = NULL;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return 0;
	}

	 /* we could create a second info struct here (end_info), but it's only
     * useful if we want to keep pre- and post-IDAT chunk info separated
     * (mainly for PNG-aware image editors and converters) */


    /* setjmp() must be called in every function that calls a PNG-reading
     * libpng function */

	if (setjmp(png_jmpbuf(png_ptr)) != 0) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return 0;
	}

	png_init_io(png_ptr, rgba);
	png_set_sig_bytes(png_ptr, 8);  /* we already read the 8 signature bytes */

	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *)&png->width, (png_uint_32 *)&png->height, &png->depth, &png->type,
		NULL, NULL, NULL);

	if (png->type == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png_ptr);
	if (png->type == PNG_COLOR_TYPE_GRAY && png->depth < 8)
		png_set_expand(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_ptr);
	if (png->depth == 16)
		png_set_strip_16(png_ptr);
	if (png->type == PNG_COLOR_TYPE_GRAY ||
		png->type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	    /* unlike the example in the libpng documentation, we have *no* idea where
     * this file may have come from--so if it doesn't have a file gamma, don't
     * do any correction ("do no harm") */

	double exp = 1.0 * 2.2;
	char *display_exponent = getenv("SCREEN_GAMMA");
	if (display_exponent)
		exp = atof(display_exponent);

	double  gamma;
    if (png_get_gAMA(png_ptr, info_ptr, &gamma))
        png_set_gamma(png_ptr, exp, gamma);

	// Update the png info struct.
	png_read_update_info(png_ptr, info_ptr);

	// Row size in bytes.
	png_size_t row_byte = png_get_rowbytes(png_ptr, info_ptr);
	png_get_channels(png_ptr, info_ptr);

	png->buffer = (uint8_t *)malloc(row_byte * png->height);
	if (!png->buffer) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return 0;
	}

	png_bytep* row_point = (png_bytep*)malloc(sizeof(png_bytep) * png->height);
	if (!row_point) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return 0;
	}

	 /* set the individual row_pointers to point at the correct offsets */
	int i;
	for (i = 0; i < png->height; ++i) {
		row_point[i] = png->buffer + i * row_byte;
	}

	/* now we can go ahead and just read the whole image */
	png_read_image(png_ptr, row_point);

	/* and we're done!  (png_read_end() can be omitted if no processing of
     * post-IDAT text/time/etc. is desired) */

	free(row_point);
	row_point = NULL;

	png_read_end(png_ptr, NULL);

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
	uint8_t *buffer = (uint8_t *)malloc(size);
	int ok = loadpng_from_file(rgba, buffer, size, &png);
	
	if (buffer) {
		free(buffer);
	}
	if (rgba) {
		fclose(rgba);
	}
	if (!ok) {
		if (png.buffer) {
			free(png.buffer);
		}
		luaL_error(L, "Invalid file %s", filename);
	}

	assert(png.depth == 8);
 	int type = 0;
	switch (png.type)
	{
	case PNG_COLOR_TYPE_RGB:
		type = TEXTURE_RGB;
		break;
	case PNG_COLOR_TYPE_RGBA:
	case PNG_COLOR_TYPE_PALETTE:
		type = TEXTURE_RGBA8;
		break;
	case PNG_COLOR_TYPE_GA:
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
