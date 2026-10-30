#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/ES3LookAndFeel.h"
#include "UI/TriangleMixComponent.h"
#include "UI/XYPadComponent.h"
#include "UI/RouterSlotComponent.h"
#include "Engine/PresetManager.h"

class ES3Editor : public juce::AudioProcessorEditor
{
public:
    explicit ES3Editor (ES3Processor&);
    ~ES3Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void initControl (juce::Slider& slider, const juce::String& paramId,
                      bool isRotary, bool isBipolar = false, double doubleClickDefault = 0.0);
    void initCombo (juce::ComboBox& box, const juce::String& paramId,
                    const juce::StringArray& items = {});
    void initButton (juce::Button& btn, const juce::String& paramId,
                     bool isToggle = true);

    ES3Processor& processor;
    es3::ui::ES3LookAndFeel customLookAndFeel;

    // --- TOP BAR ---
    juce::Label titleLabel;
    juce::ComboBox presetBox;
    juce::TextButton prevPresetBtn { "<" }, nextPresetBtn { ">" };
    juce::ToggleButton polyBtn { "Poly" }, monoBtn { "Mono" }, legatoBtn { "Legato" };
    juce::ToggleButton unisonBtn { "Unison" };
    juce::Slider voicesSlider;
    juce::ComboBox oscStartBox;
    juce::TextButton fltResetBtn { "Flt Reset" };

    // --- OSCILLATOR SECTION (LEFT) ---
    juce::Slider tuneSlider, analogSlider, glideSlider, bendRangeSlider;
    juce::Label tuneLabel, analogLabel, glideLabel, bendLabel;

    struct OscUI
    {
        juce::ComboBox waveBox;
        juce::Slider coarseSlider;
        juce::Slider fineSlider;
        juce::Slider pwSlider;
    };
    OscUI oscUI[3];
    juce::Slider osc1FmSlider;
    juce::Slider osc3ColorSlider;

    std::unique_ptr<es3::ui::TriangleMixComponent> triangleMix;

    // --- FILTER SECTION (CENTER) ---
    juce::Slider f1CutSlider, f1ResSlider, f1DriveSlider;
    juce::ComboBox f1ModeBox;
    juce::Slider f2CutSlider, f2ResSlider;
    juce::ComboBox f2SlopeBox;
    juce::Slider blendSlider, envAmtSlider, keyTrackSlider;
    juce::ToggleButton routingBtn { "Series" };
    juce::ToggleButton fltOnBtn { "Filter On" };

    // --- EFFECTS & OUTPUT (RIGHT) ---
    juce::Slider volumeSlider, sineLevelSlider;
    juce::Slider distDriveSlider, distToneSlider;
    juce::ComboBox distTypeBox;
    juce::ComboBox modTypeBox;
    juce::Slider modIntensitySlider, modSpeedSlider;

    std::unique_ptr<es3::ui::XYPadComponent> xyPad;

    // --- ROUTER (MIDDLE STRIP) ---
    std::unique_ptr<es3::ui::RouterSlotComponent> routerSlots[10];
    juce::TextButton routerTabBtn { "Router" }, vectorTabBtn { "Vector" };
    juce::ToggleButton vectorEnabledBtn { "Vector On" };

    // --- LFOs & ENVELOPES (BOTTOM) ---
    juce::ComboBox lfo1WaveBox, lfo2WaveBox;
    juce::Slider lfo1RateSlider, lfo1FadeSlider;
    juce::Slider lfo2RateSlider, lfo2FadeSlider;

    // Envelopes: ENV1 (Flt), ENV2 (Free Mod), ENV3 (Amp)
    juce::Slider e2AttackSlider, e2DecaySlider, e2SustainSlider, e2ReleaseSlider;
    juce::Slider e3AttackSlider, e3DecaySlider, e3SustainSlider, e3ReleaseSlider;
    juce::Slider ampAttackSlider, ampDecaySlider, ampSustainSlider, ampReleaseSlider;

    // Attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttach;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttach;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttach;

    std::vector<es3::Preset> factoryPresets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ES3Editor)
};
