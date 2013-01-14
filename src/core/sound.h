#pragma once

#include "../cpu/z80.h"

// NeoGPC Sound

///
/// Neogeo pocket sound functions
///
void soundStep(int cycles);
void soundUpdate(void);
void ngpSoundStart();
void ngpSoundExecute();
void ngpSoundOff();
void ngpSoundInterrupt();