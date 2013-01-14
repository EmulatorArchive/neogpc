#pragma once

#include <Windows.h>

#define INPUT_DIRECT9 0
#define INPUT_RAW 1

enum
{
	INPUT_UP = 0,
	INPUT_LEFT,
	INPUT_DOWN,
	INPUT_RIGHT,
	INPUT_A,
	INPUT_B,
	INPUT_START
};

void input_init(HINSTANCE,HWND);
void input_reset();
void input_update();
void input_config(int);
void input_shutdown();
