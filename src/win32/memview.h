#ifndef __MEMVIEW_H__
#define __MEMVIEW_H__

#include "tlcs900h.h"
#include "memory.h"
#include "resource.h"
#include <Windows.h>

#ifdef NEOGPC_DEBUGGER

extern HWND g_memViewHwnd;
extern BOOL g_memViewActive;

void openMemoryEditor();
void DisplayMemoryEditor();

#endif

#endif // __MEMVIEW_H__