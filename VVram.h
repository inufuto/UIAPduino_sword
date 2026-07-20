#pragma once
#include "cate.h"

constexpr byte VVramWidth = 24;
constexpr byte VVramHeight = 16;
constexpr byte StageTop = 1;

extern byte VVram[];

inline ptr<byte> VPut(ptr<byte> pVVram, byte c) {
    *pVVram++ = c;
    return pVVram;
}

extern ptr<byte> VVramPtr(byte x, byte y);
extern ptr<byte> VPut2C(ptr<byte> pVVram, byte c);
extern ptr<byte> VErase2(ptr<byte> pVVram);
