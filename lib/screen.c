#include "screen.h"
#include "render.h"
#include "spritepack.h"

struct screen {
	int width;
	int height;
	float scale;
	float invw;
	float invh;
};

static struct screen SCREEN;
static struct render *R = NULL;

void 
screen_initrender(struct render *r) {
	R = r;
	// for ejoy2d compatibility, ejoy2d may call screen_init before screen_initrender
	screen_init(SCREEN.width, SCREEN.height, SCREEN.scale);
}

void
screen_init(float w, float h, float scale) {
	SCREEN.width = (int)w;
	SCREEN.height = (int)h;
	SCREEN.scale = scale;
	SCREEN.invw = 2.0f / SCREEN_SCALE / w;	// 屏幕坐标取值范围[-1,1]，所以长宽绝对值为2，这里invw用于把逻辑像素转成屏幕坐标
	SCREEN.invh = -2.0f / SCREEN_SCALE / h;
	if (R) {
		render_setviewport(R, 0, 0, w * scale, h * scale );
	}
}

// 把逻辑像素值转成屏幕坐标[-1, 1]
void
screen_trans(float *x, float *y) {
	*x *= SCREEN.invw;
	*y *= SCREEN.invh;
}

void
screen_scissor(int x, int y, int w, int h) {
	y = SCREEN.height - y - h;
	if (x<0) {
		w += x;
		x = 0;
	} else if (x>SCREEN.width) {
		w=0;
		h=0;
	}
	if (y<0) {
		h += y;
		y = 0;
	} else if (y>SCREEN.height) {
		w=0;
		h=0;
	}
	if (w<=0 || h<=0) {
		w=0;
		h=0;
	}
	x *= SCREEN.scale;
	y *= SCREEN.scale;
	w *= SCREEN.scale;
	h *= SCREEN.scale;

	render_setscissor(R,x,y,w,h);
}

bool screen_is_visible(float x,float y)
{
	return x >= 0.0f && x <= 2.0f && y>=-2.0f && y<= 0.0f;
}
bool screen_is_poly_invisible(const float* points,int len,int stride)
{
	int i =0;
	// test left of x
	bool invisible = true;
	for(i =0; i < len && invisible;++i)
	{
		if(points[i*stride] >= 0.0f)
			invisible = false;
	}
	if(invisible)
		return true;
	
	// test right of axis x
	invisible = true;
	for(i =0; i < len && invisible;++i)
	{
		if(points[i*stride] <= 2.0f)
			invisible = false;
	}
	if(invisible)
		return true;

	// test above of axis y
	invisible = true;
	for(i =0; i < len && invisible;++i)
	{
		if(points[i*stride +1] >= -2.0f)
			invisible = false;
	}
	if(invisible)
		return true;
	
	// test below of axis y
	invisible = true;
	for(i =0; i < len && invisible;++i)
	{
		if(points[i*stride +1] <= 0.0f)
			invisible = false;
	}
	return invisible;
}


