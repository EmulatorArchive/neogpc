#include "resource.h"
#include "video.h"
#include "video_directx.h"

// Read the INI file, scale as necessary
int WINDOW_SCALE_WIDTH [] = {160+6, 320+6, 480+6, 640+6, 800+6};
int WINDOW_SCALE_HEIGHT [] = {152+48, 304+48, 456+48, 608+48, 760+48};

// Declare our window hwnd
HWND g_hwnd;

BOOL video_init(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow, WNDPROC WndProc, int fullscreen,
	int scale)
{
    WNDCLASSEX wc;

    //Step 1: Registering the Window Class
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MYMENU);;
    wc.lpszClassName = "NeoGPC";
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    // Step 2: Creating the Window
    g_hwnd = CreateWindowEx(
        WS_EX_WINDOWEDGE,
        "NeoGPC",
        "NeoGPC - Neo Geo Pocket Color Emulator",
        WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_SCALE_WIDTH[scale-1], WINDOW_SCALE_HEIGHT[scale-1],
        NULL, NULL, hInstance, NULL);

    if(g_hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    // Initialize DirectX video driver
    dx9vid_init();

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    return TRUE;
}

void video_reset()
{
}

void video_update()
{
   // Update our pixel buffer
   dx9vid_update();
}

void video_render()
{
   dx9vid_render();
}

void video_resize(int scale)
{
	// Update win32 window
	RECT wndRect;
	::GetWindowRect(g_hwnd, &wndRect);
	::SetWindowPos(g_hwnd, NULL, wndRect.left, wndRect.top, WINDOW_SCALE_WIDTH[scale-1], WINDOW_SCALE_HEIGHT[scale-1], 0);

	// Update DirectX9 texture
	dx9vid_resize(scale);
}

void video_shutdown()
{
   dx9vid_shutdown();
}
