#pragma once

#include "tlcs900h.h"

#ifdef TLCS900H_DEBUGGER

#include <Windows.h>

// Debugger for TLCS900h
void tlcs_debug_execute(int cycles);
void tlcs900h_debug_decode_rom();

extern void setupTLCS900hDebugger(HWND);
extern void runTLCS900hDebugger();
extern void shutdownTLCS900hDebugger();
extern void setupZ80Debugger(HWND);
extern void runZ80Debugger();
extern void shutdownZ80Debugger();

#endif
