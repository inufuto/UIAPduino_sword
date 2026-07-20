#include "ch32fun.h"
#include "Main.h"
#include "Status.h"
#include "ScanKeys.h"
#include "Stage.h"
#include "Sound.h"
#include "Man.h"
#include "Monster.h"
#include "Ball.h"
#include "Point.h"
#include "Vram.h"
#include "Oled.h"
#include "Timer.h"

extern void WaitTimer(byte t);

word Score;
word HiScore;
byte RemainCount;
byte CurrentStage;
static byte Clock;

void Main() {
    HiScore = 0;
    Score = 0;
    CurrentStage = 0;
    RemainCount = 3;

    title:
    auto selection = Title();
    // play:
    Score = 0;
    if (selection == 0) {
        CurrentStage = 0;
    }
    RemainCount = 3;
    stage:
    InitStage();
    DrawAll();
    Clock = 0;
    static sbyte monsterNum = 0;
    Sound_Start();
    StartBGM();
    do {
        if ((Clock & 3) == 0) {
            MoveMan();
            if (monsterNum >= 0) {
                MoveMonsters();
                monsterNum -= 7;
            }
            monsterNum += 4;
            MoveBalls();

            UpdatePoints();

            DrawAll();
            WaitTimer(8 / CoordRate);
        }
        if ((Clock & 15) == 0) {
            WaitTimer(8 / CoordRate);
        }
        if (RemainCount == 0) {
            ShowMan();
            DrawAll();
            StopBGM();
            WaitTimer(30);
            PrintGameOver();
            Sound_GameOver();
            goto title;
        }
        ++Clock;
    } while (BoxCount != 0);
    StopBGM();
    ShowMan();
    DrawAll();
    WaitTimer(30);
    Sound_Clear();
    ++CurrentStage;
    goto stage;
}


void AddScore(word pts) 
{
    Score += pts;
    if (Score > HiScore) {
        HiScore = Score;
    }
    PrintScore();
}


int main()
{
    SystemInit();
    InitOled();
    InitKeys();
    InitSound();
    InitTimer();
    Main();
}

void _deb(){}