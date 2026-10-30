#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "EngineParams.h"
#include "Envelope.h"
#include "Filter.h"
#include "Lfo.h"
#include "Oscillator.h"
#include "Router.h"
#include "VectorEnvelope.h"
#include "Effects.h"

namespace es3
{
// Mapeamento PROVISÓRIO do triângulo (coordenadas normalizadas 0..1):
// vértices: Osc1 (0,0), Osc2 (1,0.5), Osc3 (0,1); pesos baricêntricos lineares.
// Fora do triângulo: pesos negativos zerados e renormalizados (aproximação).
inline void computeMixGains (float x, float y, float g[3])
{
    x = std::clamp (x, 0.f, 1.f);
    y = std::clamp (y, 0.f, 1.f);
    float w1 = 1.f - 0.5f * x - y, w2 = x, w3 = y - 0.5f * x;
    w1 = std::max (w1, 0.f); w2 = std::max (w2, 0.f); w3 = std::max (w3, 0.f);
    float sum = w1 + w2 + w3;
    if (sum < 1.0e-6f) { w1 = 1.f; w2 = w3 = 0.f; sum = 1.f; }
    g[0] = w1 / sum; g[1] = w2 / sum; g[2] = w3 / sum;
}

class AudioEngine
{
public:
    static constexpr int kMaxVoices = 16;
    static constexpr int kFadeSamples = 96;   // fade-out ao roubar voz (evita clique)
    static constexpr int kChunk = 256;

    void prepare (double sr)
    {
        sampleRate = sr;
        for (auto& v : voices)
        {
            v = Voice {};
            v.env.setSampleRate (sr);
            v.env2.setSampleRate (sr);
            v.env3.setSampleRate (sr);
            v.lfo1.setSampleRate (sr);
            applyEnv (v);
        }
        globalLfo2.setSampleRate (sr);
        effectsProcessor.prepare (sr);
        stackCount = 0; sustainDown = false; bendNorm = 0.f; modWheel = 0.f; aftertouch = 0.f;
        gainSmooth = (float) (1.0 - std::exp (-1.0 / (0.005 * sr)));
        masterGain = targetGain;
    }

    void setParams (const EngineParams& p)
    {
        params = p;
        params.maxVoices = std::clamp (params.maxVoices, 1, kMaxVoices);
        targetGain = (float) std::pow (10.0, params.volumeDb / 20.0);

        // LFO2 global params
        globalLfo2.setWave (params.lfo2.wave);
        globalLfo2.setRate (params.lfo2.rate);
        globalLfo2.setFadeSeconds (params.lfo2.fadeSeconds);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            applyEnv (voices[i]);
            // LFO1 per-voice params
            voices[i].lfo1.setWave (params.lfo1.wave);
            voices[i].lfo1.setRate (params.lfo1.rate);
            voices[i].lfo1.setFadeSeconds (params.lfo1.fadeSeconds);

            if (params.voiceMode == 0 && i >= params.maxVoices && voices[i].active && voices[i].key)
                keyRelease (voices[i]);
        }
    }

    void noteOn (int note, float vel)
    {
        note = std::clamp (note, 0, 127);
        vel = std::clamp (vel, 1.f / 127.f, 1.f);
        if (params.voiceMode == 0) polyNoteOn (note, vel); else monoNoteOn (note, vel);
    }

    void noteOff (int note)
    {
        note = std::clamp (note, 0, 127);
        if (params.voiceMode == 0) polyNoteOff (note); else monoNoteOff (note);
    }

    void sustainPedal (bool down)
    {
        sustainDown = down;
        if (down) return;
        for (auto& v : voices)
            if (v.sustained) { v.sustained = false; if (! v.key) releaseEnvs (v); }
    }

    void pitchBend (float normalised) { bendNorm = std::clamp (normalised, -1.f, 1.f); }
    void setModWheel (float v)        { modWheel = std::clamp (v, 0.f, 1.f); }
    void setAftertouch (float v)      { aftertouch = std::clamp (v, 0.f, 1.f); }

    void allNotesOff()
    {
        stackCount = 0;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            v.key = false; v.sustained = false;
            if (v.stealing) v.pendNote = -1;
            releaseEnvs (v);
        }
    }

    void allSoundOff()
    {
        stackCount = 0;
        effectsProcessor.reset();
        for (auto& v : voices)
        {
            v.active = false; v.key = false; v.sustained = false; v.stealing = false;
            v.env.reset(); v.env2.reset(); v.env3.reset();
        }
    }

    int activeVoiceCount() const
    {
        int n = 0;
        for (auto& v : voices) if (v.active) ++n;
        return n;
    }

    // SOMA o resultado em l/r (o chamador deve zerar os buffers antes).
    void process (float* l, float* r, int n)
    {
        int done = 0;
        while (done < n)
        {
            const int c = std::min (kChunk, n - done);
            renderChunk (l + done, r + done, c);
            done += c;
        }
        effectsProcessor.process (l, r, n, params.effects);
    }

private:
    struct Voice
    {
        bool active = false, key = false, sustained = false, stealing = false;
        int note = -1, pendNote = -1, fadeLeft = 0;
        float velocity = 0.f, pendVel = 0.f;
        float randomMod = 0.f;          // valor aleatório fixo por nota (para fonte Random)
        uint64_t age = 0;
        std::array<Oscillator, 3> osc {};
        double fmPrev = 0.0;
        Envelope env, env2, env3;
        Lfo lfo1;
        VectorEnvelope vectorEnv;
        FilterSection flt;
        // Glide
        double glideFreq[3] = {0, 0, 0};   // frequência atual com glide
        double glideTarget[3] = {0, 0, 0};
        bool glideActive = false;
        float analogDrift = 0.f;           // drift analógico por nota
        double subPhase = 0.0;             // fase do sub-oscilador seno
    };

    static constexpr float kVoiceGain = 0.25f;   // headroom provisório
    static constexpr double kFmDepthCycles = 1.5; // profundidade máx. de FM (PROVISÓRIO)

    void applyEnv (Voice& v)
    {
        v.env.setParams (params.ampAttack, params.ampDecay, params.ampSustain, params.ampRelease);
        v.env2.setParams (params.env2Attack, params.env2Decay, params.env2Sustain, params.env2Release);
        v.env3.setParams (params.env3Attack, params.env3Decay, params.env3Sustain, params.env3Release);
    }
    static void releaseEnvs (Voice& v) { v.env.noteOff(); v.env2.noteOff(); v.env3.noteOff(); }
    static void triggerEnvs (Voice& v) { v.env.noteOn(); v.env2.noteOn(); v.env3.noteOn(); }

    static double startPhase (Wave w, bool hard)
    {
        switch (w)
        {
            case Wave::Sine:     return hard ? 0.25 : 0.0;     // pico / cruzamento por zero (PROVISÓRIO)
            case Wave::Triangle: return hard ? 0.0 : 0.25;
            case Wave::Saw:
            case Wave::SyncSaw:  return hard ? 0.9999 : 0.5;
            default:             return 0.0;
        }
    }

    void initPhases (Voice& v)
    {
        for (int i = 0; i < 3; ++i)
        {
            double ph = 0.0;
            if (params.oscStart == 0) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; ph = (double) (rng >> 8) / 16777216.0; }
            else ph = startPhase (params.osc[(size_t) i].wave, params.oscStart == 2);
            v.osc[(size_t) i].reset (ph);
        }
    }

    void startVoice (Voice& v, int note, float vel, bool resetPhase)
    {
        v.active = true; v.key = true; v.sustained = false; v.stealing = false;
        v.note = note; v.velocity = vel; v.pendNote = -1;
        v.age = ++ageCounter;
        // Random modulation source: fixed per note
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        v.randomMod = (float) (rng >> 8) / 16777216.f;
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        v.analogDrift = ((float) (rng & 0xFFFF) / 65536.f - 0.5f) * 0.25f;

        if (resetPhase) { initPhases (v); v.fmPrev = 0.0; }
        if (params.flt.reset) v.flt.reset();

        // LFO1 retrigger per voice
        v.lfo1.retrigger (0.0);
        // Vector envelope trigger
        v.vectorEnv.setSampleRate (sampleRate);
        v.vectorEnv.setParams (params.vectorEnv);
        v.vectorEnv.trigger();
        // Glide: set up target frequencies but keep current if glide is active
        v.glideActive = params.glideTime > 0.001f && v.glideFreq[0] > 0.0;

        triggerEnvs (v);
    }

    void updateFilter (Voice& v, const ModOffsets& mod)
    {
        const auto& f = params.flt;
        // Apply modulation offsets to cutoff (in octaves) and resonance
        const float envAmt1 = f.envAmt + mod.cutoff1 / 8.f;
        const float envAmt2 = f.envAmt + mod.cutoff2 / 8.f;
        const float e2 = v.env2.isActive() ? 0.f : 0.f;  // we pass e2 separately
        // Note: We need the actual env2 value here; we get it from the mod context
        // For backward compat, use the existing effectiveCutoff but add router offsets
        const float res1 = std::clamp (f.f1Res + mod.res1, 0.f, 1.f);
        const float res2 = std::clamp (f.f2Res + mod.res2, 0.f, 1.f);
        const float drv = std::clamp (f.drive + mod.drive, 0.f, 1.f);

        FilterParams modFlt = f;
        modFlt.f1Res = res1;
        modFlt.f2Res = res2;
        modFlt.drive = drv;

        const double c1 = std::clamp (f.f1Cut * std::pow (2.0, (double) mod.cutoff1), 10.0, 22000.0);
        const double c2 = std::clamp (f.f2Cut * std::pow (2.0, (double) mod.cutoff2), 10.0, 22000.0);
        v.flt.setCutoffs (modFlt, c1, c2, sampleRate);
    }

    void keyRelease (Voice& v)
    {
        v.key = false;
        if (sustainDown) v.sustained = true; else releaseEnvs (v);
    }

    void polyNoteOn (int note, float vel)
    {
        for (int i = 0; i < params.maxVoices; ++i)
            if (voices[i].active && ! voices[i].stealing && voices[i].note == note)
            { startVoice (voices[i], note, vel, false); return; }

        for (int i = 0; i < params.maxVoices; ++i)
            if (! voices[i].active) { startVoice (voices[i], note, vel, true); return; }

        int pick = -1; bool pickReleasing = false; uint64_t pickAge = 0;
        for (int i = 0; i < params.maxVoices; ++i)
        {
            const bool releasing = ! voices[i].key && ! voices[i].sustained;
            const uint64_t a = voices[i].age;
            if (pick < 0 || (releasing && ! pickReleasing) || (releasing == pickReleasing && a < pickAge))
            { pick = i; pickReleasing = releasing; pickAge = a; }
        }
        Voice& v = voices[pick];
        if (! v.stealing) { v.stealing = true; v.fadeLeft = kFadeSamples; }
        v.pendNote = note; v.pendVel = vel;
    }

    void polyNoteOff (int note)
    {
        for (auto& v : voices)
        {
            if (! v.active) continue;
            if (v.stealing && v.pendNote == note) { v.pendNote = -1; continue; }
            if (! v.stealing && v.key && v.note == note) keyRelease (v);
        }
    }

    void stackRemove (int note)
    {
        int j = 0;
        for (int i = 0; i < stackCount; ++i) if (stack[i] != note) stack[j++] = stack[i];
        stackCount = j;
    }

    void monoNoteOn (int note, float vel)
    {
        const bool anyHeld = stackCount > 0;
        stackRemove (note);
        if (stackCount < (int) stack.size()) stack[stackCount++] = note;
        Voice& v = voices[0];
        if (params.voiceMode == 2 && anyHeld && v.active)
        { v.note = note; v.velocity = vel; v.key = true; v.age = ++ageCounter; v.glideActive = params.glideTime > 0.001f; }
        else
            startVoice (v, note, vel, ! v.active);
    }

    void monoNoteOff (int note)
    {
        const bool wasTop = stackCount > 0 && stack[stackCount - 1] == note;
        stackRemove (note);
        Voice& v = voices[0];
        if (! v.active) return;
        if (stackCount > 0)
        {
            if (wasTop)
            {
                v.note = stack[stackCount - 1];
                v.glideActive = params.glideTime > 0.001f;
                if (params.voiceMode == 1) triggerEnvs (v);   // Mono retrigga, Legato não
            }
        }
        else keyRelease (v);
    }

    void renderChunk (float* l, float* r, int n)
    {
        std::array<float, kChunk> monoL {}, monoR {};
        const double bend = (double) bendNorm * params.bendRange;
        const double tune = params.tuneCents / 100.0;
        const float globalLfo2Val = globalLfo2.next();   // LFO2 monofônico: um valor por chunk

        for (auto& v : voices)
        {
            if (! v.active) continue;

            for (int s = 0; s < n; ++s)
            {
                const float ampEnv = v.env.next();
                const float e2 = v.env2.next();
                const float e3 = v.env3.next();
                const float lfo1Val = v.lfo1.next();

                // Build modulation context
                ModContext ctx;
                ctx.env1 = ampEnv;
                ctx.env2 = e2;
                ctx.env3 = e3;
                ctx.lfo1 = lfo1Val;
                ctx.lfo2 = globalLfo2Val;
                ctx.velocity = v.velocity;
                ctx.keyTrack = (float) (v.note - 60) / 60.f;
                ctx.modWheel = modWheel;
                ctx.aftertouch = aftertouch;
                ctx.pitchBend = bendNorm;
                ctx.random = v.randomMod;

                // Compute modulation offsets via Router
                ModOffsets mod = computeModOffsets (params.modSlots, ctx);

                // Also apply legacy ENV2→cutoff (envAmt) for backward compatibility
                // The envAmt is a direct link: ENV2 * envAmt * 8 octaves → cutoff
                mod.cutoff1 += params.flt.envAmt * 8.f * e2 + params.flt.keyTrack * (v.note - 60) / 12.f;
                mod.cutoff2 += params.flt.envAmt * 8.f * e2 + params.flt.keyTrack * (v.note - 60) / 12.f;

                // Update filter every 8 samples
                if ((s & 7) == 0) updateFilter (v, mod);

                // Vector envelope → mix position
                float vecX = 0.f, vecY = 0.f;
                v.vectorEnv.next (vecX, vecY);
                const float effMixX = std::clamp (params.mixX + mod.mixX + vecX, 0.f, 1.f);
                const float effMixY = std::clamp (params.mixY + mod.mixY + vecY, 0.f, 1.f);
                float g[3]; computeMixGains (effMixX, effMixY, g);

                // Compute frequencies with pitch modulation
                double f[3];
                for (int i = 0; i < 3; ++i)
                {
                    float pitchMod = mod.pitch;
                    if (i == 0) pitchMod += mod.osc1Pitch;
                    if (i == 1) pitchMod += mod.osc2Pitch;
                    if (i == 2) pitchMod += mod.osc3Pitch;
                    const double semis = (v.note - 69) + params.osc[i].coarse + params.osc[i].fine / 100.0 + tune + bend + pitchMod + (v.analogDrift * params.analog);
                    double freq = 440.0 * std::pow (2.0, semis / 12.0);

                    // Glide
                    if (v.glideActive && params.glideTime > 0.001f)
                    {
                        v.glideTarget[i] = freq;
                        if (v.glideFreq[i] <= 0.0) v.glideFreq[i] = freq;
                        const double glideCoef = 1.0 - std::exp (-1.0 / (params.glideTime * sampleRate));
                        v.glideFreq[i] += (freq - v.glideFreq[i]) * glideCoef;
                        f[i] = v.glideFreq[i];
                    }
                    else
                    {
                        f[i] = freq;
                        v.glideFreq[i] = freq;
                    }
                }
                v.osc[2].setNoiseColor (params.osc[2].noiseColor);

                float fade = 1.f;
                if (v.stealing)
                {
                    fade = (float) v.fadeLeft / (float) kFadeSamples;
                    if (--v.fadeLeft <= 0)
                    {
                        if (v.pendNote >= 0) { startVoice (v, v.pendNote, v.pendVel, true); }
                        else { v.active = false; v.stealing = false; v.env.reset(); v.env2.reset(); v.env3.reset(); }
                    }
                }
                else if (! v.env.isActive()) { v.active = false; break; }

                float outs[3];
                {
                    double dt[3];
                    for (int i = 0; i < 3; ++i) dt[i] = Oscillator::clampDt (f[i] / sampleRate);
                    const float bs = 1.f - std::clamp (params.aliasing, 0.f, 1.f);

                    // Apply PW modulation
                    float pw0 = std::clamp (params.osc[0].pw + mod.pw1, 0.f, 1.f);
                    float pw1 = std::clamp (params.osc[1].pw + mod.pw2, 0.f, 1.f);
                    float pw2 = std::clamp (params.osc[2].pw + mod.pw3, 0.f, 1.f);

                    // FM amount with modulation
                    float fmAmt = std::clamp (params.osc[0].fm + mod.fm, 0.f, 1.f);

                    // Osc 1 (FM por Osc 2 apenas na onda senoide; usa a saída anterior de Osc 2)
                    double pm = 0.0;
                    if (params.osc[0].wave == Wave::Sine && fmAmt > 0.f)
                        pm = (double) fmAmt * kFmDepthCycles * v.fmPrev;
                    outs[0] = v.osc[0].shape (params.osc[0].wave, dt[0], pw0, bs, pm);
                    v.osc[0].advance (dt[0]);
                    const bool wrap1 = v.osc[0].didWrap();
                    const double frac1 = v.osc[0].wrapFraction();

                    for (int i = 1; i < 3; ++i)
                    {
                        const Wave w = params.osc[(size_t) i].wave;
                        const float pw = (i == 1) ? pw1 : pw2;
                        if (w == Wave::Ring && i == 1)   // anel: saída de Osc 1 x onda quadrada de Osc 2
                            outs[i] = outs[0] * v.osc[(size_t) i].shape (Wave::Square, dt[i], pw, bs);
                        else
                            outs[i] = v.osc[(size_t) i].shape (w, dt[i], pw, bs);
                        v.osc[(size_t) i].advance (dt[i]);
                        if ((w == Wave::SyncSquare || w == Wave::SyncSaw) && wrap1)
                            v.osc[(size_t) i].setPhase (dt[i] * frac1);   // sync rígido com reinício sub-amostra
                    }
                    v.fmPrev = outs[1];
                }
                float sum = g[0] * outs[0] + g[1] * outs[1] + g[2] * outs[2];

                // Sine Level (sub-oscillator sine, 1 oitava abaixo, presente no ES2)
                if (params.sineLevel > 0.001f)
                {
                    const double sineSemis = (v.note - 69) - 12.0 + tune + bend;
                    const double sineFreq = 440.0 * std::pow (2.0, sineSemis / 12.0);
                    const double dtSine = Oscillator::clampDt (sineFreq / sampleRate);
                    sum += params.sineLevel * (float) std::sin (2.0 * kPi * v.subPhase);
                    v.subPhase += dtSine;
                    if (v.subPhase >= 1.0) v.subPhase -= 1.0;
                }

                if (params.flt.on) sum = v.flt.process (sum);

                // Volume modulation
                float volMod = 1.f + mod.volume;
                volMod = std::max (volMod, 0.f);

                // Pan modulation (provisório: -1 esquerda, +1 direita, 0 centro)
                const float panVal = std::clamp (mod.pan, -1.f, 1.f);
                const float panL = std::cos ((panVal + 1.f) * 0.25f * (float) kPi);
                const float panR = std::sin ((panVal + 1.f) * 0.25f * (float) kPi);

                const float sample = sum * ampEnv * v.velocity * fade * kVoiceGain * volMod;
                monoL[s] += sample * panL;
                monoR[s] += sample * panR;
                if (! v.active) break;
            }
        }

        for (int s = 0; s < n; ++s)
        {
            masterGain += (targetGain - masterGain) * gainSmooth;
            l[s] += monoL[s] * masterGain;
            r[s] += monoR[s] * masterGain;
        }
    }

    EngineParams params {};
    std::array<Voice, kMaxVoices> voices {};
    std::array<int, 32> stack {};
    int stackCount = 0;
    double sampleRate = 44100.0;
    float bendNorm = 0.f, modWheel = 0.f, aftertouch = 0.f;
    float masterGain = 1.f, targetGain = 1.f, gainSmooth = 0.001f;
    bool sustainDown = false;
    uint64_t ageCounter = 0;
    uint32_t rng = 0x9E3779B9u;
    Lfo globalLfo2;   // LFO2 é monofônico (global)
    EffectsProcessor effectsProcessor;
};
}
