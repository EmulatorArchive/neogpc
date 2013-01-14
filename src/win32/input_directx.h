#pragma once

#define DIRECTINPUT_VERSION 0x0800

#include <Windows.h>
#include <dinput.h>

void dx9_input_init(HINSTANCE, HWND);
void dx9_input_reset();
void dx9_input_update();
void dx9_input_config(int);
void dx9_input_shutdown();

extern LPDIRECTINPUT8 m_diObject;          // DirectInput object
extern LPDIRECTINPUTDEVICE8 m_diDevice;    // DirectInput device

extern char m_buffer[256];
extern unsigned char * inputByte;
