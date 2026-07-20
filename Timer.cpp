#include <ch32fun.h>
#include "cate.h"
#include "Timer.h"
#include "Sound.h"

static volatile byte TimerCount;
static constexpr uint32_t kTimerHz = 60;
static constexpr uint32_t kSysTickStep = FUNCONF_SYSTEM_CORE_CLOCK / kTimerHz;

extern "C" void SysTick_Handler(void) __attribute__((interrupt));
extern "C" void SysTick_Handler(void)
{
    // Schedule next tick to keep period stable even if ISR latency varies.
    SysTick->CMP += kSysTickStep;
    SysTick->SR = 0;
    ++TimerCount;
    SoundHandler();
}


void InitTimer()
{
    TimerCount = 0;
    SysTick->CTLR = 0;
    SysTick->CMP = kSysTickStep - 1;
    SysTick->CNT = 0;
    SysTick->SR = 0;
    SysTick->CTLR = SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE | SYSTICK_CTLR_STCLK;
    NVIC_EnableIRQ(SysTick_IRQn);
}


void WaitTimer(uint8_t t)
{
    while (TimerCount < t) {
        __asm__ volatile("nop");
    }
    TimerCount = 0;
}