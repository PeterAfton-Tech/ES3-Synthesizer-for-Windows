#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <string>

namespace es3
{

struct Preset
{
    juce::String name;
    juce::String category;
    std::vector<std::pair<juce::String, float>> paramValues;
};

class PresetManager
{
public:
    static std::vector<Preset> getFactoryPresets()
    {
        std::vector<Preset> presets;

        // 1. User Default (Inspirado no ES2-Default-Patch.jpg)
        {
            Preset p;
            p.name = "User Default";
            p.category = "Init";
            p.paramValues = {
                { "out_volume", 0.0f },
                { "global_voices", 8.0f },
                { "global_voice_mode", 0.0f }, // Poly
                { "osc1_wave", 2.0f },         // Saw
                { "osc2_wave", 3.0f },         // Square
                { "osc3_wave", 4.0f },         // Noise
                { "osc_mix_x", 0.0f },         // Osc 1 100%
                { "osc_mix_y", 0.0f },
                { "flt_on", 1.0f },
                { "flt1_cutoff", 20000.0f },
                { "flt2_cutoff", 20000.0f },
                { "flt_routing", 1.0f },       // Series
                { "amp_attack", 0.005f },
                { "amp_decay", 0.3f },
                { "amp_sustain", 0.8f },
                { "amp_release", 0.2f },
                { "env2_attack", 0.005f },
                { "env2_decay", 0.3f },
                { "env2_sustain", 0.0f },
                { "env2_release", 0.2f },
                { "fx_dist_drive", 0.0f },
                { "fx_mod_type", 0.0f }
            };
            presets.push_back (p);
        }

        // 2. Fat ES2 Bass
        {
            Preset p;
            p.name = "Fat ES2 Bass";
            p.category = "Bass";
            p.paramValues = {
                { "global_voice_mode", 1.0f }, // Mono
                { "global_glide", 0.08f },
                { "osc1_wave", 2.0f },         // Saw
                { "osc1_coarse", -12.0f },
                { "osc2_wave", 3.0f },         // Square
                { "osc2_coarse", -12.0f },
                { "osc2_fine", 6.0f },
                { "osc3_wave", 0.0f },         // Sine sub
                { "osc3_coarse", -24.0f },
                { "osc_mix_x", 0.33f },
                { "osc_mix_y", 0.45f },
                { "flt_on", 1.0f },
                { "flt1_type", 0.0f },         // Lo
                { "flt1_cutoff", 1800.0f },
                { "flt1_res", 0.25f },
                { "flt2_slope", 3.0f },        // Fat mode!
                { "flt2_cutoff", 1200.0f },
                { "flt2_res", 0.45f },
                { "flt_env_amt", 0.65f },
                { "env2_attack", 0.002f },
                { "env2_decay", 0.22f },
                { "env2_sustain", 0.15f },
                { "env2_release", 0.10f },
                { "amp_attack", 0.001f },
                { "amp_decay", 0.35f },
                { "amp_sustain", 0.70f },
                { "amp_release", 0.12f },
                { "fx_dist_drive", 0.35f },
                { "fx_dist_type", 0.0f },       // Soft
                { "fx_dist_tone", 0.45f },
                { "out_sine_level", 0.40f }
            };
            presets.push_back (p);
        }

        // 3. Logic Classic Saw Lead
        {
            Preset p;
            p.name = "Logic Classic Saw Lead";
            p.category = "Lead";
            p.paramValues = {
                { "global_voice_mode", 2.0f }, // Legato
                { "global_glide", 0.06f },
                { "global_analog", 0.30f },
                { "osc1_wave", 2.0f },         // Saw
                { "osc2_wave", 2.0f },         // Saw
                { "osc2_fine", 9.0f },
                { "osc3_wave", 7.0f },         // Sync Saw
                { "osc3_fine", -9.0f },
                { "osc_mix_x", 0.33f },
                { "osc_mix_y", 0.33f },
                { "flt_on", 1.0f },
                { "flt1_cutoff", 5500.0f },
                { "flt1_res", 0.30f },
                { "flt2_cutoff", 7500.0f },
                { "flt_env_amt", 0.40f },
                { "env2_attack", 0.005f },
                { "env2_decay", 0.40f },
                { "amp_attack", 0.005f },
                { "amp_decay", 0.50f },
                { "amp_sustain", 0.85f },
                { "amp_release", 0.35f },
                { "fx_dist_drive", 0.20f },
                { "fx_mod_type", 1.0f },       // Chorus
                { "fx_mod_intensity", 0.45f },
                { "fx_mod_speed", 0.35f }
            };
            presets.push_back (p);
        }

        // 4. Vintage 80s Poly Brass
        {
            Preset p;
            p.name = "Vintage 80s Poly Brass";
            p.category = "Poly";
            p.paramValues = {
                { "global_voice_mode", 0.0f }, // Poly
                { "global_voices", 8.0f },
                { "global_analog", 0.45f },
                { "osc1_wave", 2.0f },         // Saw
                { "osc2_wave", 2.0f },         // Saw
                { "osc2_fine", 8.0f },
                { "osc3_wave", 3.0f },         // Square
                { "osc3_pw", 0.35f },
                { "osc_mix_x", 0.25f },
                { "osc_mix_y", 0.25f },
                { "flt_on", 1.0f },
                { "flt1_cutoff", 3200.0f },
                { "flt1_res", 0.20f },
                { "flt2_cutoff", 2800.0f },
                { "flt_env_amt", 0.70f },
                { "env2_attack", 0.065f },
                { "env2_decay", 0.60f },
                { "env2_sustain", 0.45f },
                { "env2_release", 0.35f },
                { "amp_attack", 0.035f },
                { "amp_decay", 0.70f },
                { "amp_sustain", 0.75f },
                { "amp_release", 0.35f },
                { "fx_mod_type", 1.0f },       // Chorus
                { "fx_mod_intensity", 0.50f },
                { "fx_mod_speed", 0.25f }
            };
            presets.push_back (p);
        }

        // 5. Morphing Vector Pad
        {
            Preset p;
            p.name = "Morphing Vector Pad";
            p.category = "Pad";
            p.paramValues = {
                { "global_voice_mode", 0.0f },
                { "global_voices", 8.0f },
                { "global_analog", 0.25f },
                { "vec_enabled", 1.0f },
                { "osc1_wave", 1.0f },         // Triangle
                { "osc2_wave", 2.0f },         // Saw
                { "osc2_fine", 7.0f },
                { "osc3_wave", 8.0f },         // Ring
                { "osc_mix_x", 0.40f },
                { "osc_mix_y", 0.40f },
                { "flt_on", 1.0f },
                { "flt1_cutoff", 4200.0f },
                { "flt1_res", 0.35f },
                { "flt2_cutoff", 3500.0f },
                { "amp_attack", 0.45f },
                { "amp_decay", 1.20f },
                { "amp_sustain", 0.85f },
                { "amp_release", 0.95f },
                { "fx_mod_type", 3.0f },       // Phaser
                { "fx_mod_intensity", 0.55f },
                { "fx_mod_speed", 0.20f }
            };
            presets.push_back (p);
        }

        // 6. Resonant FM Pluck
        {
            Preset p;
            p.name = "Resonant FM Pluck";
            p.category = "Pluck";
            p.paramValues = {
                { "global_voice_mode", 0.0f },
                { "osc1_wave", 0.0f },         // Sine
                { "osc1_fm", 0.45f },          // FM by Osc 2
                { "osc2_wave", 0.0f },         // Sine
                { "osc2_coarse", 12.0f },
                { "osc3_wave", 4.0f },         // Noise
                { "osc_mix_x", 0.15f },
                { "osc_mix_y", 0.15f },
                { "flt_on", 1.0f },
                { "flt1_cutoff", 6000.0f },
                { "flt1_res", 0.50f },
                { "flt_env_amt", 0.75f },
                { "env2_attack", 0.001f },
                { "env2_decay", 0.18f },
                { "env2_sustain", 0.0f },
                { "env2_release", 0.15f },
                { "amp_attack", 0.001f },
                { "amp_decay", 0.28f },
                { "amp_sustain", 0.0f },
                { "amp_release", 0.20f },
                { "fx_dist_drive", 0.15f },
                { "fx_mod_type", 1.0f },       // Chorus
                { "fx_mod_intensity", 0.30f }
            };
            presets.push_back (p);
        }

        return presets;
    }

    static void applyPreset (const Preset& p, juce::AudioProcessorValueTreeState& apvts)
    {
        for (const auto& [paramId, value] : p.paramValues)
        {
            if (auto* param = apvts.getParameter (paramId))
            {
                param->setValueNotifyingHost (param->convertTo0to1 (value));
            }
        }
    }
};

} // namespace es3
