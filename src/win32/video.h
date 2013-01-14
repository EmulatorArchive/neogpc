#pragma once

#include <Windows.h>

#define VIDEO_DIRECT9 1
#define VIDEO_OPENGL 0

// Window Scale (X)
extern int WINDOW_SCALE_WIDTH [];
extern int WINDOW_SCALE_HEIGHT [];

extern HWND g_hwnd;

BOOL video_init(HINSTANCE, HINSTANCE, LPSTR, int, WNDPROC, int, int);
void video_reset();
void video_update();
void video_render();
void video_resize(int);
void video_shutdown();
