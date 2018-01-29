#include <windows.h>

#include <GL/glew.h>
#include <GL/wglew.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "winfw.h"

#define CLASSNAME L"EJOY"
#define WINDOWNAME L"EJOY"
#define WINDOWSTYLE (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX)

static void
set_pixel_format_to_hdc(HDC hDC)
{
	int color_deep;
	PIXELFORMATDESCRIPTOR pfd;

	color_deep = GetDeviceCaps(hDC, BITSPIXEL);
	
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType	= PFD_TYPE_RGBA;
	pfd.cColorBits	= color_deep;
	pfd.cDepthBits	= 0;
	pfd.cStencilBits = 0;
	pfd.iLayerType	= PFD_MAIN_PLANE;

	int pixelFormat = ChoosePixelFormat(hDC, &pfd);

	SetPixelFormat(hDC, pixelFormat, &pfd);
}

static void
init_window(HWND hWnd) {
	HDC hDC = GetDC(hWnd);

	set_pixel_format_to_hdc(hDC);
	HGLRC glrc = wglCreateContext(hDC);

	if (glrc == 0) {
		exit(1);
	}

	wglMakeCurrent(hDC, glrc);

	if ( glewInit() != GLEW_OK ) {
		exit(1);
	}

	glViewport(0, 0, WIDTH, HEIGHT);

	ReleaseDC(hWnd, hDC);
}

static void
update_frame(HDC hDC) {
	ejoy2d_win_frame();
	SwapBuffers(hDC);

}

static void
get_xy(LPARAM lParam, int *x, int *y) {
	*x = (short)(lParam & 0xffff); 
	*y = (short)((lParam>>16) & 0xffff); 
}

#define UNI_SHIFT 10
#define UNI_BASE 0x0010000
#define UNI_MASK 0x3FF
#define UNI_SUR_HIGH_START 0xD800
#define UNI_SUR_LOW_START 0xDC00

static void
utf16to8(const uint16_t utf16[2], uint8_t utf8[4]) {
	uint32_t ch = utf16[0];
	if ((utf16[0] & UNI_SUR_HIGH_START) == UNI_SUR_HIGH_START) {
		ch = (utf16[0] & UNI_MASK) << UNI_SHIFT;
		ch += utf16[1] & UNI_MASK;
		ch += UNI_BASE;
	}

	if (ch <= 0x7F) {
		utf8[0] = ch;
	}
	else if (ch <= 0x7FF) {
		utf8[0] = (ch >> 6) | 0xC0;
		utf8[1] = (ch & 0x3F) | 0x80;
	}
	else if (ch <= 0xFFFF) {
		utf8[0] = (ch >> 12) | 0xE0;
		utf8[1] = ((ch >> 6) & 0x3F) | 0x80;
		utf8[2] = (ch & 0x3F) | 0x80;
	}
	else if (ch <= 0x1FFFFF) {
		utf8[0] = (ch >> 18) | 0xF0;
		utf8[1] = ((ch >> 12) & 0x3F) | 0x80;
		utf8[2] = ((ch >> 6) & 0x3F) | 0x80;
		utf8[3] = (ch & 0x3F) | 0x80;
	}
}

static LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE:
		init_window(hWnd);
		SetTimer(hWnd,0,10,NULL);
		break;
	case WM_PAINT: {
		if (GetUpdateRect(hWnd, NULL, FALSE)) {
			HDC hDC = GetDC(hWnd);
			update_frame(hDC);
			ValidateRect(hWnd, NULL);
			ReleaseDC(hWnd, hDC);
		}
		return 0;
	}
	case WM_TIMER : {
		static DWORD last = timeGetTime();
		DWORD curr = timeGetTime();
		ejoy2d_win_update(curr - last);
		last = curr;
		InvalidateRect(hWnd, NULL , FALSE);
		break;
	}
	case WM_DESTROY:
		ejoy2d_win_close();
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONUP: {
		int x,y;
		get_xy(lParam, &x, &y); 
		ejoy2d_win_touch(x,y,TOUCH_END);
		break;
	}
	case WM_LBUTTONDOWN: {
		int x,y;
		get_xy(lParam, &x, &y); 
		ejoy2d_win_touch(x,y,TOUCH_BEGIN);
		break;
	}
	case WM_MOUSEMOVE: {
		int x,y;
		get_xy(lParam, &x, &y); 
		ejoy2d_win_touch(x,y,TOUCH_MOVE);
		break;
	}
	case WM_CHAR: {
		uint16_t utf16[2] = { wParam,  0};
		uint8_t utf8[4] = { 0 };
		utf16to8(utf16, utf8);
		ejoy2d_win_char((char *)utf8);
		break;
	}
	}
	return DefWindowProcW(hWnd, message, wParam, lParam);
}

static void
register_class()
{
	WNDCLASSW wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = GetModuleHandleW(0);
	wndclass.hIcon = 0;
	wndclass.hCursor = 0;
	wndclass.hbrBackground = 0;
	wndclass.lpszMenuName = 0; 
	wndclass.lpszClassName = CLASSNAME;

	RegisterClassW(&wndclass);
}

static HWND
create_window(int w, int h) {
	RECT rect;

	rect.left=0;
	rect.right=w;
	rect.top=0;
	rect.bottom=h;

	AdjustWindowRect(&rect,WINDOWSTYLE,0);

	HWND wnd=CreateWindowExW(0,CLASSNAME,WINDOWNAME,
		WINDOWSTYLE, CW_USEDEFAULT,0,
		rect.right-rect.left,rect.bottom-rect.top,
		0,0,
		GetModuleHandleW(0),
		0);

	return wnd;
}

int
main(int argc, char *argv[]) {
	register_class();
	HWND wnd = create_window(WIDTH,HEIGHT);
	ejoy2d_win_init(argc, argv);

	ShowWindow(wnd, SW_SHOWDEFAULT);
	UpdateWindow(wnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
