#pragma once
// Núcleo de DSP independente do JUCE (apenas biblioteca padrão).
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace es3
{
constexpr double kPi = 3.14159265358979323846;

// Ordem = índice dos parâmetros (não reordenar: presets/automação dependem disso).
enum class Wave { Sine = 0, Triangle, Saw, Square, Noise, Digital, SyncSquare, SyncSaw, Ring };

class Oscillator
{
public:
    void   reset (double p = 0.0) { phase = p; wrapped = false; }
    void   setPhase (double p)    { phase = p; }
    double getPhase() const       { return phase; }
    void   setNoiseColor (float c) { color = std::clamp (c, 0.f, 1.f); }
    bool   didWrap() const        { return wrapped; }
    double wrapFraction() const   { return wrapFrac; }   // fração do último passo decorrida desde o ciclo

    // Gera a amostra na fase atual (sem avançar).
    // blepScale: 1 = PolyBLEP completo (sem aliasing), 0 = forma ingênua (aliasing máximo).
    // pw: 0..1 (0 = 50%, 1 = pulso mais estreito). phaseMod: modulação de fase em ciclos (só senoide).
    float shape (Wave w, double dt, float pw, float blepScale, double phaseMod = 0.0)
    {
        const double t = phase;
        double out = 0.0;
        switch (w)
        {
            case Wave::Sine:     out = std::sin (2.0 * kPi * (t + phaseMod)); break;
            case Wave::Triangle: out = 4.0 * std::fabs (t - 0.5) - 1.0; break;
            case Wave::Saw:
            case Wave::SyncSaw:  out = 2.0 * t - 1.0 - blepScale * polyBlep (t, dt); break;
            case Wave::Square:
            case Wave::SyncSquare:
            case Wave::Ring:
            {
                const double d = 0.5 - 0.48 * std::clamp ((double) pw, 0.0, 1.0);
                double t2 = t + 1.0 - d; if (t2 >= 1.0) t2 -= 1.0;
                double raw = (t < d ? 1.0 : -1.0) + blepScale * (polyBlep (t, dt) - polyBlep (t2, dt));
                const double dc = 2.0 * d - 1.0;
                out = (raw - dc) / (1.0 + std::fabs (dc));
                break;
            }
            case Wave::Noise:    out = nextNoise(); break;
            case Wave::Digital:  // PLACEHOLDER (as 100 Digiwaves originais não estão disponíveis)
                out = (std::sin (2.0 * kPi * t) + 0.5 * std::sin (4.0 * kPi * t) + 0.33 * std::sin (6.0 * kPi * t)) / 1.5;
                break;
        }
        return (float) out;
    }

    void advance (double dt)
    {
        phase += dt; wrapped = false; wrapFrac = 0.0;
        if (phase >= 1.0) { phase -= 1.0; wrapped = true; wrapFrac = dt > 0.0 ? phase / dt : 0.0; }
    }

    // Conveniência (usada nos testes)
    float process (Wave w, double freq, double sampleRate, float pw = 0.f, float blepScale = 1.f)
    {
        const double dt = clampDt (freq / sampleRate);
        const float o = shape (w, dt, pw, blepScale);
        advance (dt);
        return o;
    }

    static double clampDt (double dt) { return dt < 0.0 ? 0.0 : (dt > 0.49 ? 0.49 : dt); }

    static double polyBlep (double t, double dt)
    {
        if (dt <= 0.0) return 0.0;
        if (t < dt)       { t /= dt;               return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt;    return t * t + t + t + 1.0; }
        return 0.0;
    }

private:
    double uniform01()
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return ((double) (rng >> 8) + 1.0) / 16777217.0;   // (0,1]
    }
    // Ruído branco GAUSSIANO (a gravação de referência tem curtose 3.01; uniforme daria 1.8). RMS ~0.3.
    double nextNoise()
    {
        const double white = kNoiseSigma * std::sqrt (-2.0 * std::log (uniform01())) * std::cos (2.0 * kPi * uniform01());
        if (color <= 0.f) return std::clamp (white, -1.0, 1.0);
        const double a = 1.0 - 0.97 * color;                   // passa-baixa de 1 polo (PROVISÓRIO)
        lp += a * (white - lp);
        const double g = 1.0 / std::sqrt (a / (2.0 - a));      // mantém o RMS parecido com o ruído branco
        return std::clamp (lp * g, -1.0, 1.0);
    }
    static constexpr double kNoiseSigma = 0.3;
    double phase = 0.0, wrapFrac = 0.0, lp = 0.0;
    bool wrapped = false;
    float color = 0.f;
    uint32_t rng = 0x12345678u;
};
}
