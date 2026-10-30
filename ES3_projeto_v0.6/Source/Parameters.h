#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// IDs de parâmetros ESTÁVEIS: nunca renomeie depois de publicar (automação do FL Studio depende deles).
// Faixas e curvas abaixo são PROVISÓRIAS (não verificadas contra o ES2 real).
namespace ES3::ID
{
    inline constexpr auto tune = "global_tune", analog = "global_analog", glide = "global_glide",
                          bendRange = "global_bend_range", voices = "global_voices",
                          voiceMode = "global_voice_mode", unison = "global_unison",
                          volume = "out_volume", sineLevel = "out_sine_level",
                          oscStart = "osc_start", ampAttack = "amp_attack", ampDecay = "amp_decay", ampSustain = "amp_sustain", ampRelease = "amp_release",
                          mixX = "osc_mix_x", mixY = "osc_mix_y",
                          fltBlend = "flt_blend", fltRouting = "flt_routing",
                          fltDrive = "flt_drive", fltOn = "flt_on", fltReset = "flt_reset",
                          fltEnvAmt = "flt_env_amt", fltKey = "flt_key_track",
                          env2Attack = "env2_attack", env2Decay = "env2_decay", env2Sustain = "env2_sustain", env2Release = "env2_release",
                          env3Attack = "env3_attack", env3Decay = "env3_decay", env3Sustain = "env3_sustain", env3Release = "env3_release",
                          distType = "fx_dist_type", distDrive = "fx_dist_drive", distTone = "fx_dist_tone",
                          modType = "fx_mod_type", modIntensity = "fx_mod_intensity", modSpeed = "fx_mod_speed",
                          aliasing = "adv_aliasing",
                          lfo1Wave = "lfo1_wave", lfo1Rate = "lfo1_rate", lfo1Fade = "lfo1_fade",
                          lfo2Wave = "lfo2_wave", lfo2Rate = "lfo2_rate", lfo2Fade = "lfo2_fade",
                          vectorEnabled = "vec_enabled";

    inline juce::String osc (int n, const char* p) { return "osc" + juce::String (n) + "_" + p; }
    inline juce::String flt (int n, const char* p) { return "flt" + juce::String (n) + "_" + p; }
    inline juce::String mod (int slot, const char* p) { return "mod" + juce::String (slot) + "_" + p; }
}

namespace ES3
{
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    using F = AudioParameterFloat; using C = AudioParameterChoice; using I = AudioParameterInt;
    auto pid = [] (const String& s) { return ParameterID { s, 1 }; };
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<F> (pid (ID::tune), "Tune", NormalisableRange<float> (-100.f, 100.f, 0.01f), 0.f));
    p.push_back (std::make_unique<F> (pid (ID::analog), "Analog", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<F> (pid (ID::glide), "Glide", NormalisableRange<float> (0.f, 5.f, 0.001f, 0.3f), 0.f));
    p.push_back (std::make_unique<I> (pid (ID::bendRange), "Bend Range", 0, 24, 2));
    p.push_back (std::make_unique<I> (pid (ID::voices), "Voices", 1, 16, 8));
    p.push_back (std::make_unique<C> (pid (ID::voiceMode), "Voice Mode", StringArray { "Poly", "Mono", "Legato" }, 0));
    p.push_back (std::make_unique<AudioParameterBool> (pid (ID::unison), "Unison", false));

    for (int n = 1; n <= 3; ++n)
    {
        const String s (n);
        p.push_back (std::make_unique<C> (pid (ID::osc (n, "wave")), "Osc " + s + " Wave",
                     StringArray { "Sine", "Triangle", "Saw", "Square", "Noise", "Digital Wave", "Sync Square", "Sync Saw", "Ring" }, 2));
        p.push_back (std::make_unique<I> (pid (ID::osc (n, "coarse")), "Osc " + s + " Coarse", -36, 36, 0));
        p.push_back (std::make_unique<F> (pid (ID::osc (n, "fine")), "Osc " + s + " Fine", NormalisableRange<float> (-100.f, 100.f, 0.01f), 0.f));
        p.push_back (std::make_unique<F> (pid (ID::osc (n, "pw")), "Osc " + s + " Pulse Width", NormalisableRange<float> (0.f, 1.f), 0.f));
    }

    p.push_back (std::make_unique<F> (pid (ID::osc (1, "fm")), "Osc 1 FM", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<F> (pid (ID::osc (3, "color")), "Osc 3 Noise Color", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<C> (pid (ID::oscStart), "Osc Start", StringArray { "Free", "Soft", "Hard" }, 0));
    p.push_back (std::make_unique<F> (pid (ID::mixX), "Mix X", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<F> (pid (ID::mixY), "Mix Y", NormalisableRange<float> (0.f, 1.f), 0.f));

    // Filtro 1: multimodo 12 dB/oit. Filtro 2: passa-baixa 12/18/24 dB e Fat.
    p.push_back (std::make_unique<C> (pid (ID::flt (1, "type")), "Filter 1 Mode", StringArray { "Lo", "Hi", "Peak", "BP", "BR" }, 0));
    p.push_back (std::make_unique<F> (pid (ID::flt (1, "cutoff")), "Filter 1 Cutoff", NormalisableRange<float> (20.f, 20000.f, 0.f, 0.3f), 20000.f));
    p.push_back (std::make_unique<F> (pid (ID::flt (1, "res")), "Filter 1 Resonance", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<C> (pid (ID::flt (2, "slope")), "Filter 2 Slope", StringArray { "12 dB", "18 dB", "24 dB", "Fat" }, 2));
    p.push_back (std::make_unique<F> (pid (ID::flt (2, "cutoff")), "Filter 2 Cutoff", NormalisableRange<float> (20.f, 20000.f, 0.f, 0.3f), 20000.f));
    p.push_back (std::make_unique<F> (pid (ID::flt (2, "res")), "Filter 2 Resonance", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<F> (pid (ID::fltBlend), "Filter Blend", NormalisableRange<float> (-1.f, 1.f), 0.f));
    p.push_back (std::make_unique<C> (pid (ID::fltRouting), "Filter Routing", StringArray { "Parallel", "Series" }, 1));
    p.push_back (std::make_unique<F> (pid (ID::fltDrive), "Filter Drive", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<AudioParameterBool> (pid (ID::fltOn), "Filter On", true));
    p.push_back (std::make_unique<AudioParameterBool> (pid (ID::fltReset), "Flt Reset", false));
    p.push_back (std::make_unique<F> (pid (ID::fltEnvAmt), "ENV2 to Cutoff", NormalisableRange<float> (-1.f, 1.f), 0.f));
    p.push_back (std::make_unique<F> (pid (ID::fltKey), "Key to Cutoff", NormalisableRange<float> (0.f, 1.f), 0.f));

    // ENV2
    p.push_back (std::make_unique<F> (pid (ID::env2Attack),  "Env 2 Attack",  NormalisableRange<float> (0.f, 10.f, 0.f, 0.3f), 0.005f));
    p.push_back (std::make_unique<F> (pid (ID::env2Decay),   "Env 2 Decay",   NormalisableRange<float> (0.001f, 10.f, 0.f, 0.3f), 0.3f));
    p.push_back (std::make_unique<F> (pid (ID::env2Sustain), "Env 2 Sustain", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<F> (pid (ID::env2Release), "Env 2 Release", NormalisableRange<float> (0.f, 10.f, 0.f, 0.3f), 0.2f));

    // ENV3
    p.push_back (std::make_unique<F> (pid (ID::env3Attack),  "Env 3 Attack",  NormalisableRange<float> (0.f, 10.f, 0.f, 0.3f), 0.005f));
    p.push_back (std::make_unique<F> (pid (ID::env3Decay),   "Env 3 Decay",   NormalisableRange<float> (0.001f, 10.f, 0.f, 0.3f), 0.3f));
    p.push_back (std::make_unique<F> (pid (ID::env3Sustain), "Env 3 Sustain", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<F> (pid (ID::env3Release), "Env 3 Release", NormalisableRange<float> (0.f, 10.f, 0.f, 0.3f), 0.2f));

    // Efeitos (ainda placeholders no áudio)
    p.push_back (std::make_unique<C> (pid (ID::distType), "Distortion Type", StringArray { "Soft", "Hard" }, 0));
    p.push_back (std::make_unique<F> (pid (ID::distDrive), "Distortion", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<F> (pid (ID::distTone), "Tone", NormalisableRange<float> (0.f, 1.f), 0.5f));
    p.push_back (std::make_unique<C> (pid (ID::modType), "Mod FX Type", StringArray { "Off", "Chorus", "Flanger", "Phaser" }, 0));
    p.push_back (std::make_unique<F> (pid (ID::modIntensity), "Mod FX Intensity", NormalisableRange<float> (0.f, 1.f), 0.5f));
    p.push_back (std::make_unique<F> (pid (ID::modSpeed), "Mod FX Speed", NormalisableRange<float> (0.f, 1.f), 0.3f));
    p.push_back (std::make_unique<F> (pid (ID::sineLevel), "Sine Level", NormalisableRange<float> (0.f, 1.f), 0.f));
    p.push_back (std::make_unique<F> (pid (ID::volume), "Volume", NormalisableRange<float> (-48.f, 6.f, 0.01f), 0.f));

    // Amp ADSR
    p.push_back (std::make_unique<F> (pid (ID::ampAttack),  "Amp Attack",  NormalisableRange<float> (0.f, 10.f, 0.f, 0.3f), 0.005f));
    p.push_back (std::make_unique<F> (pid (ID::ampDecay),   "Amp Decay",   NormalisableRange<float> (0.001f, 10.f, 0.f, 0.3f), 0.3f));
    p.push_back (std::make_unique<F> (pid (ID::ampSustain), "Amp Sustain", NormalisableRange<float> (0.f, 1.f), 0.8f));
    p.push_back (std::make_unique<F> (pid (ID::ampRelease), "Amp Release", NormalisableRange<float> (0.f, 10.f, 0.f, 0.3f), 0.2f));

    // Aliasing (configuração avançada — não é controle principal)
    p.push_back (std::make_unique<F> (pid (ID::aliasing), "Aliasing", NormalisableRange<float> (0.f, 1.f), es3::kDefaultAliasing));

    // LFO 1 (polifônico)
    p.push_back (std::make_unique<C> (pid (ID::lfo1Wave), "LFO 1 Wave",
                 StringArray { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Smooth" }, 0));
    p.push_back (std::make_unique<F> (pid (ID::lfo1Rate), "LFO 1 Rate", NormalisableRange<float> (0.05f, 50.f, 0.f, 0.3f), 5.f));
    p.push_back (std::make_unique<F> (pid (ID::lfo1Fade), "LFO 1 Fade", NormalisableRange<float> (0.f, 10.f, 0.f, 0.3f), 0.f));

    // LFO 2 (monofônico)
    p.push_back (std::make_unique<C> (pid (ID::lfo2Wave), "LFO 2 Wave",
                 StringArray { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Smooth" }, 0));
    p.push_back (std::make_unique<F> (pid (ID::lfo2Rate), "LFO 2 Rate", NormalisableRange<float> (0.05f, 50.f, 0.f, 0.3f), 5.f));
    p.push_back (std::make_unique<F> (pid (ID::lfo2Fade), "LFO 2 Fade", NormalisableRange<float> (0.f, 10.f, 0.f, 0.3f), 0.f));

    // Router: 10 slots de modulação (target, source, via, amount)
    StringArray sourceNames { "Off", "ENV1", "ENV2", "ENV3", "LFO1", "LFO2",
                              "Velocity", "Key", "ModWheel", "Aftertouch", "PitchBend", "Random" };
    StringArray targetNames { "Off", "Pitch", "Osc1 Pitch", "Osc2 Pitch", "Osc3 Pitch",
                              "Cutoff 1", "Cutoff 2", "Cutoff 1+2",
                              "Reso 1", "Reso 2", "PW1", "PW2", "PW3",
                              "FM", "Volume", "Pan", "Mix X", "Mix Y",
                              "LFO1 Rate", "LFO2 Rate", "Drive" };
    for (int i = 1; i <= 10; ++i)
    {
        const String si (i);
        p.push_back (std::make_unique<C> (pid (ID::mod (i, "target")), "Mod " + si + " Target", targetNames, 0));
        p.push_back (std::make_unique<C> (pid (ID::mod (i, "source")), "Mod " + si + " Source", sourceNames, 0));
        p.push_back (std::make_unique<C> (pid (ID::mod (i, "via")),    "Mod " + si + " Via",    sourceNames, 0));
        p.push_back (std::make_unique<F> (pid (ID::mod (i, "amount")), "Mod " + si + " Amount", NormalisableRange<float> (-1.f, 1.f), 0.f));
    }

    // Vector envelope enabled
    p.push_back (std::make_unique<AudioParameterBool> (pid (ID::vectorEnabled), "Vector Enabled", false));

    return { p.begin(), p.end() };
}
}
