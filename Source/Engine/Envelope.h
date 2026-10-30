#pragma once
#include <algorithm>
#include <cmath>

namespace es3
{
// ADSR. Com Attack = 0 a subida leva ~50 µs e com Release = 0 a queda leva ~60 µs (2-3 amostras a 48 kHz),
// como observado nas gravações de referência (docs/relatorio_referencias_ES2.md). Esses pisos NÃO têm
// fade de proteção: o corte seco é intencional (característica do ES2). Curvas intermediárias: PROVISÓRIAS.
class Envelope
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };
    static constexpr double kMinAttackSec  = 0.00005;
    static constexpr double kMinReleaseSec = 0.00006;

    void setSampleRate (double sr) { sampleRate = sr; recompute(); }
    void setParams (float a, float d, float s, float r)
    { attack = a; decay = d; sustain = std::clamp (s, 0.f, 1.f); release = r; recompute(); }

    void noteOn()  { stage = Stage::Attack; }
    void noteOff()
    {
        if (stage == Stage::Idle) return;
        stage = Stage::Release;
        if (releaseLinear) linearStep = level / (float) releaseSamples;
    }
    void reset()   { stage = Stage::Idle; level = 0.f; }
    bool isActive() const { return stage != Stage::Idle; }
    Stage getStage() const { return stage; }

    float next()
    {
        switch (stage)
        {
            case Stage::Idle: return 0.f;
            case Stage::Attack:
                level += attackInc;
                if (level >= 1.f) { level = 1.f; stage = Stage::Decay; }
                break;
            case Stage::Decay:
                level = sustain + (level - sustain) * decayCoef;
                if (std::fabs (level - sustain) < 1.0e-4f) { level = sustain; stage = Stage::Sustain; }
                break;
            case Stage::Sustain: level = sustain; break;
            case Stage::Release:
                if (releaseLinear) level -= linearStep; else level *= releaseCoef;
                if (level < 1.0e-4f || (releaseLinear && level <= 0.f)) { level = 0.f; stage = Stage::Idle; }
                break;
        }
        return level;
    }

private:
    void recompute()
    {
        const double a = std::max ((double) attack, kMinAttackSec);
        attackInc = (float) (1.0 / std::max (1.0, a * sampleRate));
        decayCoef = (float) std::exp (-4.6 / (std::max (decay, 0.001f) * sampleRate));
        const double r = std::max ((double) release, kMinReleaseSec);
        const double relSamp = r * sampleRate;
        releaseLinear = relSamp < 8.0;                       // muito curto: rampa linear (queda seca)
        releaseSamples = std::max (1, (int) std::lround (relSamp));
        releaseCoef = (float) std::exp (-4.6 / relSamp);
    }
    double sampleRate = 44100.0;
    float attack = 0.005f, decay = 0.3f, sustain = 0.8f, release = 0.2f;
    float attackInc = 0.01f, decayCoef = 0.999f, releaseCoef = 0.999f, linearStep = 0.f, level = 0.f;
    int releaseSamples = 3; bool releaseLinear = false;
    Stage stage = Stage::Idle;
};
}
