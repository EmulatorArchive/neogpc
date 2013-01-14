#include "sound.h"

#include "sound_directx.h"

void win_sound_init(HWND hWnd)
{
   dx9_sound_init(hWnd);
}

void win_sound_reset()
{
	dx9_sound_reset();
}

void win_sound_pause()
{
	dx9_sound_pause();
}

void win_sound_update()
{
   dx9_sound_update();
}

void win_sound_shutdown()
{
   dx9_sound_shutdown();
}
