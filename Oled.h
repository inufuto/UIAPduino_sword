#pragma once

enum OledCmd : uint8_t {
    SetContrast = 0x81,
    DisplayAllOnResume = 0xA4,
    DisplayAllOn = 0xA5,
    NormalDisplay = 0xA6,
    InvertDisplay = 0xA7,
    DisplayOff = 0xAE,
    DisplayOn = 0xAF,
    SetDisplayOffset = 0xD3,
    SetComPins = 0xDA,
    SetVcomDetect = 0xDB,
    SetDisplayClockDiv = 0xD5,
    SetPrecharge = 0xD9,
    SetMultiplex = 0xA8,
    SetLowColumn = 0x00,
    SetHighColumn = 0x10,
    SetStartLine = 0x40,
    AddressingMode = 0x20,
    ColumnAddr = 0x21,
    PageAddr = 0x22,
    ComScanInc = 0xC0,
    ComScanDec = 0xC8,
    SegRemap = 0xA1,
    ChargePump = 0x8D,

    Page = 0xB0,
    ColumnLow = 0x00,
    ColumnHigh = 0x10,

    LeftToRight = 0xA0,
    RightToLeft = 0xA1,
    TopToBottom = 0xC0,
    BottomToTop = 0xC8,
};

extern bool SendOledCmd(uint8_t cmd);
extern bool SendOledCmd(const uint8_t* cmds, int length);
extern bool SendOledData(uint8_t data);
extern bool SendOledData(const uint8_t* data, int length);
extern void InitOled();
