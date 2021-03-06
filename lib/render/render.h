#ifndef ejoy3d_render_h
#define ejoy3d_render_h

#include <stdint.h>

typedef unsigned int RID;	/* 屏蔽了 opengl 层的 id */

struct render;

struct render_init_args {
	int max_buffer;
	int max_layout;
	int max_target;
	int max_texture;
	int max_shader;
};

/* 顶点的一个attrib */
struct vertex_attrib {
	const char * name;	/* attrib name和shader脚本中的对应 */
	int vbslot;			/* 顶点attrib对应的buffer
						   对应于struct render.vbslot[vbslot]，得到RID再到buffer array中去找具体的buffer数据
						   之所以不直接存RID而是多一个间接的render.vbslot[vbslot]是因为考虑到每一个slot都有意义
						   默认的slot为0，目前似乎也只有这一个vb */
	int n;				/* item count，比如color为4 */
	int size;			/* item size(byte)，比如color为sizeof(uint8_t) */
	int offset;			/* 此attrib在整个vertex结构中的偏移 */
};

struct shader_init_args {
	const char * vs;
	const char * fs;
	int texture;					/* texture_uniform count */
	const char **texture_uniform;	/* 脚本中sample名字 */
};

enum RENDER_OBJ {
	INVALID = 0,
	VERTEXLAYOUT = 1,
	VERTEXBUFFER = 2,
	INDEXBUFFER = 3,
	TEXTURE = 4,
	TARGET = 5,
	SHADER = 6,
};

enum TEXTURE_TYPE {
	TEXTURE_2D = 0,
	TEXTURE_CUBE,
};

enum TEXTURE_FORMAT {
	TEXTURE_INVALID = 0,
	TEXTURE_RGBA8,
	TEXTURE_RGBA4,
	TEXTURE_RGB,
	TEXTURE_RGB565,
	TEXTURE_A8,
	TEXTURE_DEPTH,	// use for render target
	TEXTURE_PVR2,
	TEXTURE_PVR4,
	TEXTURE_ETC1,
};

enum BLEND_FORMAT {
	BLEND_DISABLE = 0,
	BLEND_ZERO,
	BLEND_ONE,
	BLEND_SRC_COLOR,
	BLEND_ONE_MINUS_SRC_COLOR,
	BLEND_SRC_ALPHA,
	BLEND_ONE_MINUS_SRC_ALPHA,
	BLEND_DST_ALPHA,
	BLEND_ONE_MINUS_DST_ALPHA,
	BLEND_DST_COLOR,
	BLEND_ONE_MINUS_DST_COLOR,
	BLEND_SRC_ALPHA_SATURATE,
};

enum DEPTH_FORMAT {
	DEPTH_DISABLE = 0,
	DEPTH_LESS_EQUAL,
	DEPTH_LESS,
	DEPTH_EQUAL,
	DEPTH_GREATER,
	DEPTH_GREATER_EQUAL,
	DEPTH_ALWAYS,
};

enum CLEAR_MASK {
	MASKC = 0x1,
	MASKD = 0x2,
	MASKS = 0x4,
};

enum UNIFORM_FORMAT {
	UNIFORM_INVALID = 0,
	UNIFORM_FLOAT1,
	UNIFORM_FLOAT2,
	UNIFORM_FLOAT3,
	UNIFORM_FLOAT4,
	UNIFORM_FLOAT33,
	UNIFORM_FLOAT44,
};

enum DRAW_MODE {
	DRAW_TRIANGLE = 0,
	DRAW_LINE,
};

enum CULL_MODE {
	CULL_DISABLE = 0,
	CULL_FRONT,
	CULL_BACK,
};

int render_version(struct render *R);
int render_size(struct render_init_args *args);
struct render * render_init(struct render_init_args *args, void * buffer, int sz);
void render_exit(struct render * R);

/*
** Q:liuchang 
*/
void render_set(struct render *R, enum RENDER_OBJ what, RID id, int slot);
void render_release(struct render *R, enum RENDER_OBJ what, RID id);

/* 记录一类顶点的attrib信息 */
RID render_register_vertexlayout(struct render *R, int n, struct vertex_attrib * attrib);

/*
** 创建顶点或者索引缓存
**	what:VERTEXBUFFER or INDEXBUFFER
**	data:初始数据
**	n:item count
**	stride:item size，一个顶点sizeof(struct vertex)或者索引sizeof(uint16_t)的大小
*/
RID render_buffer_create(struct render *R, enum RENDER_OBJ what, const void *data, int n, int stride);
/*
** 更新顶点或者索引缓存
*/
void render_buffer_update(struct render *R, RID id, const void * data, int n);

/*
** 创建一个纹理对象
*/
RID render_texture_create(struct render *R, int width, int height, enum TEXTURE_FORMAT format, enum TEXTURE_TYPE type, int mipmap);
/*
** 填充纹理数据
*/
void render_texture_update(struct render *R, RID id, int width, int height, const void *pixels, int slice, int miplevel);
// subupdate only support slice 0, miplevel 0
void render_texture_subupdate(struct render *R, RID id, const void *pixels, int x, int y, int w, int h);

/*
** 创建一个渲染目标(attach texture)
*/
RID render_target_create(struct render *R, int width, int height, enum TEXTURE_FORMAT format);
// render_release TARGET would not release the texture attachment
RID render_target_texture(struct render *R, RID rt);

/*
** 创建一个program对象
*/
RID render_shader_create(struct render *R, struct shader_init_args *args);
void render_shader_bind(struct render *R, RID id);
/*
**获得获得uniform的location(id)
*/
int render_shader_locuniform(struct render *R, const char * name);
/*
** 设置location指定的uniform的值
*/
void render_shader_setuniform(struct render *R, int loc, enum UNIFORM_FORMAT format, const float *v);

void render_setviewport(struct render *R, int x, int y, int width, int height );
void render_setscissor(struct render *R, int x, int y, int width, int height );

void render_setblend(struct render *R, enum BLEND_FORMAT src, enum BLEND_FORMAT dst);
void render_setdepth(struct render *R, enum DEPTH_FORMAT d);
void render_setcull(struct render *R, enum CULL_MODE c);
void render_enabledepthmask(struct render *R, int enable);
void render_enablescissor(struct render *R, int enable);

void render_state_reset(struct render *R);

void render_clear(struct render *R, enum CLEAR_MASK mask, unsigned long argb);
void render_draw(struct render *R, enum DRAW_MODE mode, int fromidx, int ni);

#endif
