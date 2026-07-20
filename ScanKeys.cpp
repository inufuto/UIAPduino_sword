#include "ch32fun.h"
#include "ScanKeys.h"

void InitKeys()
{
    // Enable GPIO peripheral clocks before touching CFGLR/OUTDR.
    funGpioInitA();
    funGpioInitC();

    funAnalogInit();
    funPinMode(PC4, GPIO_CFGLR_IN_ANALOG);
    funPinMode(PA2, GPIO_CFGLR_IN_PUPD);
    funDigitalWrite(PA2, FUN_HIGH); // PUPD + HIGH selects pull-up.
}

byte ScanKeys()
{
    static const struct { 
        uint16_t value;
        byte key;
    } KeyMap[] = {
        { 47, 0x00 },
        { 147, Keys_Right },
        { 232, Keys_Up },
        { 305, Keys_Right | Keys_Up },
        { 368, Keys_Down },
        { 415, Keys_Right | Keys_Down },
        { 477, Keys_Up | Keys_Down },
        { 524, Keys_Left },
        { 552, Keys_Left | Keys_Right },
        { 592, Keys_Left | Keys_Up },
        { 1024, Keys_Left | Keys_Down },
    };

    byte keys = 0;
    auto analogValue = funAnalogRead(2);
    if (analogValue != 0) {
        for (const auto& entry : KeyMap) {
            if (analogValue < entry.value) {
                keys |= entry.key;
                break;
            }
        }
    }
    if (funDigitalRead(PA2) == 0) {
        keys |= Keys_Button0;
    }
    return keys;
}