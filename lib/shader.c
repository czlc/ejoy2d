/*http://blog.codingnow.com/2014/09/ejoy2d_shader.html */

#include "shader.h"
#include "material.h"
#include "fault.h"
#include "array.h"
#include "renderbuffer.h"
#include "texture.h"
#include "matrix.h"
#include "spritepack.h"
#include "screen.h"
#include "label.h"

#include "render.h"
#include "blendmode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define MAX_PROGRAM 16

#define BUFFER_OFFSET(f) ((intptr_t)&(((struct vertex *)NULL)->f))

#define MAX_UNIFORM 16
#define MAX_TEXTURE_CHANNEL 8

struct uniform {
	int loc;
	int offset;					/* 此uniform数据在缓存(program.uniform_value or material.uniform)中的偏移量 */
	enum UNIFORM_FORMAT type;
};

/* 共有16个program对象 ,上层使用它，通过它来操作对应的shader对象 */
struct program {
	RID prog;								/* 关联的shader对象，见render.c */
	struct material * material;				/* 当前的material，它通常来自各个sprite，在此记录的目的是如果先后2次绘制material相同就不用apply_material，并无其它作用，只是判断是否需要 apply */
	int texture_number;						/* 此program中sample2D 个数 */
	int uniform_number;						/* 有效uniform 数*/
	struct uniform uniform[MAX_UNIFORM];	/* uniform 数组，描述了 uniform 情况 */
	bool reset_uniform;						/* uniform 是否有修改 */
	bool uniform_change[MAX_UNIFORM];		/* 标志具体哪些uniform有修改 */
	float uniform_value[MAX_UNIFORM * 16];	/* 全局uniform，如果有material则会被覆盖 */
};

struct render_state {
	struct render * R;
	int current_program;					// 当前program template id
	struct program program[MAX_PROGRAM];	// 16个shader program template，7个系统，9个用户自定义
	RID tex[MAX_TEXTURE_CHANNEL];			// 每个channel 相当于一支独立的笔
	int blendchange;						// 混合参数改变不再是默认值
	int drawcall;
	RID vertex_buffer;
	RID index_buffer;						// index buffer
	RID layout;								// vertex attribute 在数组(render.attrib)中的位置
	struct render_buffer vb;
};

static struct render_state *RS = NULL;

void lsprite_initrender(struct render *r);

void
shader_init() {
	if (RS) return;

	struct render_state * rs = (struct render_state *) malloc(sizeof(*rs));
	memset(rs, 0 , sizeof(*rs));

	struct render_init_args RA;
	// todo: config these args
	RA.max_buffer = 128;
	RA.max_layout = 4;
	RA.max_target = 128;
	RA.max_texture = 256;
	RA.max_shader = MAX_PROGRAM;

	int rsz = render_size(&RA);
	rs->R = (struct render *)malloc(rsz);
	rs->R = render_init(&RA, rs->R, rsz);
	texture_initrender(rs->R);
	screen_initrender(rs->R);
	label_initrender(rs->R);
	lsprite_initrender(rs->R);
	renderbuffer_initrender(rs->R);

	rs->current_program = -1;
	rs->blendchange = 0;
	render_setblend(rs->R, BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);

	uint16_t idxs[6 * MAX_COMMBINE];
	int i;
	for (i=0;i<MAX_COMMBINE;i++) {
		idxs[i*6] = i*4;
		idxs[i*6+1] = i*4+1;
		idxs[i*6+2] = i*4+2;
		idxs[i*6+3] = i*4;
		idxs[i*6+4] = i*4+2;
		idxs[i*6+5] = i*4+3;
	}
	
	rs->index_buffer = render_buffer_create(rs->R, INDEXBUFFER, idxs, 6 * MAX_COMMBINE, sizeof(uint16_t));
	rs->vertex_buffer = render_buffer_create(rs->R, VERTEXBUFFER, NULL,  4 * MAX_COMMBINE, sizeof(struct vertex));

	struct vertex_attrib va[4] = {
		{ "position", 0, 2, sizeof(float), BUFFER_OFFSET(vp.vx) },
		{ "texcoord", 0, 2, sizeof(uint16_t), BUFFER_OFFSET(vp.tx) },
		{ "color", 0, 4, sizeof(uint8_t), BUFFER_OFFSET(rgba) },
		{ "additive", 0, 4, sizeof(uint8_t), BUFFER_OFFSET(add) },
	};
	rs->layout = render_register_vertexlayout(rs->R, sizeof(va)/sizeof(va[0]), va);
	render_set(rs->R, VERTEXLAYOUT, rs->layout, 0);
	render_set(rs->R, INDEXBUFFER, rs->index_buffer, 0);
	render_set(rs->R, VERTEXBUFFER, rs->vertex_buffer, 0); /* 使用0号slot */

	RS = rs;
}

void
shader_reset() {
	struct render_state *rs = RS;
	render_state_reset(rs->R);
	render_setblend(rs->R, BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);
	if (RS->current_program != -1) {
		render_shader_bind(rs->R, RS->program[RS->current_program].prog);
	}
	render_set(rs->R, VERTEXLAYOUT, rs->layout, 0);
	render_set(rs->R, TEXTURE, RS->tex[0], 0);
	render_set(rs->R, INDEXBUFFER, RS->index_buffer,0);
	render_set(rs->R, VERTEXBUFFER, RS->vertex_buffer,0);
}

static void
program_init(struct program * p, const char *FS, const char *VS, int texture, const char ** texture_uniform_name) {
	struct render *R = RS->R;
	memset(p, 0, sizeof(*p));
	struct shader_init_args args;
	args.vs = VS;
	args.fs = FS;
	args.texture = texture;
	args.texture_uniform = texture_uniform_name;
	p->prog = render_shader_create(R, &args);
 	render_shader_bind(R, p->prog); /* Q:liuchang 这2句似乎没什么意义? */
 	render_shader_bind(R, 0);
}

void 
shader_load(int prog, const char *fs, const char *vs, int texture, const char ** texture_uniform_name) {
	struct render_state *rs = RS;
	assert(prog >=0 && prog < MAX_PROGRAM);
	struct program * p = &rs->program[prog];
	if (p->prog) {
		render_release(RS->R, SHADER, p->prog);
		p->prog = 0;
	}
	program_init(p, fs, vs, texture, texture_uniform_name);
	p->texture_number = texture;
	RS->current_program = -1;
}

void 
shader_unload() {
	if (RS == NULL) {
		return;
	}
	struct render *R = RS->R;
	texture_initrender(NULL);
	screen_initrender(NULL);
	label_initrender(NULL);
	lsprite_initrender(NULL);
	renderbuffer_initrender(NULL);

	render_exit(R);
	free(R);
	free(RS);
	RS = NULL;
}

void
reset_drawcall_count() {
	if (RS) {
		RS->drawcall = 0;
	}
}

int
drawcall_count() {
	if (RS) {
		return RS->drawcall;
	} else {
		return 0;
	}
}

static void 
renderbuffer_commit(struct render_buffer * rb) {
	struct render *R = RS->R;
	render_draw(R, DRAW_TRIANGLE, 0, 6 * rb->object);
}

// 任何渲染状态改变都应该调用
static void
rs_commit() {
	struct render_buffer * rb = &(RS->vb);
	if (rb->object == 0)
		return;
	RS->drawcall++;
	struct render *R = RS->R;
	render_buffer_update(R, RS->vertex_buffer, rb->vb, 4 * rb->object);
	renderbuffer_commit(rb);

	rb->object = 0;
}

void 
shader_drawbuffer(struct render_buffer * rb, float tx, float ty, float scale) {
	rs_commit();	// 绘制新的drawbuffer的时候需要把之前的绘制命令提交

	RID glid = texture_glid(rb->texid);
	if (glid == 0)
		return;
	shader_texture(glid, 0);
	render_set(RS->R, VERTEXBUFFER, rb->vbid, 0);

	float sx = scale;
	float sy = scale;
	screen_trans(&sx, &sy);
	screen_trans(&tx, &ty);
	float v[4] = { sx, sy, tx, ty };

	// we should call shader_adduniform to add "st" uniform first
	shader_setuniform(PROGRAM_RENDERBUFFER, 0, UNIFORM_FLOAT4, v);

	shader_program(PROGRAM_RENDERBUFFER, NULL);
	RS->drawcall++;

	renderbuffer_commit(rb);

	render_set(RS->R, VERTEXBUFFER, RS->vertex_buffer, 0);
}

// 改变某个通道的纹理，id为otexid
void
shader_texture(int id, int channel) {
	assert(channel < MAX_TEXTURE_CHANNEL);
	if (RS->tex[channel] != id) {
		rs_commit();
		RS->tex[channel] = id;
		render_set(RS->R, TEXTURE, id, channel);
	}
}

static void
apply_uniform(struct program *p) {
	struct render *R = RS->R;
	int i;
	for (i=0;i<p->uniform_number;i++) {
		if (p->uniform_change[i]) {
			struct uniform * u = &p->uniform[i];
			if (u->loc >=0) 
				render_shader_setuniform(R, u->loc, u->type, p->uniform_value + u->offset);
		}
	}
	p->reset_uniform = false;
}

void
shader_program(int n, struct material *m) {
	struct program *p = &RS->program[n];
	if (RS->current_program != n || p->reset_uniform || m) {	/* Q:liuchang 带有m的相同的spr多次绘制也会每次commit，效率不高 */
		rs_commit();
	}
	/* 先应用全局uniform，如果sprite的material也有自己的uniform再覆盖 */
	if (RS->current_program != n) {
		RS->current_program = n;
		render_shader_bind(RS->R, p->prog);
		p->material = NULL;
		apply_uniform(p);
	} else if (p->reset_uniform) {
		apply_uniform(p);
	}
	if (m) {
		material_apply(n, m);
	}
}

void
shader_draw(const struct vertex_pack vb[4], uint32_t color, uint32_t additive) {
	if (renderbuffer_add(&RS->vb, vb, color, additive)) {
		rs_commit();
	}
}

static void
draw_quad(const struct vertex_pack *vbp, uint32_t color, uint32_t additive, int max, int index) {
	struct vertex_pack vb[4];
	int i;
	vb[0] = vbp[0];	// first point
	for (i=1;i<4;i++) {
		int j = i + index;
		int n = (j <= max) ? j : max;
		vb[i] = vbp[n];
	}
	shader_draw(vb, color, additive);
}

void
shader_drawpolygon(int n, const struct vertex_pack *vb, uint32_t color, uint32_t additive) {
	int i = 0;
	--n;
	do {
		draw_quad(vb, color, additive, n, i);
		i+=2;
	} while (i<n-1);
}

void 
shader_flush() {
	rs_commit();
}

void
shader_defaultblend() {
	if (RS->blendchange) {
		rs_commit();
		RS->blendchange = 0;
		render_setblend(RS->R, BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);
	}
}

void
shader_blend(int m1, int m2) {
	if (m1 != BLEND_GL_ONE || m2 != BLEND_GL_ONE_MINUS_SRC_ALPHA) {
		// 变回来得用shader_defaultblend，否则有问题
		rs_commit();
		RS->blendchange = 1;
		enum BLEND_FORMAT src = blend_mode(m1);
		enum BLEND_FORMAT dst = blend_mode(m2);
		render_setblend(RS->R, src, dst);
	}
}

void 
shader_clear(unsigned long argb) {
	render_clear(RS->R, MASKC, argb);
}

int 
shader_version() {
	return render_version(RS->R);
}

void 
shader_scissortest(int enable) {
	render_enablescissor(RS->R, enable);
}

int 
shader_uniformsize(enum UNIFORM_FORMAT t) {
	int n = 0;
	switch(t) {
	case UNIFORM_INVALID:
		n = 0;
		break;
	case UNIFORM_FLOAT1:
		n = 1;
		break;
	case UNIFORM_FLOAT2:
		n = 2;
		break;
	case UNIFORM_FLOAT3:
		n = 3;
		break;
	case UNIFORM_FLOAT4:
		n = 4;
		break;
	case UNIFORM_FLOAT33:
		n = 9;
		break;
	case UNIFORM_FLOAT44:
		n = 16;
		break;
	}
	return n;
}

void 
shader_setuniform(int prog, int index, enum UNIFORM_FORMAT t, float *v) {
	rs_commit();
	struct program * p = &RS->program[prog];
	assert(index >= 0 && index < p->uniform_number);
	struct uniform *u = &p->uniform[index];
	assert(t == u->type);
	int n = shader_uniformsize(t);
	memcpy(p->uniform_value + u->offset, v, n * sizeof(float));
	p->reset_uniform = true;
	p->uniform_change[index] = true;
}

int 
shader_adduniform(int prog, const char * name, enum UNIFORM_FORMAT t) {
	// reset current_program
	assert(prog >=0 && prog < MAX_PROGRAM);
	shader_program(prog, NULL);
	struct program * p = &RS->program[prog];
	assert(p->uniform_number < MAX_UNIFORM);
	int loc = render_shader_locuniform(RS->R, name); // 获得uniform location
	int index = p->uniform_number++;
	struct uniform * u = &p->uniform[index];
	u->loc = loc;
	u->type = t;
	if (index == 0) {
		u->offset = 0;
	} else {
		struct uniform * lu = &p->uniform[index-1];
		u->offset = lu->offset + shader_uniformsize(lu->type);
	}
	if (loc < 0)
		return -1;
	return index;
}

// material system

struct material {
	struct program *p;
	int texture[MAX_TEXTURE_CHANNEL];	/* 保存的是gtexid */
	bool uniform_enable[MAX_UNIFORM];	/* 记录material中哪些uniform是有效的 */
	float uniform[1];					/* 存储uniform数据 */
	bool reset;							/* material 数据是否需要刷新，纹理或者 uniform 被修改的时候要改 */
};

/* 获得一个prog对应material的大小，因为prog定了，uniform也就定下来了，能通过material修改的也有限*/
int 
material_size(int prog) {
	if (prog < 0 || prog >= MAX_PROGRAM)
		return 0;
	struct program *p = &RS->program[prog];
	if (p->uniform_number == 0 && p->texture_number == 0) {
		return 0;
	}
	struct uniform * lu = &p->uniform[p->uniform_number-1];
	int total = lu->offset + shader_uniformsize(lu->type);
	return sizeof(struct material) + (total-1) * sizeof(float);
}

struct material * 
material_init(void *self, int size, int prog) {
	int rsz = material_size(prog);
	struct program *p = &RS->program[prog];
	assert(size >= rsz);
	memset(self, 0, rsz);
	struct material * m = (struct material *)self;
	m->p = p;
	m->reset = false;
	int i;
	for (i=0;i<MAX_TEXTURE_CHANNEL;i++) {
		m->texture[i] = -1;
	}

	return m;
}

// index 为uniform loc
int 
material_setuniform(struct material *m, int index, int n, const float *v) {
	struct program * p = m->p;
	assert(index >= 0 && index < p->uniform_number);
	struct uniform * u = &p->uniform[index];
	if (shader_uniformsize(u->type) != n) {
		return 1;
	}
	memcpy(m->uniform + u->offset, v, n * sizeof(float));
	m->uniform_enable[index] = true;
	m->reset = true;
	return 0;
}

void 
material_apply(int prog, struct material *m) {
	struct program * p = m->p;
	if (p != &RS->program[prog])
		return;
	if (p->material == m && !m->reset) {
		return;
	}
	m->reset = false;
	p->material = m;
	p->reset_uniform = true;
	int i;
	for (i=0;i<p->uniform_number;i++) {
		if (m->uniform_enable[i]) {
			struct uniform * u = &p->uniform[i];
			if (u->loc >=0) {
				render_shader_setuniform(RS->R, u->loc, u->type, m->uniform + u->offset);
			}
		}
	}
	for (i=0;i<p->texture_number;i++) {
		int tex = m->texture[i];
		if (tex >= 0) {
			RID glid = texture_glid(tex);
			if (glid) {
				shader_texture(glid, i);
			}
		}
	}
}

int
material_settexture(struct material *m, int channel, int texture) {
	if (channel >= MAX_TEXTURE_CHANNEL) {
		return 1;
	}
	m->texture[channel] = texture;
	m->reset = true;
	return 0;
}
