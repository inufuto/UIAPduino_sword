#include <cstring>
#include "cate.h"
#include "Vram.h"
#include "Oled.h"
#include "VVram.h"
#include "Stage.h"
#include "Sprite.h"

extern const byte AsciiPattern[];
extern const byte CharPattern[];

constexpr auto CharPatternSize = 2;

static byte Backup[VVramWidth * VVramHeight];

void InvalidateVramBackup()
{
    memset(Backup, 0xff, VVramWidth * VVramHeight);
}

static bool Locate(word vram)
{
    byte page = vram >> 8;
    byte column = vram;
    return 
        SendOledCmd(OledCmd::Page + page) &&
        SendOledCmd(OledCmd::ColumnLow + (column & 0x0F)) &&
        SendOledCmd(OledCmd::ColumnHigh + (column >> 4));
}


void ClearScreen()
{
    for (byte page = 0; page < 8; ++page) {
        Locate(page << 8);
        for (byte col = 0; col < 128; ++col) {
            SendOledData(0x00);
        }
    }
    memset(VVram, 0x00, VVramWidth * VVramHeight);
    memset(Backup, 0x00, VVramWidth * VVramHeight);
}

word PrintC(word vram, byte c)
{
    static const char AsciiTable[] = " 0123456789>ACEFGIMNORSTUV";
    auto pPattern = AsciiPattern;
    for (auto i = 0; AsciiTable[i] != '\0'; ++i) {
        if (AsciiTable[i] == c) {
            goto found;
        }
        pPattern += CharWidth;
    }
    pPattern = AsciiPattern; // not found, use space
found:
    Locate(vram);
    SendOledData(pPattern, CharWidth);
    return vram + VramStep;
}


static bool SendUL(byte upper, byte lower)
{
    return
        SendOledData((upper & 0x0f) | (lower << 4)) &&
        SendOledData((upper >> 4) | (lower & 0xf0));
}

word Put2C(word vram, byte c)
{
    auto pUpperPattern = CharPattern + (c * CharPatternSize);
    auto pLowerPattern = pUpperPattern + CharPatternSize * 2;
    Locate(vram);
    for (auto i = 0; i < CharPatternSize * 2; ++i) {
        auto upperByte = *pUpperPattern++;
        auto lowerByte = *pLowerPattern++;
        SendUL(upperByte, lowerByte);
    }
    return vram + VramStep * 2;
}

void VVramToVram()
{
    const auto* pVVram = VVram;
    auto pBackup = Backup;
    auto vram = Vram;
    for (auto y = 0; y < VVramHeight / 2; ++y) {
        bool skipped = true;
        for (auto x = 0; x < VVramWidth; ++x) {
            auto upper = pVVram[0];
            auto lower = pVVram[VVramWidth];
            if (upper != pBackup[0] || lower != pBackup[VVramWidth]) {
                if (skipped) {
                    skipped = false;
                    if (!Locate(vram)) goto onError;
                }
                auto pUpperPattern = CharPattern + (upper * CharPatternSize);
                auto pLowerPattern = CharPattern + (lower * CharPatternSize);
                for (auto i = 0; i < CharPatternSize; ++i) {
                    auto upperByte = *pUpperPattern++;
                    auto lowerByte = *pLowerPattern++;
                    if (!SendUL(upperByte, lowerByte)) goto onError;
                }
                pBackup[0] = upper;
                pBackup[VVramWidth] = lower;
            }
            else {
                onError:
                skipped = true;
            }
            vram += VramStep;
            ++pVVram;
            ++pBackup;
        }
        pVVram += VVramWidth;
        pBackup += VVramWidth;
        vram += VramRowSize - (VramStep * VVramWidth);
    }
}


void DrawAll()
{
    DrawBackGround();
    DrawSprites();
    VVramToVram();
}
