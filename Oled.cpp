#include <stdint.h>
#include "ch32fun.h"
#include "Oled.h"

#define SSD1306_ADDR 0x3C
static constexpr uint32_t kI2CTimeout = 200000;
static constexpr uint8_t kI2CRetryCount = 2;
static constexpr uint8_t kRecoverResetThreshold = 4;
static constexpr uint8_t kRecoverCooldownFrames = 8;
static constexpr uint8_t kI2CSdaPin = PC1;
static constexpr uint8_t kI2CSclPin = PC2;

static bool WaitStar1(uint16_t flag)
{
    for (uint32_t i = 0; i < kI2CTimeout; ++i) {
        if (I2C1->STAR1 & flag) {
            return true;
        }
    }
    return false;
}

static bool WaitStar2(uint16_t flag)
{
    for (uint32_t i = 0; i < kI2CTimeout; ++i) {
        if (I2C1->STAR2 & flag) {
            return true;
        }
    }
    return false;
}

static void DelayRecoverCycles(uint32_t cycles)
{
    for (volatile uint32_t i = 0; i < cycles; ++i) {
        __asm__ volatile("nop");
    }
}

static void ConfigureI2CPinsAlternate()
{
    GPIOC->CFGLR &= ~(0xFF << 4);
    GPIOC->CFGLR |= (0xDF << 4);
}

static void RecoverI2CBusByClock()
{
    funPinMode(kI2CSdaPin, GPIO_CFGLR_OUT_10Mhz_OD);
    funPinMode(kI2CSclPin, GPIO_CFGLR_OUT_10Mhz_OD);

    funDigitalWrite(kI2CSdaPin, FUN_HIGH);
    funDigitalWrite(kI2CSclPin, FUN_HIGH);
    DelayRecoverCycles(2000);

    for (uint8_t i = 0; i < 16 && funDigitalRead(kI2CSdaPin) == 0; ++i) {
        funDigitalWrite(kI2CSclPin, FUN_LOW);
        DelayRecoverCycles(2000);
        funDigitalWrite(kI2CSclPin, FUN_HIGH);
        DelayRecoverCycles(2000);
    }

    // Generate a STOP condition to release stuck slaves.
    funDigitalWrite(kI2CSdaPin, FUN_LOW);
    DelayRecoverCycles(2000);
    funDigitalWrite(kI2CSclPin, FUN_HIGH);
    DelayRecoverCycles(2000);
    funDigitalWrite(kI2CSdaPin, FUN_HIGH);
    DelayRecoverCycles(2000);
}

static void RecoverI2C()
{
    // Abort transfer, release bus by SCL clocking, then reset/re-enable I2C.
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

    I2C1->CTLR1 |= I2C_CTLR1_STOP;
    DelayRecoverCycles(10000);
    I2C1->CTLR1 &= ~I2C_CTLR1_PE;
    DelayRecoverCycles(10000);

    RecoverI2CBusByClock();
    ConfigureI2CPinsAlternate();

    RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
    DelayRecoverCycles(2000);
    RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1;

    I2C1->CTLR2 = 48;
    I2C1->CKCFGR = 40;
    I2C1->CTLR1 |= I2C_CTLR1_PE;
    DelayRecoverCycles(10000);
}

static bool StartI2C()
{
    I2C1->CTLR1 |= I2C_CTLR1_START;
    return WaitStar1(I2C_STAR1_SB);
}

static void StopI2C()
{
    I2C1->CTLR1 |= I2C_CTLR1_STOP;
}

static bool WriteI2CByte(uint8_t byte)
{
    I2C1->DATAR = byte;
    return WaitStar1(I2C_STAR1_TXE);
}

static bool PreSend()
{
    for (uint8_t retry = 0; retry < kI2CRetryCount; ++retry) {
        if (StartI2C() && WriteI2CByte(SSD1306_ADDR << 1) && WaitStar2(I2C_STAR2_MSL)) {
            return true;
        }
        StopI2C();
        RecoverI2C();
    }
    return false;
}

bool SendOledCmd(uint8_t cmd)
{
    if (!PreSend()) {
        return false;
    }
    if (!WriteI2CByte(0x00) || !WriteI2CByte(cmd)) {
        StopI2C();
        RecoverI2C();
        return false;
    }
    StopI2C();
    return true;
}

bool SendOledCmd(const uint8_t* cmds, int length)
{
    if (!PreSend()) {
        return false;
    }
    if (!WriteI2CByte(0x00)) {
        StopI2C();
        RecoverI2C();
        return false;
    }
    for (int i = 0; i < length; i++) {
        if (!WriteI2CByte(cmds[i])) {
            StopI2C();
            RecoverI2C();
            return false;
        }
    }
    StopI2C();
    return true;
}

bool SendOledData(uint8_t data)
{
    if (!PreSend()) {
        return false;
    }
    if (!WriteI2CByte(0x40) || !WriteI2CByte(data)) {
        StopI2C();
        RecoverI2C();
        return false;
    }
    StopI2C();
    return true;
}

bool SendOledData(const uint8_t* data, int length)
{
    if (!PreSend()) {
        return false;
    }
    if (!WriteI2CByte(0x40)) {
        StopI2C();
        RecoverI2C();
        return false;
    }
    for (int i = 0; i < length; i++) {
        if (!WriteI2CByte(data[i])) {
            StopI2C();
            RecoverI2C();
            return false;
        }
    }
    StopI2C();
    return true;
}

void InitOled()
{
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

    ConfigureI2CPinsAlternate();

    I2C1->CTLR2 = 48;
    I2C1->CKCFGR = 40;
    I2C1->CTLR1 |= I2C_CTLR1_PE;

    static const uint8_t cmds[] = {
        OledCmd::DisplayOff, // Display OFF
        OledCmd::AddressingMode, 0x02, // Set Memory Addressing Mode to Page Addressing Mode
        OledCmd::ChargePump, 0x14, // Enable charge pump
        OledCmd::RightToLeft, // Set segment re-map
        OledCmd::BottomToTop, // Set COM scan direction
        OledCmd::DisplayOn // Display ON
    };
    SendOledCmd(cmds, sizeof(cmds));
}
