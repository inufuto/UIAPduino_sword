#include <stdint.h>
#include "ch32fun.h"
#include "Sound.h"
#include "Uncopyable.h"

constexpr auto Tempo = 180;

constexpr auto ToneChannelCount = 3;
constexpr auto MaxVolume = 63;

constexpr auto EndPlaying = 0x00;
constexpr auto Repeat = 0xff;

enum NoteLength
{
    N8 = 6,
    N8L = 8,
    N8R = 4,
    N8P = N8 * 3 / 2,
    N4 = N8 * 2,
    N4P = N4 * 3 / 2,
    N2 = N4 * 2,
    N2P = N2 * 3 / 2,
    N1 = N2 * 2,
    N16 = N8 / 2,
};

enum Scale
{
    E2 = 1,
    F2 = 2,
    F2S = 3,
    G2 = 4,
    G2S = 5,
    A2 = 6,
    A2S = 7,
    B2 = 8,
    C3 = 9,
    C3S = 10,
    D3 = 11,
    D3S = 12,
    E3 = 13,
    F3 = 14,
    F3S = 15,
    G3 = 16,
    G3S = 17,
    A3 = 18,
    A3S = 19,
    B3 = 20,
    C4 = 21,
    C4S = 22,
    D4 = 23,
    D4S = 24,
    E4 = 25,
    F4 = 26,
    F4S = 27,
    G4 = 28,
    G4S = 29,
    A4 = 30,
    A4S = 31,
    B4 = 32,
    C5 = 33,
    C5S = 34,
    D5 = 35,
    D5S = 36,
    E5 = 37,
    F5 = 38,
    F5S = 39,
    G5 = 40,
};

static uint16_t Frequencies[] = {
    82, // E2
    87, // F2
    92, // F#2
    98, // G2
    104, // G#2
    110, // A2
    117, // A#2
    123, // B2
    131, // C3
    139, // C#3
    147, // D3
    156, // D#3
    165, // E3
    175, // F3
    185, // F#3
    196, // G3
    208, // G#3
    220, // A3
    233, // A#3
    247, // B3
    262, // C4
    277, // C#4
    294, // D4
    311, // D#4
    330, // E4
    349, // F4
    370, // F#4
    392, // G4
    415, // G#4
    440, // A4
    466, // A#4
    494, // B4
    523, // C5
    554, // C#5
    587, // D5
    622, // D#5
    659, // E5
    698, // F5
    740, // F#5
    784, // G5
};

class ToneChannel : public Uncopyable
{
private:
    uint32_t phase;
    uint8_t volume;
    uint32_t phaseDelta;

    const uint8_t* volatile pMelody;
    volatile uint16_t melodyOffset;
    volatile uint8_t noteLength;
public:
    void Reset();
    uint8_t Sample();
    void Next();
    void StartMelody(const uint8_t* pMelody);
    bool IsPlaying() const { return pMelody != nullptr; }
};

class EffectChannel : public Uncopyable
{
private:
    uint32_t noiseState;
    uint8_t volume;
    uint32_t phase;
    uint32_t phaseDelta;
public:
    void Reset();
    uint8_t Sample();
    void Next();
    void Start(uint16_t frequency);
};

static ToneChannel ToneChannels[ToneChannelCount];
EffectChannel Effect;
static volatile bool Enabled;

namespace {
    constexpr uint32_t PwmCarrierHz = 187500;
    constexpr uint32_t MixSampleRateHz = 11025;
    constexpr uint16_t DefaultEffectFrequencyHz = 4000;
    constexpr uint16_t PwmPeriod = 256;
    constexpr uint16_t MixPeriod = static_cast<uint16_t>(FUNCONF_SYSTEM_CORE_CLOCK / MixSampleRateHz);
    constexpr uint16_t MixDenominator = MaxVolume * (ToneChannelCount + 1);

    uint16_t MixToDuty()
    {
        uint16_t sum = 0;
        for (auto& channel : ToneChannels) {
            sum += channel.Sample();
        }
        sum += Effect.Sample();

        return static_cast<uint16_t>((static_cast<uint32_t>(sum) * (PwmPeriod - 1)) / MixDenominator);
    }
}

void ToneChannel::Reset()
{
    volume = 0;
    pMelody = nullptr;
}

uint8_t ToneChannel::Sample()
{
    if (volume == 0 || phaseDelta == 0) {
        return 0;
    }

    phase += phaseDelta;
    return (phase & 0x80000000u) != 0 ? volume : 0;
}

void ToneChannel::Next()
{
    if (pMelody == nullptr) return;
    if (--noteLength == 0) {
        keep:
        auto length = pMelody[melodyOffset++];
        if (length == EndPlaying) {
            pMelody = nullptr;
            volume = 0;
            return;
        }
        if (length == Repeat) {
            melodyOffset = 0;
            goto keep;
        }
        noteLength = length;
        auto note = pMelody[melodyOffset++];
        if (note != 0) {
            phaseDelta = Frequencies[note - 1] == 0 ? 0 : static_cast<uint32_t>((static_cast<uint64_t>(Frequencies[note - 1]) << 32) / MixSampleRateHz);
            volume = MaxVolume;
        }
        else {
            volume = 0;
        }
    }
    else {
        if (volume > 0) {
            --volume;
        }
    }
}

void ToneChannel::StartMelody(const uint8_t *pMelody)
{
    this->pMelody = pMelody;
    melodyOffset = 0;
    noteLength = 1;
}

void EffectChannel::Reset()
{
    noiseState = 1;
    phase = 0;
    phaseDelta = 0;
    volume = 0;
}

uint8_t EffectChannel::Sample()
{
    if (volume == 0 || phaseDelta == 0) {
        return 0;
    }

    const uint32_t previousPhase = phase;
    phase += phaseDelta;

    if (phase < previousPhase) {
        const uint32_t lsb = noiseState & 1;
        noiseState >>= 1;
        if (lsb != 0) {
            noiseState ^= 0xB800;
        }
    }

    return (noiseState & 1) != 0 ? volume : 0;
}

void EffectChannel::Next()
{
    if (volume > 0) {
        --volume;
    }
}

void EffectChannel::Start(uint16_t frequency)
{
    phaseDelta = frequency == 0 ? 0 : static_cast<uint32_t>((static_cast<uint64_t>(frequency) << 32) / MixSampleRateHz);
    volume = MaxVolume;
}

extern "C" void TIM2_IRQHandler(void) __attribute__((interrupt));
extern "C" void TIM2_IRQHandler(void)
{
    if ((TIM2->INTFR & TIM_FLAG_Update) == 0) {
        return;
    }

    TIM2->INTFR = ~TIM_FLAG_Update;
    TIM1->CH2CVR = MixToDuty();
}

void InitSound()
{
    funGpioInitA();
    RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
    RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

    funPinMode(PA1, GPIO_CFGLR_OUT_50Mhz_AF_PP);

    RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

    TIM1->PSC = 0;
    TIM1->ATRLR = PwmPeriod - 1;
    TIM1->RPTCR = 0;
    TIM1->CHCTLR1 = TIM_OC2M_2 | TIM_OC2M_1 | TIM_OC2PE;
    TIM1->CCER = TIM_CC2E;
    TIM1->CH2CVR = 0;
    TIM1->BDTR = TIM_MOE;
    TIM1->SWEVGR = TIM_UG;
    TIM1->CTLR1 = TIM_ARPE | TIM_CEN;

    RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
    RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;
    TIM2->PSC = 0;
    TIM2->ATRLR = MixPeriod - 1;
    TIM2->INTFR = ~TIM_FLAG_Update;
    TIM2->DMAINTENR = TIM_IT_Update;
    TIM2->SWEVGR = TIM_UG;
    TIM2->CTLR1 = TIM_ARPE | TIM_CEN;
    NVIC_EnableIRQ(TIM2_IRQn);

    for (auto& channel : ToneChannels) {
        channel.Reset();
    }
    Effect.Reset();
    Enabled = true;
}

void SoundHandler()
{
    static int16_t time = 0;
    if (!Enabled) return;
    time -= Tempo;
    if (time <= 0) {
        time += 600 / 2;
        for (auto& channel : ToneChannels) {
            channel.Next();
        }
        Effect.Next();
    }
}

static void StartMelody(int index, const uint8_t *pMelody)
{
    auto e = Enabled;
    Enabled = false;
    ToneChannels[index].StartMelody(pMelody);
    Enabled = e;
}

static void WaitMelody(int index, const uint8_t *pMelody)
{
    Enabled = false;
    StartMelody(index, pMelody);
    Enabled = true;
    while (ToneChannels[index].IsPlaying()) {
        __asm__ volatile("nop");
    }
}


void Sound_Loose()
{
    static const uint8_t notes[] = {
        1,F5, 1,E5, 1,D5, 1,C5, 1,B4, 1,A4, 1,G4, 1,F4, 0
    };
    StartMelody(0, notes);
}

void Sound_Hit()
{
    static const uint8_t notes[] = {
        1,F4, 1,G4, 1,A4, 1,B4, 1,C5, 1,D5, 1,E5, 1,F5,
        0
    };
    StartMelody(0, notes);
}

void Sound_Up()
{
    static const uint8_t notes[] = {
        1,C4, 1, C4S, 1,D4, 1,F4, 1,A4, 1,C5,
        0
    };
    StartMelody(0, notes);
}

void Sound_Attack()
{
    static const uint8_t notes[] = {
        1,A4, 0
    };
    StartMelody(0, notes);
}

void Sound_Start()
{
    static const uint8_t notes[] = {
        N4,A4, N4,A4, N8,A4, N4,C5, N8,D5,
        N2,E5, N4,0, N4,0,
        0
    };
    WaitMelody(1, notes);
}

void Sound_Clear() 
{
    static const uint8_t notes[] = {
        N8,A4, N8,0, N8,A4, N8,G4, N8,A4, N4,C5, N8,D5, N8,0, N8,C5, N8,0, N4P,A4, N2,0,
        0
    };
    WaitMelody(1, notes);
}

void Sound_GameOver()
{
    static const uint8_t notes[] = {
        N8,A4, N8,E5, N8,D5, N8,C5, N8,D5, N8,C5, N8,B4, N4P,A4,
        N8,0, N4,G4, N8,G4, N4,A4,
        0
    };
    WaitMelody(1, notes);
}

void StartBGM()
{
    static const uint8_t notes1[] = {
        N4,A4, N4,A4, N8,A4, N4,C5, N8,D5,
        N2,E5, N4,0, N8,E5, N8,F5,
        N8P,E5, N8P,D5, N8,C5, N8P,D5, N8P,C5, N8,B4,
        N8P,C5, N8P,B4, N8,A4, N8P,B4, N8P,A4, N8,G4,

        N4,A4, N4,A4, N8,A4, N4,C5, N8,D5,
        N2,E5, N4,0, N8,E5, N8,F5,
        N8P,E5, N8P,D5, N8,C5, N8P,D5, N8P,C5, N8,B4,
        N8P,C5, N8P,B4, N8,A4, N8P,B4, N8P,A4, N8,G4,

        N8,A4, N8,A4, N4,A4, N8,0, N8,G4, N8,0, N8,A4,
        N8,C5, N8,C5, N4,C5, N8,0, N8,B4, N8,0, N8,C5,
        N8,D5, N8,D5, N4,D5, N8,0, N8,C5, N8,0, N8,D5,
        N8P,E5, N8P,D5, N8,C5, N8P,D5, N8P,C5, N8,B4,

        N8,A4, N8,A4, N4,A4, N8,0, N8,G4, N8,0, N8,A4,
        N8,C5, N8,C5, N4,C5, N8,0, N8,B4, N8,0, N8,C5,
        N8,D5, N8,D5, N4,D5, N8,0, N8,C5, N8,0, N8,D5,
        N8P,E5, N8P,D5, N8,C5, N8P,D5, N8P,C5, N8,B4,
        0xff
    };
    static const uint8_t notes2[] = {
        N4,F2, N8,0, N8,F2, N8,0, N8,F2, N8,0, N8,F2,
        N4,C3, N8,0, N8,C3, N8,0, N8,C3, N8, 0, N8,C3,
        N4,D3, N8,0, N8,D3, N8,0, N8,D3, N8,0, N8,D3,
        N8,F2, N8,0, N8,F2, N8,0, N8,G2, N8,0, N8,G2, N8,0,

        N4,F2, N8,0, N8,F2, N8,0, N8,F2, N8,0, N8,F2,
        N4,C3, N8,0, N8,C3, N8,0, N8,C3, N8, 0, N8,C3,
        N4,D3, N8,0, N8,D3, N8,0, N8,D3, N8,0, N8,D3,
        N8,F2, N8,0, N8,F2, N8,0, N8,G2, N8,0, N8,G2, N8,0,

        N4,A2, N8,0, N8,A2, N8,0, N8,A2, N8,0, N8,A2,
        N4,F2, N8,0, N8,F2, N8,0, N8,F2, N8,0, N8,F2,
        N4,D3, N8,0, N8,D3, N8,0, N8,D3, N8,0, N8,D3,
        N8,C3, N8,0, N8,C3, N8,0, N8,B2, N8,0, N8,B2, N8,0,

        N4,A2, N8,0, N8,A2, N8,0, N8,A2, N8,0, N8,A2,
        N4,F2, N8,0, N8,F2, N8,0, N8,F2, N8,0, N8,F2,
        N4,D3, N8,0, N8,D3, N8,0, N8,D3, N8,0, N8,D3,
        N8,C3, N8,0, N8,C3, N8,0, N8,B2, N8,0, N8,B2, N8,0,
        0xff
    };
    Enabled = false;
    StartMelody(1, notes1);
    StartMelody(2, notes2);
    Enabled = true;
}

void StopBGM()
{
    Enabled = false;
    for (auto& channel : ToneChannels) {
        channel.Reset();
    }
    Enabled = true;
}
