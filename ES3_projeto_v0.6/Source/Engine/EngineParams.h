#pragma once
#include <array>
#include "Oscillator.h"
#include "Filter.h"
#include "Lfo.h"
#include "Router.h"
#include "VectorEnvelope.h"
#include "Effects.h"

namespace es3
{
// Quanto do aliasing "ingênuo" é mantido nas formas serra/quadrada (0 = PolyBLEP completo, 1 = ingênuo).
// VALOR PROVISÓRIO: o ES2 original não foi medido. Calibrar com gravações de referência autorizadas.
constexpr float kDefaultAliasing = 0.5f;

struct OscParams
{
    Wave  wave = Wave::Saw;
    int   coarse = 0;          // semitons (±36)
    float fine = 0.f;          // cents
    float pw = 0.f;            // largura de pulso 0..1 (0 = 50%)
    float fm = 0.f;            // só Osc 1: quantidade de FM por Osc 2 (onda senoide)
    float noiseColor = 0.f;    // só Osc 3: cor do ruído
};

struct LfoParams
{
    LfoWave wave = LfoWave::Sine;
    float rate = 5.f;          // Hz
    float fadeSeconds = 0.f;   // fade-in em segundos
    bool  sync = false;        // sincronizar ao tempo do DAW (futuro)
};

struct EngineParams
{
    std::array<OscParams, 3> osc {};
    float mixX = 0.f, mixY = 0.f;
    float tuneCents = 0.f;
    float bendRange = 2.f;
    int   maxVoices = 8;
    int   voiceMode = 0;          // 0 Poly, 1 Mono, 2 Legato
    int   oscStart = 0;           // 0 Free (aleatório), 1 Soft, 2 Hard
    float aliasing = kDefaultAliasing;
    float ampAttack = 0.005f, ampDecay = 0.3f, ampSustain = 0.8f, ampRelease = 0.2f;
    float env2Attack = 0.005f, env2Decay = 0.3f, env2Sustain = 0.f, env2Release = 0.2f;   // ENV2 = envelope do filtro
    float env3Attack = 0.005f, env3Decay = 0.3f, env3Sustain = 0.f, env3Release = 0.2f;   // ENV3 = envelope de modulação livre
    FilterParams flt {};
    float volumeDb = 0.f;

    // Parte 5: Router, LFOs, Vector, Glide
    std::array<ModSlot, kMaxModSlots> modSlots {};
    LfoParams lfo1 {}, lfo2 {};
    VectorEnvParams vectorEnv {};
    float glideTime = 0.f;        // segundos (0 = sem glide)
    float sineLevel = 0.f;        // nível extra de senoide de sub-osc (0..1), presente no ES2
    float analog = 0.f;           // 0..1 drift analógico de afinação
    bool  unison = false;         // modo unison
    float fltFm = 0.f;            // 0..1 modulação FM de filtro

    // Parte 6: Efeitos
    EffectParams effects {};
};
}
