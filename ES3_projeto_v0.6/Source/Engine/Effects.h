#pragma once
// Módulo de Efeitos do ES3 — Parte 6
// Recria os efeitos integrados do ES2:
// 1. Distortion (Off, Soft, Hard) com controle de Tone (Dark a Bright).
// 2. Modulation FX (Off, Chorus, Flanger, Phaser) com Intensity e Speed.

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace es3
{

enum class DistType : int
{
    Off = 0,
    Soft = 1,
    Hard = 2
};

enum class ModFxType : int
{
    Off = 0,
    Chorus = 1,
    Flanger = 2,
    Phaser = 3
};

struct EffectParams
{
    DistType  distType = DistType::Off;
    float     distDrive = 0.f;        // 0..1
    float     distTone = 0.5f;        // 0 (dark) .. 1 (bright)

    ModFxType modType = ModFxType::Off;
    float     modIntensity = 0.f;     // 0..1
    float     modSpeed = 1.0f;        // 0.05 .. 15 Hz
};

class EffectsProcessor
{
public:
    void prepare(double sr)
    {
        sampleRate = std::max(22050.0, sr);

        // Aloca buffer circular para chorus / flanger (máx ~50 ms @ 192 kHz)
        maxDelaySamples = static_cast<int>(sampleRate * 0.050) + 16;
        delayBufferL.assign(maxDelaySamples, 0.0f);
        delayBufferR.assign(maxDelaySamples, 0.0f);
        writeIndex = 0;

        lfoPhase = 0.0;
        toneStateL = 0.0f;
        toneStateR = 0.0f;
        dcBlockL = 0.0f;
        dcBlockR = 0.0f;
        dcPrevInL = 0.0f;
        dcPrevInR = 0.0f;

        // Reset phaser all-pass filters (4 stages per channel)
        for (int i = 0; i < 4; ++i)
        {
            apStateL[i] = 0.0f;
            apStateR[i] = 0.0f;
        }
        phaserFeedbackL = 0.0f;
        phaserFeedbackR = 0.0f;
        flangerFeedbackL = 0.0f;
        flangerFeedbackR = 0.0f;
    }

    void reset()
    {
        std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
        std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
        writeIndex = 0;
        toneStateL = toneStateR = 0.0f;
        dcBlockL = dcBlockR = dcPrevInL = dcPrevInR = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            apStateL[i] = apStateR[i] = 0.0f;
        }
        phaserFeedbackL = phaserFeedbackR = 0.0f;
        flangerFeedbackL = flangerFeedbackR = 0.0f;
    }

    void process(float* left, float* right, int numSamples, const EffectParams& params)
    {
        if (numSamples <= 0) return;

        // Pré-cálculo do tone filter para distorção
        // Tone varia de lowpass suave (dark) a highpass/shelving (bright)
        // tone = 0.5 é neutro.
        const float tone = std::clamp(params.distTone, 0.0f, 1.0f);
        // cutoff varia de ~400 Hz a ~12000 Hz
        const float toneFreq = 400.0f * std::pow(30.0f, tone);
        const float toneCoeff = std::clamp(static_cast<float>(2.0 * 3.14159265358979323846 * toneFreq / sampleRate), 0.001f, 0.95f);

        // Parâmetros de modulação
        const float modLfoInc = static_cast<float>(2.0 * 3.14159265358979323846 * std::clamp(params.modSpeed, 0.05f, 15.0f) / sampleRate);
        const float intensity = std::clamp(params.modIntensity, 0.0f, 1.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = left[i];
            float inR = right[i];

            // 1. DISTORTION
            if (params.distType != DistType::Off && params.distDrive > 0.001f)
            {
                // Drive ganho: de 1x (0 dB) a até ~32x (+30 dB)
                const float driveGain = 1.0f + params.distDrive * params.distDrive * 30.0f;
                float dL = inL * driveGain;
                float dR = inR * driveGain;

                if (params.distType == DistType::Soft)
                {
                    // Soft clipping suave com saturação harmônica tipo válvula (tanh)
                    dL = std::tanh(dL);
                    dR = std::tanh(dR);
                }
                else if (params.distType == DistType::Hard)
                {
                    // Hard clipping puro do ES2
                    dL = std::clamp(dL, -1.0f, 1.0f);
                    dR = std::clamp(dR, -1.0f, 1.0f);
                }

                // DC blocker pós distorção para evitar offset contínuo
                const float rDC = 0.995f;
                dcBlockL = dL - dcPrevInL + rDC * dcBlockL;
                dcPrevInL = dL;
                dL = dcBlockL;

                dcBlockR = dR - dcPrevInR + rDC * dcBlockR;
                dcPrevInR = dR;
                dR = dcBlockR;

                // Tone control (One-pole lowpass / tilt)
                toneStateL += toneCoeff * (dL - toneStateL);
                toneStateR += toneCoeff * (dR - toneStateR);

                // Se tone > 0.5, mistura com o sinal original brilhante; se tone < 0.5, atenua agudos
                float outDistL, outDistR;
                if (tone >= 0.5f)
                {
                    float brightMix = (tone - 0.5f) * 2.0f;
                    outDistL = (1.0f - brightMix) * toneStateL + brightMix * dL;
                    outDistR = (1.0f - brightMix) * toneStateR + brightMix * dR;
                }
                else
                {
                    outDistL = toneStateL;
                    outDistR = toneStateR;
                }

                // Compensação de ganho para não estourar o mixer
                const float comp = 1.0f / (1.0f + params.distDrive * 2.5f);
                inL = outDistL * comp;
                inR = outDistR * comp;
            }

            // 2. MODULATION FX (Chorus, Flanger, Phaser)
            if (params.modType != ModFxType::Off && intensity > 0.001f)
            {
                // Atualiza LFO
                lfoPhase += modLfoInc;
                if (lfoPhase >= 2.0 * 3.14159265358979323846)
                    lfoPhase -= 2.0 * 3.14159265358979323846;

                const float lfoL = static_cast<float>(std::sin(lfoPhase));
                // Canal direito defasado em 90 graus para abertura estéreo
                const float lfoR = static_cast<float>(std::cos(lfoPhase));

                if (params.modType == ModFxType::Chorus)
                {
                    // CHORUS: delay médio ~18ms, excursão de ~4ms
                    delayBufferL[writeIndex] = inL;
                    delayBufferR[writeIndex] = inR;

                    const float baseDelayMs = 18.0f;
                    const float modDepthMs = 4.0f * intensity;

                    float delayMsL = baseDelayMs + modDepthMs * lfoL;
                    float delayMsR = baseDelayMs + modDepthMs * lfoR;

                    float delaySampL = static_cast<float>(delayMsL * 0.001 * sampleRate);
                    float delaySampR = static_cast<float>(delayMsR * 0.001 * sampleRate);

                    float wetL = readDelayLinear(delayBufferL, writeIndex, delaySampL);
                    float wetR = readDelayLinear(delayBufferR, writeIndex, delaySampR);

                    // Mistura dry / wet (máx 50% para chorus tradicional)
                    float wetMix = intensity * 0.6f;
                    inL = (1.0f - wetMix * 0.5f) * inL + wetMix * wetL;
                    inR = (1.0f - wetMix * 0.5f) * inR + wetMix * wetR;
                }
                else if (params.modType == ModFxType::Flanger)
                {
                    // FLANGER: delay curto ~2.5ms a ~4.5ms com feedback metálico
                    const float feedback = 0.65f * intensity;
                    delayBufferL[writeIndex] = inL + flangerFeedbackL * feedback;
                    delayBufferR[writeIndex] = inR + flangerFeedbackR * feedback;

                    const float baseDelayMs = 2.5f;
                    const float modDepthMs = 2.0f * intensity;

                    float delayMsL = baseDelayMs + modDepthMs * (0.5f + 0.5f * lfoL);
                    float delayMsR = baseDelayMs + modDepthMs * (0.5f + 0.5f * lfoR);

                    float delaySampL = static_cast<float>(delayMsL * 0.001 * sampleRate);
                    float delaySampR = static_cast<float>(delayMsR * 0.001 * sampleRate);

                    float wetL = readDelayLinear(delayBufferL, writeIndex, delaySampL);
                    float wetR = readDelayLinear(delayBufferR, writeIndex, delaySampR);

                    flangerFeedbackL = std::clamp(wetL, -1.5f, 1.5f);
                    flangerFeedbackR = std::clamp(wetR, -1.5f, 1.5f);

                    float wetMix = intensity * 0.7f;
                    inL = inL + wetMix * wetL;
                    inR = inR + wetMix * wetR;
                }
                else if (params.modType == ModFxType::Phaser)
                {
                    // PHASER: 4 estágios all-pass em cascata com feedback
                    // LFO varre a frequência de corte de ~300 Hz a ~3500 Hz
                    const float minFreq = 300.0f;
                    const float maxFreq = 3500.0f;
                    const float fL = minFreq * std::pow(maxFreq / minFreq, 0.5f + 0.5f * lfoL * intensity);
                    const float fR = minFreq * std::pow(maxFreq / minFreq, 0.5f + 0.5f * lfoR * intensity);

                    // Coeficiente do filtro all-pass: a = (tan(pi*f/sr) - 1) / (tan(pi*f/sr) + 1)
                    float tanL = std::tan(static_cast<float>(3.14159265358979323846 * fL / sampleRate));
                    float aL = (tanL - 1.0f) / (tanL + 1.0f);

                    float tanR = std::tan(static_cast<float>(3.14159265358979323846 * fR / sampleRate));
                    float aR = (tanR - 1.0f) / (tanR + 1.0f);

                    const float phaserFb = 0.6f * intensity;
                    float sL = inL + phaserFeedbackL * phaserFb;
                    float sR = inR + phaserFeedbackR * phaserFb;

                    for (int st = 0; st < 4; ++st)
                    {
                        // Allpass: y[n] = a * x[n] + x[n-1] - a * y[n-1]
                        float yL = aL * sL + apStateL[st];
                        apStateL[st] = sL - aL * yL;
                        sL = yL;

                        float yR = aR * sR + apStateR[st];
                        apStateR[st] = sR - aR * yR;
                        sR = yR;
                    }

                    phaserFeedbackL = std::clamp(sL, -1.5f, 1.5f);
                    phaserFeedbackR = std::clamp(sR, -1.5f, 1.5f);

                    // Mistura com sinal direto cria os entalhes característicos
                    float wetMix = intensity * 0.7f;
                    inL = inL + wetMix * sL;
                    inR = inR + wetMix * sR;
                }

                writeIndex = (writeIndex + 1) % maxDelaySamples;
            }

            left[i] = inL;
            right[i] = inR;
        }
    }

private:
    float readDelayLinear(const std::vector<float>& buf, int currentWrite, float delaySamples) const
    {
        float readPos = static_cast<float>(currentWrite) - delaySamples;
        while (readPos < 0.0f) readPos += static_cast<float>(maxDelaySamples);

        int idx0 = static_cast<int>(readPos);
        int idx1 = (idx0 + 1) % maxDelaySamples;
        float frac = readPos - static_cast<float>(idx0);

        return (1.0f - frac) * buf[idx0] + frac * buf[idx1];
    }

    double sampleRate = 48000.0;
    int maxDelaySamples = 4096;
    int writeIndex = 0;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;

    double lfoPhase = 0.0;
    float toneStateL = 0.0f;
    float toneStateR = 0.0f;
    float dcBlockL = 0.0f;
    float dcBlockR = 0.0f;
    float dcPrevInL = 0.0f;
    float dcPrevInR = 0.0f;

    // Phaser states
    std::array<float, 4> apStateL {};
    std::array<float, 4> apStateR {};
    float phaserFeedbackL = 0.0f;
    float phaserFeedbackR = 0.0f;

    // Flanger feedback
    float flangerFeedbackL = 0.0f;
    float flangerFeedbackR = 0.0f;
};

} // namespace es3
