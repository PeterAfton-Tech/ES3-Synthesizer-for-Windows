#pragma once
// Router de modulação do ES3 — Parte 5. Baseado na documentação do ES2 (Apple, Sound On Sound).
// O ES2 tem um Router com 10 slots, cada um com Target, Via, Source e Amount.
// O "Via" multiplica a profundidade do Source pelo valor de uma segunda fonte (ex: ModWheel escalona o LFO).
// Implementação: PROVISÓRIA. Fontes e destinos serão expandidos conforme necessário.
#include <algorithm>
#include <array>
#include <cmath>

namespace es3
{
// Fontes de modulação (ordem estável: presets dependem dos índices).
enum class ModSource
{
    Off = 0, ENV1, ENV2, ENV3, LFO1, LFO2,
    Velocity, KeyTrack, ModWheel, Aftertouch, PitchBend,
    Random,
    kCount
};

// Destinos de modulação.
enum class ModTarget
{
    Off = 0,
    Pitch,          // afinação global (todos os osciladores)
    Osc1Pitch, Osc2Pitch, Osc3Pitch,
    Cutoff1, Cutoff2, CutoffBoth,
    Resonance1, Resonance2,
    PW1, PW2, PW3,
    FM,             // FM do Osc 1
    Volume,
    Pan,
    MixX, MixY,
    LFO1Rate, LFO2Rate,
    Drive,
    Osc2Wave,       // reservado para modulação de wave (digiwave)
    kCount
};

struct ModSlot
{
    ModTarget target = ModTarget::Off;
    ModSource source = ModSource::Off;
    ModSource via    = ModSource::Off;
    float amount     = 0.f;   // -1..+1
};

static constexpr int kMaxModSlots = 10;

// Offsets de modulação acumulados por voz a cada bloco.
struct ModOffsets
{
    float pitch      = 0.f;   // semitons
    float osc1Pitch  = 0.f;
    float osc2Pitch  = 0.f;
    float osc3Pitch  = 0.f;
    float cutoff1    = 0.f;   // oitavas de modulação
    float cutoff2    = 0.f;
    float res1       = 0.f;
    float res2       = 0.f;
    float pw1        = 0.f;
    float pw2        = 0.f;
    float pw3        = 0.f;
    float fm         = 0.f;
    float volume     = 0.f;
    float pan        = 0.f;
    float mixX       = 0.f;
    float mixY       = 0.f;
    float lfo1Rate   = 0.f;
    float lfo2Rate   = 0.f;
    float drive      = 0.f;

    void clear()
    {
        pitch = osc1Pitch = osc2Pitch = osc3Pitch = 0.f;
        cutoff1 = cutoff2 = res1 = res2 = 0.f;
        pw1 = pw2 = pw3 = fm = 0.f;
        volume = pan = mixX = mixY = 0.f;
        lfo1Rate = lfo2Rate = drive = 0.f;
    }
};

// Contexto com os valores de todas as fontes para uma voz num instante.
struct ModContext
{
    float env1 = 0.f, env2 = 0.f, env3 = 0.f;
    float lfo1 = 0.f, lfo2 = 0.f;
    float velocity = 0.f;        // 0..1
    float keyTrack = 0.f;        // (note-60)/60, normalizado ~-1..+1
    float modWheel = 0.f;        // 0..1
    float aftertouch = 0.f;      // 0..1
    float pitchBend = 0.f;       // -1..+1
    float random = 0.f;          // 0..1 (fixo por nota)
};

inline float getSourceValue (const ModContext& ctx, ModSource src)
{
    switch (src)
    {
        case ModSource::Off:        return 0.f;
        case ModSource::ENV1:       return ctx.env1;
        case ModSource::ENV2:       return ctx.env2;
        case ModSource::ENV3:       return ctx.env3;
        case ModSource::LFO1:       return ctx.lfo1;
        case ModSource::LFO2:       return ctx.lfo2;
        case ModSource::Velocity:   return ctx.velocity;
        case ModSource::KeyTrack:   return ctx.keyTrack;
        case ModSource::ModWheel:   return ctx.modWheel;
        case ModSource::Aftertouch: return ctx.aftertouch;
        case ModSource::PitchBend:  return ctx.pitchBend;
        case ModSource::Random:     return ctx.random;
        default: return 0.f;
    }
}

inline void applySlotToOffsets (const ModSlot& slot, const ModContext& ctx, ModOffsets& off)
{
    if (slot.target == ModTarget::Off || slot.source == ModSource::Off) return;
    float val = getSourceValue (ctx, slot.source) * slot.amount;
    if (slot.via != ModSource::Off)
        val *= getSourceValue (ctx, slot.via);

    // Escalas dos destinos: os ranges são PROVISÓRIOS.
    // Pitch: amount ±1 → ±24 semitons (2 oitavas).
    // Cutoff: amount ±1 → ±8 oitavas.
    // PW: amount ±1 → ±0.5 (adicionado ao pw base).
    // Volume: ±1 → ±1 (multiplicador).
    // Pan: ±1 → ±1.
    // Mix: ±1 → ±1.
    // FM: ±1 → ±1.
    // LFO Rate: ±1 → ±10 Hz.
    // Drive: ±1 → ±1.
    // Resonance: ±1 → ±1.
    switch (slot.target)
    {
        case ModTarget::Pitch:      off.pitch += val * 24.f; break;
        case ModTarget::Osc1Pitch:  off.osc1Pitch += val * 24.f; break;
        case ModTarget::Osc2Pitch:  off.osc2Pitch += val * 24.f; break;
        case ModTarget::Osc3Pitch:  off.osc3Pitch += val * 24.f; break;
        case ModTarget::Cutoff1:    off.cutoff1 += val * 8.f; break;
        case ModTarget::Cutoff2:    off.cutoff2 += val * 8.f; break;
        case ModTarget::CutoffBoth: off.cutoff1 += val * 8.f; off.cutoff2 += val * 8.f; break;
        case ModTarget::Resonance1: off.res1 += val; break;
        case ModTarget::Resonance2: off.res2 += val; break;
        case ModTarget::PW1:        off.pw1 += val * 0.5f; break;
        case ModTarget::PW2:        off.pw2 += val * 0.5f; break;
        case ModTarget::PW3:        off.pw3 += val * 0.5f; break;
        case ModTarget::FM:         off.fm += val; break;
        case ModTarget::Volume:     off.volume += val; break;
        case ModTarget::Pan:        off.pan += val; break;
        case ModTarget::MixX:       off.mixX += val; break;
        case ModTarget::MixY:       off.mixY += val; break;
        case ModTarget::LFO1Rate:   off.lfo1Rate += val * 10.f; break;
        case ModTarget::LFO2Rate:   off.lfo2Rate += val * 10.f; break;
        case ModTarget::Drive:      off.drive += val; break;
        default: break;
    }
}

inline ModOffsets computeModOffsets (const std::array<ModSlot, kMaxModSlots>& slots, const ModContext& ctx)
{
    ModOffsets off;
    off.clear();
    for (auto& s : slots)
        applySlotToOffsets (s, ctx, off);
    return off;
}

} // namespace es3
