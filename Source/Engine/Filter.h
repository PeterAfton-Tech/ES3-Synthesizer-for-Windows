#pragma once
// Filtros do ES3 (independentes do JUCE). Estrutura segue a documentação da Apple/SOS:
//  - Filtro 1: multimodo 12 dB/oit (Lo, Hi, Peak, BP, BR)
//  - Filtro 2: passa-baixa de 12/18/24 dB/oit, mais "Fat" (24 dB com compensação de graves)
//  - Roteamento série/paralelo, Blend -1..+1, Drive.
// Topologias e curvas de ressonância/drive são MODELOS PRÓPRIOS (não é o circuito do ES2): PROVISÓRIO.
#include <algorithm>
#include <cmath>
#include "Oscillator.h"

namespace es3
{
enum class F1Mode { Lo = 0, Hi, Peak, BP, BR };
enum class F2Slope { Db12 = 0, Db18, Db24, Fat };

struct FilterParams
{
    bool  on = true, series = true, reset = false;
    F1Mode f1Mode = F1Mode::Lo;
    F2Slope f2Slope = F2Slope::Db24;
    float f1Cut = 20000.f, f1Res = 0.f, f2Cut = 20000.f, f2Res = 0.f;   // Hz, 0..1
    float blend = 0.f;       // -1 = só Filtro 1, +1 = só Filtro 2
    float drive = 0.f;       // 0..1
    float envAmt = 0.f;      // ENV2 -> Cut 1+2 (-1..1); será substituído pelo Router (Parte 5)
    float keyTrack = 0.f;    // 0..1 (1 = segue a nota a 100%)
};

constexpr double kEnvOctaves = 8.0;   // oitavas de modulação com envAmt = 1 (PROVISÓRIO)

inline double effectiveCutoff (double base, float envAmt, float env, float keyTrack, int note)
{
    const double oct = (double) envAmt * kEnvOctaves * (double) env + (double) keyTrack * (note - 60) / 12.0;
    return std::clamp (base * std::pow (2.0, oct), 10.0, 22000.0);
}

// SVF de 2 polos (TPT/ZDF)
class Svf
{
public:
    void reset() { ic1 = ic2 = 0.0; }
    void set (double fc, double sr, double res)
    {
        fc = std::clamp (fc, 10.0, 0.45 * sr);
        g = std::tan (kPi * fc / sr);
        k = 1.414 + (0.02 - 1.414) * std::clamp (res, 0.0, 1.0);
        a1 = 1.0 / (1.0 + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    // lo, band(normalizada, ganho 1 no centro), hi
    void tick (double x, double& lo, double& bn, double& hi)
    {
        const double v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0 * v1 - ic1; ic2 = 2.0 * v2 - ic2;
        lo = v2; bn = k * v1; hi = x - k * v1 - v2;
    }
    double getK() const { return k; }
private:
    double g = 0.1, k = 1.414, a1 = 1, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
};

// Escada de N polos simples com realimentação (resolvida sem atraso). N = 3 ou 4.
class Ladder
{
public:
    void reset() { for (auto& v : z) v = 0.0; }
    void set (double fc, double sr, double res, int numPoles, bool compensate)
    {
        n = numPoles; fc = std::clamp (fc, 10.0, 0.45 * sr);
        const double g = std::tan (kPi * fc / sr); G = g / (1.0 + g);
        k = std::clamp (res, 0.0, 1.0) * (n == 3 ? 8.0 : 4.0);   // k de auto-oscilação: 8 (3 polos), 4 (4 polos)
        inGain = compensate ? 1.0 + k : 1.0;
    }
    double tick (double x)
    {
        x *= inGain;
        double Gn = 1.0, S = 0.0;               // S = soma das contribuições dos estados
        double coef[4];
        for (int i = n - 1; i >= 0; --i) { coef[i] = Gn * (1.0 - G); Gn *= G; }
        for (int i = 0; i < n; ++i) S += coef[i] * z[i];
        const double yN = (Gn * x + S) / (1.0 + k * Gn);
        double u = x - k * yN;
        for (int i = 0; i < n; ++i) { const double y = G * u + (1.0 - G) * z[i]; z[i] = 2.0 * y - z[i]; u = y; }
        return u;
    }
private:
    int n = 4; double G = 0.1, k = 0.0, inGain = 1.0, z[4] = { 0, 0, 0, 0 };
};

class FilterSection
{
public:
    void reset() { f1.reset(); f2svf.reset(); lad.reset(); }

    void setCutoffs (const FilterParams& p, double c1, double c2, double sr)
    {
        params = p;
        f1.set (c1, sr, p.f1Res);
        if (p.f2Slope == F2Slope::Db12) f2svf.set (c2, sr, p.f2Res);
        else lad.set (c2, sr, p.f2Res, p.f2Slope == F2Slope::Db18 ? 3 : 4, p.f2Slope == F2Slope::Fat);
        driveGain = 1.0 + 8.0 * (double) p.drive;
        // Filtros totalmente abertos (cutoff no máximo, sem ressonância nem drive) são transparentes
        open = p.f1Mode == F1Mode::Lo && c1 >= 19500.0 && c2 >= 19500.0 && p.f1Res < 0.01f && p.f2Res < 0.01f && p.drive <= 0.f;
    }

    float process (float xin)
    {
        const auto& p = params;
        const double x = (double) xin;
        if (open) return xin;
        if (p.series)
        {
            const double d1 = drive (x);
            const double y1 = runF1 (d1);
            const double b = (double) p.blend;
            const double in2 = b > 0.0 ? y1 + (x - y1) * b : y1;       // Blend>0 contorna parcialmente o Filtro 1
            const double y2 = runF2 (drive (in2));
            return (float) (b < 0.0 ? y2 + (y1 - y2) * (-b) : y2);     // Blend<0 contorna parcialmente o Filtro 2
        }
        const double d = drive (x);                                    // paralelo: um único Drive antes dos dois
        const double b = (double) p.blend, wA = 0.5 * (1.0 - b), wB = 0.5 * (1.0 + b);
        return (float) (wA * runF1 (d) + wB * runF2 (d));
    }

private:
    double drive (double x) const { return driveGain <= 1.0 ? x : std::tanh (driveGain * x) / driveGain * (1.0 + 0.5 * (driveGain - 1.0) / 8.0); }
    double runF1 (double x)
    {
        double lo, bn, hi; f1.tick (x, lo, bn, hi);
        switch (params.f1Mode)
        {
            case F1Mode::Lo:   return lo;
            case F1Mode::Hi:   return hi;
            case F1Mode::BP:   return bn;
            case F1Mode::BR:   return x - bn;
            case F1Mode::Peak: return x + 3.0 * (double) params.f1Res * bn;   // até +12 dB no centro (PROVISÓRIO)
        }
        return lo;
    }
    double runF2 (double x)
    {
        if (params.f2Slope == F2Slope::Db12) { double lo, bn, hi; f2svf.tick (x, lo, bn, hi); return lo; }
        return lad.tick (x);
    }
    FilterParams params {};
    Svf f1, f2svf; Ladder lad; double driveGain = 1.0; bool open = true;
};
}
