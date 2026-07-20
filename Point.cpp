#include "Point.h"
#include "Sprite.h"
#include "Chars.h"
#include "Main.h"

constexpr byte MaxTime = 6;

static const word Values[] = { 10, 20, 40, 80 };

Point Points[PointSpriteCount];
byte PointRate;

void InitPoints()
{
    byte sprite = Sprite_Point;
    for (auto pPoint = Points; pPoint < Points + PointSpriteCount; ++pPoint) {
        pPoint->sprite = sprite;
        pPoint->time = 0;
        HideSprite(sprite);
        ++sprite;
    }
}


void StartPoint(byte x, byte y)
{
    AddScore(Values[PointRate]);
    for (auto pPoint = Points; pPoint < Points + PointSpriteCount; ++pPoint) {
        if (pPoint->time == 0) {
            pPoint->time = MaxTime << CoordShift;
            pPoint->x = x;
            pPoint->y = y;
            ShowSprite(pPoint, Pattern_Point + PointRate);
            ++PointRate;
            break;
        }
    }
}


void UpdatePoints()
{
    for (auto pPoint = Points; pPoint < Points + PointSpriteCount; ++pPoint) {
        if (pPoint->time != 0) {
            if (--pPoint->time == 0) {
                HideSprite(pPoint->sprite);
            }
        }
    }
}
