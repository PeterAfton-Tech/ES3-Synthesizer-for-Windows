// Testes do núcleo de DSP (sem JUCE). Executável: es3_engine_tests
#include <cstdio>
#include <cmath>
#include <vector>
#include "Engine/AudioEngine.h"
#include "Engine/Lfo.h"

using namespace es3;
static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++failures; std::printf("  FALHOU: "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static std::vector<float> render (AudioEngine& e, int n)
{
    std::vector<float> l (n, 0.f), r (n, 0.f);
    e.process (l.data(), r.data(), n);
    return l;
}

// frequência por cruzamentos ascendentes por zero, com interpolação linear
static double measureFreq (const std::vector<float>& x, double sr, int from)
{
    double first = -1, last = -1; int count = 0;
    for (size_t i = (size_t) from + 1; i < x.size(); ++i)
        if (x[i - 1] < 0.f && x[i] >= 0.f)
        {
            const double t = (double) (i - 1) + (-x[i - 1]) / (double) (x[i] - x[i - 1]);
            if (first < 0) first = t;
            last = t; ++count;
        }
    return count < 2 ? 0.0 : (count - 1) * sr / (last - first);
}

static EngineParams sineParams()
{
    EngineParams p; p.osc[0].wave = Wave::Sine; p.mixX = 0; p.mixY = 0;
    p.ampAttack = 0.0005f; p.ampSustain = 1.f; p.ampRelease = 0.05f; return p;
}

static double peak (const std::vector<float>& x) { double m = 0; for (float v : x) m = std::max (m, (double) std::fabs (v)); return m; }
static bool finite (const std::vector<float>& x) { for (float v : x) if (! std::isfinite (v)) return false; return true; }

static void testPitch()
{
    std::printf("[pitch]\n");
    const double sr = 48000;
    struct C { int note, coarse; float fine, tune, bend; double expect; } cases[] = {
        { 69, 0, 0, 0, 0, 440.0 }, { 45, 0, 0, 0, 0, 110.0 }, { 93, 0, 0, 0, 0, 1760.0 },
        { 69, 12, 0, 0, 0, 880.0 }, { 69, 0, 100, 0, 0, 466.1638 }, { 69, 0, 0, 100, 0, 466.1638 },
        { 69, 0, 0, 0, 1.f, 493.8833 } };
    for (auto& c : cases)
    {
        AudioEngine e; e.prepare (sr); auto p = sineParams();
        p.osc[0].coarse = c.coarse; p.osc[0].fine = c.fine; p.tuneCents = c.tune; p.bendRange = 2;
        e.setParams (p); e.pitchBend (c.bend); e.noteOn (c.note, 1.f);
        auto x = render (e, 48000);
        const double f = measureFreq (x, sr, 2000);
        std::printf("  nota %d -> medido %.4f Hz (esperado %.4f)\n", c.note, f, c.expect);
        CHECK (std::fabs (f - c.expect) < 0.05, "afinação fora de 0.05 Hz");
    }
}

static void testMix()
{
    std::printf("[mix triangulo]\n");
    float g[3];
    struct P { float x, y; float e0, e1, e2; } pts[] = { {0,0,1,0,0}, {1,.5f,0,1,0}, {0,1,0,0,1}, {1.f/3,.5f,1/3.f,1/3.f,1/3.f},
        {.5f,.25f,.5f,.5f,0}, {0,.5f,.5f,0,.5f}, {.5f,.75f,0,.5f,.5f} };
    for (auto& p : pts)
    {
        computeMixGains (p.x, p.y, g);
        std::printf("  (%.3f,%.3f) -> %.3f %.3f %.3f\n", p.x, p.y, g[0], g[1], g[2]);
        CHECK (std::fabs (g[0]-p.e0) < 1e-3 && std::fabs (g[1]-p.e1) < 1e-3 && std::fabs (g[2]-p.e2) < 1e-3, "ganhos errados");
    }
    for (float x = -0.5f; x <= 1.5f; x += 0.1f)
        for (float y = -0.5f; y <= 1.5f; y += 0.1f)
        { computeMixGains (x, y, g); CHECK (std::fabs (g[0]+g[1]+g[2]-1.f) < 1e-4 && g[0] >= 0 && g[1] >= 0 && g[2] >= 0, "soma != 1"); }
}

static void testEnvelopeAndRelease()
{
    std::printf("[envelope/release]\n");
    AudioEngine e; e.prepare (44100); auto p = sineParams(); p.ampRelease = 0.1f; e.setParams (p);
    e.noteOn (60, 1.f);
    auto x = render (e, 22050);
    CHECK (peak (x) > 0.1, "sem som");
    e.noteOff (60);
    render (e, 44100 * 2);
    CHECK (e.activeVoiceCount() == 0, "voz não terminou após release");
    auto y = render (e, 512);
    CHECK (peak (y) == 0.0, "ruído residual após fim da voz");
    std::printf("  pico com nota sustentada: %.3f\n", peak (x));
}

static void testPolyAndSteal()
{
    std::printf("[polifonia/roubo de voz]\n");
    AudioEngine e; e.prepare (44100); auto p = sineParams(); p.maxVoices = 4; e.setParams (p);
    e.noteOn (60, 1.f); e.noteOn (64, 1.f); e.noteOn (67, 1.f);
    render (e, 1000);
    CHECK (e.activeVoiceCount() == 3, "esperava 3 vozes");
    for (int n = 70; n < 80; ++n) { e.noteOn (n, 1.f); auto x = render (e, 300); CHECK (finite (x) && peak (x) < 2.0, "saída inválida no roubo"); CHECK (e.activeVoiceCount() <= 4, "mais vozes que o limite"); }
    e.allNotesOff(); render (e, 44100 * 2);
    CHECK (e.activeVoiceCount() == 0, "allNotesOff não terminou vozes");
}

static void testSustain()
{
    std::printf("[sustain pedal]\n");
    AudioEngine e; e.prepare (44100); auto p = sineParams(); e.setParams (p);
    e.sustainPedal (true); e.noteOn (60, 1.f); render (e, 4410); e.noteOff (60);
    auto x = render (e, 44100);
    CHECK (e.activeVoiceCount() == 1 && peak (x) > 0.1, "pedal não segurou a nota");
    e.sustainPedal (false); render (e, 44100);
    CHECK (e.activeVoiceCount() == 0, "nota não soltou ao levantar o pedal");
}

static void testMono()
{
    std::printf("[mono/legato]\n");
    const double sr = 48000;
    AudioEngine e; e.prepare (sr); auto p = sineParams(); p.voiceMode = 1; e.setParams (p);
    e.noteOn (60, 1.f); render (e, 4800); e.noteOn (64, 1.f); render (e, 4800);
    auto a = render (e, 24000); double fa = measureFreq (a, sr, 100);
    e.noteOff (64); render (e, 4800); auto b = render (e, 24000); double fb = measureFreq (b, sr, 100);
    std::printf("  64 -> %.3f Hz, volta a 60 -> %.3f Hz\n", fa, fb);
    CHECK (std::fabs (fa - 329.628) < 0.1, "nota 64 incorreta");
    CHECK (std::fabs (fb - 261.626) < 0.1, "não voltou para nota 60 da pilha");
    CHECK (e.activeVoiceCount() == 1, "mono deve ter 1 voz");
    AudioEngine e2; e2.prepare (sr); p.voiceMode = 2; e2.setParams (p);
    e2.noteOn (60, 1.f); render (e2, 4800); e2.noteOn (67, 1.f);
    auto c = render (e2, 24000); CHECK (finite (c) && e2.activeVoiceCount() == 1, "legato falhou");
}

static void testAliasing()
{
    std::printf("[aliasing da serra: PolyBLEP completo / parcial / ingênuo]\n");
    const double sr = 48000; const int N = 8192; const double f0 = 4186.0091;
    auto energyOutside = [&] (float blepScale)
    {
        std::vector<double> x (N); Oscillator o;
        for (int i = 0; i < N; ++i)
            x[i] = o.process (Wave::Saw, f0, sr, 0.f, blepScale) * 0.5 * (1 - std::cos (2 * kPi * i / (N - 1)));
        double tot = 0, bad = 0;
        for (int k = 1; k < N / 2; ++k)
        {
            double re = 0, im = 0;
            for (int i = 0; i < N; ++i) { const double a = 2 * kPi * k * i / N; re += x[i] * std::cos (a); im -= x[i] * std::sin (a); }
            const double m = re * re + im * im; tot += m;
            const double fk = k * sr / N; bool harm = false;
            for (int h = 1; h * f0 < sr / 2; ++h) if (std::fabs (fk - h * f0) < 4 * sr / N) harm = true;
            if (! harm) bad += m;
        }
        return bad / tot;
    };
    const double full = energyOutside (1.f), mid = energyOutside (1.f - kDefaultAliasing), naive = energyOutside (0.f);
    std::printf("  energia fora dos harmônicos: PolyBLEP %.5f | padrão (aliasing=%.2f) %.5f | ingênua %.5f\n", full, kDefaultAliasing, mid, naive);
    CHECK (full < mid && mid < naive, "níveis de aliasing fora de ordem");
}

static double goertzel (const std::vector<float>& x, double sr, double f, int from, int n)
{
    double re = 0, im = 0;
    for (int i = 0; i < n; ++i)
    {
        const double w = 0.5 * (1 - std::cos (2 * kPi * i / (n - 1)));
        const double a = 2 * kPi * f * (double) i / sr;
        re += x[(size_t) (from + i)] * w * std::cos (a); im -= x[(size_t) (from + i)] * w * std::sin (a);
    }
    return re * re + im * im;
}

static EngineParams osc2Only (Wave w2)
{
    EngineParams p; p.oscStart = 1; p.mixX = 1.f; p.mixY = 0.5f;   // só Osc 2 audível
    p.osc[0].wave = Wave::Saw; p.osc[1].wave = w2;
    p.ampAttack = 0.0005f; p.ampSustain = 1.f; return p;
}

static void testSync()
{
    std::printf("[sync rígido]\n");
    auto periodicity = [] (Wave w2)
    {
        AudioEngine e; e.prepare (44000);   // 440 Hz => período exato de 100 amostras
        auto p = osc2Only (w2); p.osc[1].coarse = 17; p.aliasing = 0.f; e.setParams (p); e.noteOn (69, 1.f);
        auto x = render (e, 12000);
        double d = 0, t = 0;
        for (int i = 4000; i < 8000; ++i) { d += std::fabs (x[(size_t) i + 100] - x[(size_t) i]); t += std::fabs (x[(size_t) i]); }
        return d / t;
    };
    const double sync = periodicity (Wave::SyncSaw), free = periodicity (Wave::Saw);
    std::printf("  erro de periodicidade (100 amostras): sync %.4f | sem sync %.4f\n", sync, free);
    CHECK (sync < 0.02, "sync deveria forçar o período do Osc 1");
    CHECK (free > 0.3, "sem sync não deveria ser periódico em 100 amostras");
}

static void testRing()
{
    std::printf("[modulação em anel]\n");
    const double sr = 48000;
    AudioEngine e; e.prepare (sr);
    auto p = osc2Only (Wave::Ring); p.osc[0].wave = Wave::Sine; p.osc[1].coarse = 7; e.setParams (p); e.noteOn (69, 1.f);
    auto x = render (e, 24000);
    const double f2 = 440.0 * std::pow (2.0, 7.0 / 12.0);
    const double pDiff = goertzel (x, sr, f2 - 440.0, 4000, 16384), pCarrier = goertzel (x, sr, 440.0, 4000, 16384);
    std::printf("  potência em f2-f1 (%.1f Hz): %.3g | em f1 (440 Hz): %.3g\n", f2 - 440.0, pDiff, pCarrier);
    CHECK (pDiff > 100.0 * pCarrier, "anel deveria suprimir a portadora e gerar f2-f1");
}

static void testFM()
{
    std::printf("[FM do Osc 1 por Osc 2]\n");
    const double sr = 48000;
    auto h2 = [&] (float fm)
    {
        AudioEngine e; e.prepare (sr);
        EngineParams p; p.oscStart = 1; p.mixX = 0; p.mixY = 0; p.ampAttack = 0.0005f; p.ampSustain = 1.f;
        p.osc[0].wave = Wave::Sine; p.osc[0].fm = fm; p.osc[1].wave = Wave::Sine;
        e.setParams (p); e.noteOn (69, 1.f);
        auto x = render (e, 24000);
        return goertzel (x, sr, 880.0, 4000, 16384);
    };
    const double off = h2 (0.f), on = h2 (0.5f);
    std::printf("  potência em 880 Hz: FM=0 %.3g | FM=0.5 %.3g\n", off, on);
    CHECK (on > 100.0 * off, "FM não gerou bandas laterais");
}

static void testPulseWidth()
{
    std::printf("[largura de pulso]\n");
    auto posFrac = [] (float pw)
    {
        AudioEngine e; e.prepare (48000);
        EngineParams p; p.oscStart = 1; p.mixX = 0; p.mixY = 0; p.ampAttack = 0.0005f; p.ampSustain = 1.f;
        p.osc[0].wave = Wave::Square; p.osc[0].pw = pw; e.setParams (p); e.noteOn (33, 1.f);
        auto x = render (e, 24000); int pos = 0, n = 0;
        for (int i = 4000; i < 20000; ++i) { ++n; if (x[(size_t) i] > 0.f) ++pos; }
        return (double) pos / n;
    };
    const double a = posFrac (0.f), b = posFrac (1.f);
    std::printf("  fração positiva: pw=0 -> %.3f | pw=1 -> %.3f\n", a, b);
    CHECK (std::fabs (a - 0.5) < 0.03, "pw=0 deveria ser ~50%%");
    CHECK (b > 0.01 && b < 0.035, "pw=1 deveria ser ~2%%");
}

static void testNoiseColor()
{
    std::printf("[cor do ruído]\n");
    auto diffRatio = [] (float color)
    {
        AudioEngine e; e.prepare (48000);
        EngineParams p; p.mixX = 0; p.mixY = 1; p.ampAttack = 0.0005f; p.ampSustain = 1.f;
        p.osc[2].wave = Wave::Noise; p.osc[2].noiseColor = color; e.setParams (p); e.noteOn (60, 1.f);
        auto x = render (e, 24000); double d = 0, t = 0;
        for (int i = 4000; i < 24000; ++i) { d += std::pow (x[(size_t) i] - x[(size_t) i - 1], 2); t += std::pow (x[(size_t) i], 2); }
        return d / t;
    };
    const double w = diffRatio (0.f), c = diffRatio (1.f);
    std::printf("  razão de energia da derivada: branco %.3f | cor=1 %.3f\n", w, c);
    CHECK (w > 1.5 && c < 0.5 * w, "cor do ruído não escureceu o espectro");
}

static void testOscStart()
{
    std::printf("[osc start]\n");
    auto run = [] (int mode)
    {
        AudioEngine e; e.prepare (44100); EngineParams p; p.oscStart = mode; e.setParams (p);
        e.noteOn (60, 1.f); return render (e, 2000);
    };
    auto a = run (1), b = run (1), c = run (2);
    CHECK (a == b, "Soft deveria ser determinístico");
    CHECK (a != c, "Hard e Soft deveriam diferir");
}

static void testInstantEnvelope()
{
    std::printf("[envelope: ataque e release mínimos (evidência das gravações)]\n");
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        Envelope e; e.setSampleRate (sr); e.setParams (0.f, 0.3f, 0.8f, 0.f); e.noteOn();
        int up = 0; float v = 0.f;
        while (v < 1.f && up < 100) { v = e.next(); ++up; }
        for (int i = 0; i < (int) (0.5 * sr); ++i) e.next();   // vai ao sustain
        const float sus = e.next(); e.noteOff();
        int down = 0; while (e.isActive() && down < 100) { e.next(); ++down; }
        std::printf("  %.0f Hz: sobe a 1.0 em %d amostras (%.0f us); sustain %.2f cai a 0 em %d amostras (%.0f us)\n", sr, up, 1e6 * up / sr, sus, down, 1e6 * down / sr);
        CHECK (up <= 5, "attack=0 deveria subir quase instantaneamente");
        CHECK (down <= 6, "release=0 deveria cair quase instantaneamente");
        CHECK (std::fabs (sus - 0.8f) < 1e-3f, "sustain errado");
    }
    Envelope e; e.setSampleRate (48000); e.setParams (0.01f, 0.1f, 0.5f, 0.5f); e.noteOn();
    int n = 0; float v = 0; while (v < 1.f && n < 100000) { v = e.next(); ++n; }
    std::printf("  attack=10 ms: sobe em %d amostras\n", n);
    CHECK (n > 400 && n < 600, "attack de 10 ms fora do esperado");
}

static void testNoiseDistribution()
{
    std::printf("[ruído gaussiano]\n");
    Oscillator o; const int N = 400000; double s2 = 0, s4 = 0;
    for (int i = 0; i < N; ++i) { const double x = o.shape (Wave::Noise, 0.01, 0.f, 1.f); s2 += x * x; s4 += x * x * x * x; }
    s2 /= N; s4 /= N; const double kurt = s4 / (s2 * s2);
    std::printf("  RMS %.3f | curtose %.3f (referência medida: 3.01)\n", std::sqrt (s2), kurt);
    CHECK (kurt > 2.85 && kurt < 3.15, "curtose fora de ~3");
    CHECK (std::fabs (std::sqrt (s2) - 0.3) < 0.01, "RMS fora de 0.3");
}


// ---------------- Filtros ----------------
static double filterGainDb (FilterParams fp, double hz, double sr = 48000.0)
{
    FilterSection f; f.reset(); fp.on = true; f.setCutoffs (fp, fp.f1Cut, fp.f2Cut, sr);
    const int n = (int) (0.6 * sr); double acc = 0, ref = 0; int cnt = 0;
    for (int i = 0; i < n; ++i)
    {
        const double x = std::sin (2 * kPi * hz * i / sr);
        const double y = f.process ((float) x);
        if (i > n / 2) { acc += y * y; ref += x * x; ++cnt; }
    }
    return 10.0 * std::log10 (acc / ref + 1e-30);
}
static FilterParams only2 (F2Slope sl, float cut, float res = 0.f)
{ FilterParams p; p.series = false; p.blend = 1.f; p.f2Slope = sl; p.f2Cut = cut; p.f2Res = res; p.f1Cut = 20000.f; return p; }
static FilterParams only1 (F1Mode m, float cut, float res = 0.f)
{ FilterParams p; p.series = false; p.blend = -1.f; p.f1Mode = m; p.f1Cut = cut; p.f1Res = res; p.f2Cut = 20000.f; return p; }

static void testFilterModes()
{
    std::printf("[filtro 1: modos]\n");
    auto lo = only1 (F1Mode::Lo, 1000), hi = only1 (F1Mode::Hi, 1000), bp = only1 (F1Mode::BP, 1000), br = only1 (F1Mode::BR, 1000);
    const double a = filterGainDb (lo, 4000), b = filterGainDb (lo, 1000), c = filterGainDb (hi, 250), d = filterGainDb (hi, 8000);
    const double e = filterGainDb (bp, 1000), f = filterGainDb (br, 1000), g = filterGainDb (br, 100);
    auto pk = only1 (F1Mode::Peak, 1000, 1.f);
    const double h = filterGainDb (pk, 1000), i = filterGainDb (pk, 100);
    std::printf("  Lo: 4k %.1f dB (esp. -24) | fc %.1f dB (esp. -3) ; Hi: 250 Hz %.1f dB (esp. -24) | 8k %.1f dB (esp. 0)\n", a, b, c, d);
    std::printf("  BP fc %.1f dB (esp. 0) ; BR fc %.1f dB (< -20) | 100 Hz %.1f dB (0) ; Peak res=1: fc %.1f dB (esp. +12) | 100 Hz %.1f dB (0)\n", e, f, g, h, i);
    CHECK (std::fabs (a + 24.1) < 1.5, "Lo 12 dB/oit"); CHECK (b > -4.5 && b < -1.5, "Lo no corte");
    CHECK (std::fabs (c + 24.1) < 1.5, "Hi 12 dB/oit"); CHECK (std::fabs (d) < 0.5, "Hi passa agudos");
    CHECK (std::fabs (e) < 0.7, "BP ganho 1 no centro"); CHECK (f < -20.0, "BR rejeita o centro"); CHECK (std::fabs (g) < 0.5, "BR passa graves");
    CHECK (std::fabs (h - 12.0) < 1.5, "Peak +12 dB"); CHECK (std::fabs (i) < 0.5, "Peak neutro longe do centro");
}

static void testFilter2Slopes()
{
    std::printf("[filtro 2: inclinações e Fat]\n");
    struct S { F2Slope s; const char* n; double db; } sl[] = { { F2Slope::Db12, "12 dB", 12 }, { F2Slope::Db18, "18 dB", 18 }, { F2Slope::Db24, "24 dB", 24 }, { F2Slope::Fat, "Fat", 24 } };
    for (auto& x : sl)
    {
        const double slope = filterGainDb (only2 (x.s, 250), 2000) - filterGainDb (only2 (x.s, 250), 4000);
        std::printf("  %-6s queda entre 2 kHz e 4 kHz: %.1f dB/oitava (esp. %.0f)\n", x.n, slope, x.db);
        CHECK (std::fabs (slope - x.db) < 1.5, "inclinação do filtro 2");
    }
    const double lp24 = filterGainDb (only2 (F2Slope::Db24, 2000, 0.5f), 100), fat = filterGainDb (only2 (F2Slope::Fat, 2000, 0.5f), 100);
    std::printf("  graves a res=0.5: 24 dB %.1f dB (perde graves, esp. ~-9.5) | Fat %.1f dB (esp. ~0)\n", lp24, fat);
    CHECK (std::fabs (lp24 + 9.5) < 1.5, "24 dB deveria perder graves com ressonância");
    CHECK (std::fabs (fat) < 1.5, "Fat deveria compensar os graves");
}

static void testFilterRoutingAndBlend()
{
    std::printf("[roteamento e blend]\n");
    FilterParams base; base.f1Mode = F1Mode::Lo; base.f1Cut = 1000; base.f2Slope = F2Slope::Db24; base.f2Cut = 1000;
    auto at = [&] (bool series, float blend) { auto p = base; p.series = series; p.blend = blend; return filterGainDb (p, 4000); };
    const double f1only = filterGainDb (only1 (F1Mode::Lo, 1000), 4000), f2only = filterGainDb (only2 (F2Slope::Db24, 1000), 4000);
    std::printf("  só F1 %.1f dB | só F2 %.1f dB\n", f1only, f2only);
    for (bool ser : { false, true })
    {
        const double m1 = at (ser, -1.f), p1 = at (ser, 1.f);
        std::printf("  %s: blend -1 -> %.1f dB | blend +1 -> %.1f dB\n", ser ? "série   " : "paralelo", m1, p1);
        CHECK (std::fabs (m1 - f1only) < 0.5, "blend -1 deveria ser só o Filtro 1");
        CHECK (std::fabs (p1 - f2only) < 0.5, "blend +1 deveria ser só o Filtro 2");
    }
    const double mid = at (false, 0.f);
    std::printf("  paralelo blend 0 (50/50): %.1f dB (esp. ~%.1f)\n", mid, f1only - 6.0);
    CHECK (std::fabs (mid - (f1only - 6.0)) < 1.5, "50/50 em paralelo");
}

static void testFilterResonance()
{
    std::printf("[ressonância e auto-oscilação]\n");
    auto lo0 = only1 (F1Mode::Lo, 1000, 0.f), lo1 = only1 (F1Mode::Lo, 1000, 1.f);
    const double g0 = filterGainDb (lo0, 1000), g1 = filterGainDb (lo1, 1000);
    std::printf("  F1 Lo no corte: res=0 %.1f dB | res=1 %.1f dB\n", g0, g1);
    CHECK (g1 > 25.0, "ressonância máxima deveria dar pico alto");
    // auto-oscilação do filtro 2 (24 dB, res=1)
    const double sr = 48000.0; FilterSection f; auto p = only2 (F2Slope::Db24, 1000, 1.f); f.reset(); f.setCutoffs (p, 20000, 1000, sr);
    std::vector<float> y; for (int i = 0; i < (int) sr; ++i) y.push_back (f.process (i == 0 ? 1.f : 0.f));
    auto rms = [&] (int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += y[(size_t) i] * y[(size_t) i]; return std::sqrt (s / (b - a)); };
    const double r1 = rms (9600, 14400), r2 = rms (38400, 43200);
    const double fz = measureFreq (std::vector<float> (y.begin() + 9600, y.begin() + 43200), sr, 0);
    std::printf("  24 dB res=1 após impulso: RMS 0.2 s %.4f | 0.8 s %.4f | frequência %.1f Hz\n", r1, r2, fz);
    CHECK (r1 > 1e-4 && r2 / r1 > 0.5 && r2 / r1 < 2.0, "deveria sustentar a oscilação");
    CHECK (std::fabs (fz - 1000.0) < 40.0, "frequência de auto-oscilação ~ cutoff");
}

static void testFilterStability()
{
    std::printf("[estabilidade dos filtros]\n");
    uint32_t s = 777; auto rnd = [&] { s = s * 1664525u + 1013904223u; return (s >> 8) & 0xFFFF; };
    double pk = 0; bool ok = true;
    for (int trial = 0; trial < 60; ++trial)
    {
        FilterSection f; FilterParams p; p.series = rnd() & 1; p.f1Mode = (F1Mode) (rnd() % 5); p.f2Slope = (F2Slope) (rnd() % 4);
        p.f1Res = (rnd() % 1001) / 1000.f; p.f2Res = (rnd() % 1001) / 1000.f; p.blend = (rnd() % 2001) / 1000.f - 1.f; p.drive = (rnd() % 1001) / 1000.f;
        f.reset();
        for (int i = 0; i < 40000; ++i)
        {
            if (i % 8 == 0) f.setCutoffs (p, 20.0 * std::pow (1000.0, (rnd() % 1000) / 999.0), 20.0 * std::pow (1000.0, (rnd() % 1000) / 999.0), 44100.0);
            const float y = f.process ((float) ((int) (rnd() % 2001) - 1000) / 1000.f);
            if (! std::isfinite (y)) { ok = false; }
            pk = std::max (pk, (double) std::fabs (y));
        }
    }
    std::printf("  pico máximo com cutoff/ressonância/drive aleatórios: %.1f\n", pk);
    CHECK (ok, "NaN/inf no filtro"); CHECK (pk < 500.0, "filtro instável");
}

static void testEffectiveCutoff()
{
    std::printf("[modulação de cutoff: key e ENV2]\n");
    const double a = effectiveCutoff (1000, 0, 0, 1.f, 72), b = effectiveCutoff (1000, 0, 0, 1.f, 48), c = effectiveCutoff (1000, 0.5f, 1.f, 0.f, 60), d = effectiveCutoff (1000, 0, 0, 0.f, 90);
    std::printf("  key +1 oitava %.1f | key -1 oitava %.1f | ENV2 0.5 x 8 oit %.1f | sem mod %.1f\n", a, b, c, d);
    CHECK (std::fabs (a - 2000) < 0.01 && std::fabs (b - 500) < 0.01 && std::fabs (c - 16000) < 0.01 && std::fabs (d - 1000) < 0.01, "cálculo de cutoff efetivo");
}

static double diffRatioRange (const std::vector<float>& x, int a, int b)
{ double d = 0, t = 0; for (int i = a; i < b; ++i) { d += std::pow (x[(size_t) i] - x[(size_t) i - 1], 2); t += std::pow (x[(size_t) i], 2); } return d / (t + 1e-20); }

static void testFilterInEngine()
{
    std::printf("[filtro no motor: corte e ENV2]\n");
    auto make = [] { EngineParams p; p.oscStart = 1; p.mixX = 0; p.mixY = 0; p.osc[0].wave = Wave::Saw; p.ampAttack = 0.f; p.ampSustain = 1.f; return p; };
    AudioEngine e1; e1.prepare (48000); auto p = make(); p.flt.on = false; e1.setParams (p); e1.noteOn (60, 1.f); auto open = render (e1, 24000);
    AudioEngine e2; e2.prepare (48000); p = make(); p.flt.f1Cut = 500; p.flt.f2Cut = 20000; p.flt.series = true; e2.setParams (p); e2.noteOn (60, 1.f); auto closed = render (e2, 24000);
    const double ro = diffRatioRange (open, 6000, 24000), rc = diffRatioRange (closed, 6000, 24000);
    std::printf("  brilho (energia da derivada): aberto %.4f | Lo 500 Hz %.5f\n", ro, rc);
    CHECK (rc < 0.05 * ro, "filtro passa-baixa não escureceu");
    AudioEngine e3; e3.prepare (48000); p = make(); p.flt.f1Cut = 300; p.flt.f2Cut = 300; p.flt.envAmt = 1.f; p.env2Attack = 0.f; p.env2Decay = 0.15f; p.env2Sustain = 0.f; e3.setParams (p); e3.noteOn (60, 1.f);
    auto sw = render (e3, 96000);
    const double early = diffRatioRange (sw, 100, 1500), late = diffRatioRange (sw, 60000, 96000);
    std::printf("  ENV2 -> cutoff: brilho inicial %.4f | brilho depois do decay %.5f\n", early, late);
    CHECK (early > 5.0 * late, "ENV2 deveria abrir o filtro no início e fechá-lo depois");
}

// ---------------- LFO ----------------
static void testLfo()
{
    std::printf("[LFO]\n");
    const double sr = 48000; Lfo l; l.setSampleRate (sr); l.setWave (LfoWave::Sine); l.setRate (5.0); l.retrigger (0.0);
    std::vector<float> x; for (int i = 0; i < (int) (10 * sr); ++i) x.push_back (l.next());
    int cross = 0; double mx = 0; for (size_t i = 1; i < x.size(); ++i) { if (x[i - 1] < 0 && x[i] >= 0) ++cross; mx = std::max (mx, (double) std::fabs (x[i])); }
    std::printf("  seno 5 Hz em 10 s: %d ciclos, pico %.3f\n", cross, mx);
    CHECK (std::abs (cross - 50) <= 1 && mx <= 1.0001, "taxa/amplitude do LFO");
    Lfo q; q.setSampleRate (sr); q.setWave (LfoWave::Square); q.setRate (3.0); q.retrigger();
    bool only = true; for (int i = 0; i < 48000; ++i) { const float v = q.next(); if (v != 1.f && v != -1.f) only = false; }
    CHECK (only, "onda quadrada só em ±1");
    Lfo h; h.setSampleRate (sr); h.setWave (LfoWave::SampleHold); h.setRate (10.0); h.retrigger(); int changes = 0; float prev = h.next();
    for (int i = 0; i < 48000; ++i) { const float v = h.next(); if (v != prev) ++changes; prev = v; }
    std::printf("  sample&hold 10 Hz: %d trocas em 1 s\n", changes);
    CHECK (std::abs (changes - 10) <= 1, "S&H deveria trocar uma vez por ciclo");
    Lfo f; f.setSampleRate (sr); f.setWave (LfoWave::SawUp); f.setRate (7.0); f.setFadeSeconds (0.5); f.retrigger();
    double early = 0, late = 0; for (int i = 0; i < (int) (0.8 * sr); ++i) { const double v = std::fabs (f.next()); if (i < 0.05 * sr) early = std::max (early, v); if (i > 0.6 * sr) late = std::max (late, v); }
    std::printf("  fade-in 0.5 s: pico em 50 ms %.2f | após 0.6 s %.2f\n", early, late);
    CHECK (early < 0.25 && late > 0.95, "fade-in do LFO");
}

static void testStress()
{
    std::printf("[estresse aleatório]\n");
    AudioEngine e; e.prepare (44100); EngineParams p; e.setParams (p);
    uint32_t s = 12345; auto rnd = [&] { s = s * 1664525u + 1013904223u; return (s >> 8) & 0xFFFF; };
    bool ok = true; double pk = 0;
    for (int it = 0; it < 4000; ++it)
    {
        const int k = rnd() % 10;
        if (k < 4) e.noteOn (rnd() % 128, (rnd() % 127 + 1) / 127.f);
        else if (k < 7) e.noteOff (rnd() % 128);
        else if (k == 7) e.sustainPedal (rnd() & 1);
        else if (k == 8) e.pitchBend ((rnd() % 2000) / 1000.f - 1.f);
        else { for (auto& o : p.osc) { o.wave = (Wave) (rnd() % 6); o.coarse = (int) (rnd() % 49) - 24; } p.mixX = (rnd() % 1000) / 1000.f; p.mixY = (rnd() % 1000) / 1000.f; p.voiceMode = rnd() % 3; p.maxVoices = 1 + rnd() % 16; e.setParams (p); }
        auto x = render (e, 1 + rnd() % 700);
        if (! finite (x)) { ok = false; }
        pk = std::max (pk, peak (x));
    }
    std::printf("  pico máximo observado: %.3f\n", pk);
    CHECK (ok, "NaN/inf encontrado");
    CHECK (pk < 4.0, "pico excessivo");
}

static void testRouter()
{
    std::printf("[router e modulação]\n");
    AudioEngine e; e.prepare (48000);
    EngineParams p = sineParams();
    p.modSlots[0].target = ModTarget::Pitch;
    p.modSlots[0].source = ModSource::LFO1;
    p.modSlots[0].via = ModSource::Off;
    p.modSlots[0].amount = 0.5f;
    e.setParams (p);
    e.noteOn (69, 1.0f);
    auto x = render (e, 4800);
    CHECK (finite (x), "router produziu nan/inf");
    CHECK (peak (x) > 0.01, "router sem sinal");
}

static void testEffects()
{
    std::printf("[efeitos: distorção e mod fx]\n");
    AudioEngine e; e.prepare (48000);
    EngineParams p = sineParams();
    p.effects.distType = DistType::Soft;
    p.effects.distDrive = 0.5f;
    p.effects.distTone = 0.5f;
    p.effects.modType = ModFxType::Chorus;
    p.effects.modIntensity = 0.5f;
    p.effects.modSpeed = 1.0f;
    e.setParams (p);
    e.noteOn (69, 1.0f);
    auto x = render (e, 4800);
    CHECK (finite (x), "efeitos produziram nan/inf");
    CHECK (peak (x) > 0.01, "efeitos sem sinal");
}

int main()
{
    testPitch(); testMix(); testEnvelopeAndRelease(); testPolyAndSteal();
    testSustain(); testMono(); testAliasing(); testSync(); testRing(); testFM();
    testPulseWidth(); testNoiseColor(); testOscStart(); testInstantEnvelope(); testNoiseDistribution();
    testFilterModes(); testFilter2Slopes(); testFilterRoutingAndBlend(); testFilterResonance(); testFilterStability();
    testEffectiveCutoff(); testFilterInEngine(); testLfo(); testStress();
    testRouter(); testEffects();
    std::printf("\n%d verificações, %d falhas\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
