#pragma once

#include <dsound.h>

void dx9_sound_init(HWND);
void dx9_sound_reset();
void dx9_sound_pause();
void dx9_sound_update();
void dx9_sound_shutdown();

extern LPDIRECTSOUND8 m_dsObject;
extern LPDIRECTSOUNDBUFFER m_dsBuffer;
extern LPDIRECTSOUNDBUFFER m_chipBuffer;
extern LPDIRECTSOUNDBUFFER m_dacBuffer;
