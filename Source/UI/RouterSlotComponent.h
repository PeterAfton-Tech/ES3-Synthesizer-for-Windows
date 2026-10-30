#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ES3LookAndFeel.h"

namespace es3::ui
{

class RouterSlotComponent : public juce::Component
{
public:
    RouterSlotComponent (juce::AudioProcessorValueTreeState& apvts, int slotIndex)
        : slotNum (slotIndex)
    {
        const juce::String s (slotNum);

        static const juce::StringArray sourceNames { "Off", "ENV1", "ENV2", "ENV3", "LFO1", "LFO2",
                                                     "Velocity", "Key", "ModWheel", "Aftertouch", "PitchBend", "Random" };
        static const juce::StringArray targetNames { "Off", "Pitch", "Osc1 Pitch", "Osc2 Pitch", "Osc3 Pitch",
                                                     "Cutoff 1", "Cutoff 2", "Cutoff 1+2",
                                                     "Reso 1", "Reso 2", "PW1", "PW2", "PW3",
                                                     "FM", "Volume", "Pan", "Mix X", "Mix Y",
                                                     "LFO1 Rate", "LFO2 Rate", "Drive" };

        targetBox.addItemList (targetNames, 1);
        viaBox.addItemList (sourceNames, 1);
        sourceBox.addItemList (sourceNames, 1);

        // Target ComboBox
        addAndMakeVisible (targetBox);
        targetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "mod" + s + "_target", targetBox);

        // Via ComboBox
        addAndMakeVisible (viaBox);
        viaAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "mod" + s + "_via", viaBox);

        // Source ComboBox
        addAndMakeVisible (sourceBox);
        sourceAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "mod" + s + "_source", sourceBox);

        // Amount Slider (bipolar vertical)
        amountSlider.setSliderStyle (juce::Slider::LinearVertical);
        amountSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        amountSlider.getProperties().set ("bipolar", true);
        amountSlider.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (amountSlider);
        amountAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, "mod" + s + "_amount", amountSlider);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Fundo do slot LCD azul/ciano escuro
        g.setColour (Palette::lcdSlot);
        g.fillRoundedRectangle (bounds.reduced (1.0f), 2.0f);

        // Linha divisória lateral direita
        g.setColour (Palette::cyanDim.withAlpha (0.25f));
        g.drawVerticalLine ((int) bounds.getRight() - 1, bounds.getY() + 2.0f, bounds.getBottom() - 2.0f);

        // Header do slot (número)
        g.setFont (juce::Font (9.0f, juce::Font::bold));
        g.setColour (Palette::cyanDim);
        g.drawText (juce::String (slotNum), 2, 2, (int) bounds.getWidth() - 4, 10, juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (2, 3);
        area.removeFromTop (10); // espaço para o número do slot

        targetBox.setBounds (area.removeFromTop (18));
        area.removeFromTop (2);
        viaBox.setBounds (area.removeFromTop (18));
        area.removeFromTop (2);

        sourceBox.setBounds (area.removeFromBottom (18));
        area.removeFromBottom (2);

        amountSlider.setBounds (area);
    }

private:
    int slotNum = 1;
    juce::ComboBox targetBox;
    juce::ComboBox viaBox;
    juce::ComboBox sourceBox;
    juce::Slider amountSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> targetAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> viaAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttach;
};

} // namespace es3::ui
