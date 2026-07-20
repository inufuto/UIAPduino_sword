#pragma once

#include "cate.h"

constexpr auto CharWidth = 4;
constexpr auto VramStep = CharWidth;
constexpr auto VramRowSize = 0x100;
constexpr word Vram = 0x0000;

extern void ClearScreen();
extern void InvalidateVramBackup();
extern word PrintC(word vram, byte c);
extern word Put2C(word vram, byte c);
extern void VVramToVram();
extern void DrawAll();