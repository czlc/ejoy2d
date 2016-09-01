/* shader 是对绘制的简单封装 */
#ifndef EJOY_2D_SHADER_H
#define EJOY_2D_SHADER_H

#include "renderbuffer.h"
#include "render.h"

#include <stdint.h>
#include <lua.h>

#define PROGRAM_DEFAULT -1
#define PROGRAM_PICTURE 0
#define PROGRAM_RENDERBUFFER 1
#define PROGRAM_TEXT 2
#define PROGRAM_TEXT_EDGE 3
#define PROGRAM_GUI_TEXT 4
#define PROGRAM_GUI_EDGE 5

struct material;

void shader_init();
/* 
** 编译指定id 的 program
** prog: program id
** texture:sampler2D count
** texture_uniform_name:uniform sampler2D 的名字列表
*/
void shader_load(int prog, const char *fs, const char *vs, int texture, const char ** texture_uniform_name);
void shader_unload();
void shader_blend(int m1,int m2);
void shader_defaultblend();
/*
** 改变某个通道的纹理
** id 为 texture的RID
*/
void shader_texture(int id, int channel);
/*
** 绘制一个quad
*/
void shader_draw(const struct vertex_pack vb[4],uint32_t color,uint32_t additive);
/*
** 绘制一个多边形
*/
void shader_drawpolygon(int n, const struct vertex_pack *vb, uint32_t color, uint32_t additive, int qn, uint16_t *quad);
/* 
** 设置当前program和material，通常是绘制sprite的时候由sprite带来
** n:program template id
*/
void shader_program(int n, struct material *);
void shader_flush();
void shader_clear(unsigned long argb);
int shader_version();
void shader_scissortest(int enable);

int ejoy2d_shader(lua_State *L);

/*
	绘制一个render_buffer，地图tile用到
*/
void shader_drawbuffer(struct render_buffer * rb, float x, float y, float s);

/*
	desc：载入shader中的uniform变量
	prog:program template id，范围为[0,MAX_PROGRAM)
*/
int shader_adduniform(int prog, const char * name, enum UNIFORM_FORMAT t);
/* desc:设置uniform值 */
void shader_setuniform(int prog, int index, enum UNIFORM_FORMAT t, float *v);
int shader_uniformsize(enum UNIFORM_FORMAT t);

// these api may deprecated later
void shader_reset();
void shader_mask(float x, float y);
void reset_drawcall_count();
int drawcall_count();

#endif
