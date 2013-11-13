#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include "tlcs900h.h"
#include "winsound.h"
#include "resource.h"
#include <Windows.h>

#ifdef NEOGPC_DEBUGGER

extern HWND g_tlcs900hDebugHwnd;
extern BOOL g_tlcs900hActive;
extern BOOL g_tlcs900hUpdateDebug;
extern HWND g_Z80DebugHwnd;
extern BOOL g_Z80Active;
extern BOOL g_Z80UpdateDebug;

void UpdateRegs(HWND hwndDlg);
void Disassemble(HWND hwndDlg, unsigned int pcAddr, unsigned int startAddr);
INT_PTR CALLBACK TLCS900hProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void openTlcs900hDebugger();
void openZ80Debugger();

#endif

#endif // __DEBUGGER_H__