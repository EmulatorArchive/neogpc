#include "input.h"

#include "..\win32\input_directx.h"

void input_init(HINSTANCE hInst, HWND hWnd)
{
   dx9_input_init(hInst, hWnd);
}

void input_reset()
{
}

void input_update()
{
	dx9_input_update();
}

void input_config(int i)
{
	dx9_input_config(i);
}

void input_shutdown()
{
   dx9_input_shutdown();
}
