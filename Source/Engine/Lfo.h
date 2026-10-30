#pragma once
#include <cmath>
#include <cstdint>
#include "Oscillator.h"

namespace es3
{
enum class LfoWave { Sine = 0, Triangle, SawUp, SawDown, Square, SampleHold, Smooth };

// LFO (saída -1..+1). Ainda sem destino: o Router chega na Parte 5. LFO 1 será polifônico (um por voz),
// LFO 2 monofônico, conforme a documentação. Faixas e fade são PROVISÓRIOS.
class Lfo
{
public:
    void setSampleRate (double sr) { sampleRate = sr; }
    void setRate (double hz)       { rate = hz < 0.0 ? 0.0 : hz; }
    void setWave (LfoWave w)       { wave = w; }
    void setFadeSeconds (double s) { fade = s; }
    void retrigger (double startPhase = 0.0) { phase = startPhase; fadeGain = fade > 0.0 ? 0.0 : 1.0; newRandom(); }

    float next()
    {
        double out = 0.0;
        switch (wave)
        {
            case LfoWave::Sine:       out = std::sin (2.0 * kPi * phase); break;
            case LfoWave::Triangle:   out = 1.0 - 4.0 * std::fabs (phase - 0.5); break;
            case LfoWave::SawUp:      out = 2.0 * phase - 1.0; break;
            case LfoWave::SawDown:    out = 1.0 - 2.0 * phase; break;
            case LfoWave::Square:     out = phase < 0.5 ? 1.0 : -1.0; break;
            case LfoWave::SampleHold: out = heldValue; break;
            case LfoWave::Smooth:     { const double t = phase; out = prevValue + (heldValue - prevValue) * (t * t * (3.0 - 2.0 * t)); break; }
        }
        phase += rate / sampleRate;
        if (phase >= 1.0) { phase -= 1.0; newRandom(); }
        if (fadeGain < 1.0) { fadeGain += fade > 0.0 ? 1.0 / (fade * sampleRate) : 1.0; if (fadeGain > 1.0) fadeGain = 1.0; }
        return (float) (out * fadeGain);
    }

private:
    void newRandom()
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        prevValue = heldValue; heldValue = (double) (int32_t) rng / 2147483648.0;
    }
    double sampleRate = 44100.0, rate = 1.0, fade = 0.0, phase = 0.0, fadeGain = 1.0, heldValue = 0.0, prevValue = 0.0;
    LfoWave wave = LfoWave::Sine; uint32_t rng = 0xC0FFEE11u;
};
}
