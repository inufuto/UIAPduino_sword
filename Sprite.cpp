#include "Sprite.h"
#include "Chars.h"
#include "VVram.h"

constexpr byte InvalidY = 0xf0;
struct Sprite {
    byte x, y;
    byte c;
};
Sprite Sprites[Sprite_End];

void HideAllSprites()
{
    for (auto pSprite = Sprites; pSprite < Sprites + Sprite_End; ++pSprite) {
        pSprite->y = InvalidY;
    }
}


void ShowSprite(ptr<Movable> pMovable, byte pattern)
{
    ptr<Sprite> pSprite = Sprites + pMovable->sprite;
    pSprite->x = pMovable->x;
    pSprite->y = pMovable->y + StageTop;
    pSprite->c = (pattern << 2) + Char_Sprite;
}


void HideSprite(byte index) 
{
    ptr<Sprite> pSprite = Sprites + index;
    pSprite->y = InvalidY;
}


void DrawSprites()
{
    ptr<Sprite> pSprite = Sprites + Sprite_End;
    repeat (Sprite_End) {
        --pSprite;
        byte x = pSprite->x;
        byte y = pSprite->y;
        if (y <= VVramHeight - 1 && x <= VVramWidth - 1) {
            ptr<byte> pVVram = VVramPtr(x, y);
            byte c = pSprite->c;
            repeat (2) {
                repeat (2) {
                    *pVVram = c;
                    ++pVVram;
                    ++c;
                }
                pVVram += VVramWidth - 2;
            }
        }
    }
}
