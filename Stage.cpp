#include "Stage.h"
#include "Stages.h"
#include "Main.h"
#include "VVram.h"
#include "Chars.h"
#include "Man.h"
#include "Monster.h"
#include "Ball.h"
#include "Status.h"
#include "Point.h"
#include "Math.h"
#include "OneUp.h"
#include "Vram.h"
#include "Chars.h"

constexpr byte MaxTimerValue = 50;
constexpr byte MinTimerValue = 8;

constptr<Stage> pStage;
byte StageMap[MapSize];
byte BoxCount;
byte TimerValue;

byte ToColumn(byte b)
{
    return b >> 4;
}

byte ToRow(byte b)
{
    return b & 0x0f;
}


void InitStage()
{
    RndIndex = 0;
    TimerValue = MaxTimerValue;
    auto p = Stages;
    byte count = CurrentStage;
    byte i = 0;
    while (count != 0) {
        ++p;
        ++i;
        if (i >= StageCount) {
            p = Stages;
            i = 0;
        }
        --TimerValue;
        if (TimerValue < MinTimerValue) {
            TimerValue = MinTimerValue;
        }
        --count;
    }
    pStage = p;

    ClearScreen();
    PrintStatus();
    {
        BoxCount = 0;
        auto pBytes = pStage->bytes;
        byte row = 0;
        while (row < RowCount) {
            byte column = 0;
            repeat (ColumnCount / ColumnsPerByte) {
                byte b = *pBytes; ++pBytes;
                repeat (ColumnsPerByte) {
                    byte cell = b & 0x03;
                    if (cell == Cell_Box) {
                        ++BoxCount;
                    }
                    SetCell(column, row, cell);
                    b >>= 2;
                    ++column;
                }
            }
            ++row;
        }
    }
    InitMan();
    InitMonsters();
    InitBalls();
    InitPoints();
    InitOneUp();
}


static ptr<byte> MapPtr(byte column, byte row)
{
    return StageMap + (row * ColumnCount + column) / ColumnsPerByte;
}

byte GetCell(byte column, byte row)
{
    byte b = *MapPtr(column, row);
    byte shift = (column & 3) * 2;
    return (b >> shift) & 0x03;
}

void SetCell(byte column, byte row, byte cell)
{
    auto p = MapPtr(column, row);
    byte shift = (column & 3) * 2;
    byte mask = ~(0x03 << shift);
    *p = (*p & mask) | (cell << shift);
}

void DrawBackGround()
{
    auto pVVram = VVram;
    repeat (VVramWidth) {
        pVVram = VPut(pVVram, Char_Fence);
    }
    auto pMap = StageMap;
    repeat (RowCount) {
        repeat (ColumnCount / ColumnsPerByte) {
            byte b = *pMap; ++pMap;
            repeat (ColumnsPerByte) {
                byte cell = b & 0x03;
                if (cell == Cell_Space) {
                    pVVram = VErase2(pVVram);
                }
                else {
                    pVVram = VPut2C(pVVram, Char_Wall + ((cell - 1) << 2));
                }
                b >>= 2;
            }
        }
        pVVram += VVramWidth * RowHeight - ColumnCount * ColumnWidth;
    }
    repeat (VVramWidth) {
        pVVram = VPut(pVVram, Char_Fence + 1);
    }
}