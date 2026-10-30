#pragma once
// Vector Envelope do ES3 — Parte 5.
// O ES2 possui um "vector envelope" que movimenta automaticamente a posição (X,Y) no triângulo
// de mixagem ao longo do tempo, com pontos definidos pelo usuário (até 16).
// Cada ponto tem (x, y, time), e o envelope pode fazer loop.
// Implementação PROVISÓRIA: envelope linear entre pontos, loop opcional.
#include <algorithm>
#include <array>
#include <cmath>

namespace es3
{
struct VectorPoint
{
    float x = 0.f, y = 0.f;   // posição no triângulo (0..1, 0..1)
    float time = 0.5f;         // tempo para chegar ao PRÓXIMO ponto (segundos)
};

static constexpr int kMaxVectorPoints = 16;

struct VectorEnvParams
{
    int numPoints = 1;                                    // 1 = sem movimento (posição fixa)
    int loopStart = -1, loopEnd = -1;                    // -1 = sem loop
    std::array<VectorPoint, kMaxVectorPoints> points {};
    bool enabled = false;
};

class VectorEnvelope
{
public:
    void setSampleRate (double sr) { sampleRate = sr; }
    void setParams (const VectorEnvParams& p) { params = p; }

    void trigger()
    {
        segment = 0;
        phase = 0.0;
        done = false;
    }

    // Retorna (x, y) offset para mixar com a posição base.
    // Se o envelope não está ativo ou terminou, retorna (0, 0).
    void next (float& outX, float& outY)
    {
        if (!params.enabled || params.numPoints < 2 || done)
        {
            outX = outY = 0.f;
            return;
        }

        const int n = std::clamp (params.numPoints, 2, kMaxVectorPoints);
        if (segment >= n - 1)
        {
            // Verificar loop
            if (params.loopStart >= 0 && params.loopEnd >= params.loopStart && params.loopEnd < n)
            {
                segment = params.loopStart;
                phase = 0.0;
            }
            else
            {
                // Fim do envelope: fica no último ponto
                outX = params.points[(size_t)(n - 1)].x;
                outY = params.points[(size_t)(n - 1)].y;
                done = true;
                return;
            }
        }

        const auto& a = params.points[(size_t)segment];
        const auto& b = params.points[(size_t)(segment + 1)];
        const float t = (float) phase;

        outX = a.x + (b.x - a.x) * t;
        outY = a.y + (b.y - a.y) * t;

        // Avançar fase
        const double segTime = std::max ((double) a.time, 0.001);
        phase += 1.0 / (segTime * sampleRate);
        if (phase >= 1.0)
        {
            phase -= 1.0;
            ++segment;
            // Checar se precisa fazer loop
            if (segment >= n - 1 && params.loopStart >= 0 && params.loopEnd >= params.loopStart
                && segment > params.loopEnd && params.loopEnd < n - 1)
            {
                segment = params.loopStart;
            }
        }
    }

    bool isActive() const { return params.enabled && !done; }

private:
    VectorEnvParams params {};
    double sampleRate = 44100.0;
    double phase = 0.0;
    int segment = 0;
    bool done = false;
};

} // namespace es3
