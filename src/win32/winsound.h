#pragma once

#include <Windows.h>

#define SOUND_DIRECT9 0
#define SOUND_OPENAL 1

void win_sound_init(HWND);
void win_sound_reset();
void win_sound_pause();
void win_sound_update();
void win_sound_shutdown();