#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "Engine/AudioEngine.h"

class ES3Processor : public juce::AudioProcessor
{
public:
    ES3Processor();
    ~ES3Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ES3"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    void updateEngineParams();
    void handleMidi (const juce::MidiMessage&);

    struct Cache
    {
        std::atomic<float>* tune {}; std::atomic<float>* bendRange {}; std::atomic<float>* voices {};
        std::atomic<float>* voiceMode {}; std::atomic<float>* mixX {}; std::atomic<float>* mixY {};
        std::atomic<float>* wave[3] {}; std::atomic<float>* pw[3] {}; std::atomic<float>* fm {}; std::atomic<float>* color {}; std::atomic<float>* oscStart {}; std::atomic<float>* coarse[3] {}; std::atomic<float>* fine[3] {};
        std::atomic<float>* ampA {}; std::atomic<float>* ampD {}; std::atomic<float>* ampS {}; std::atomic<float>* ampR {};
        std::atomic<float>* volume {};
        std::atomic<float>* f1Type {}; std::atomic<float>* f1Cut {}; std::atomic<float>* f1Res {};
        std::atomic<float>* f2Slope {}; std::atomic<float>* f2Cut {}; std::atomic<float>* f2Res {};
        std::atomic<float>* blend {}; std::atomic<float>* routing {}; std::atomic<float>* drive {};
        std::atomic<float>* fltOn {}; std::atomic<float>* fltReset {}; std::atomic<float>* envAmt {}; std::atomic<float>* key {};
        std::atomic<float>* e2a {}; std::atomic<float>* e2d {}; std::atomic<float>* e2s {}; std::atomic<float>* e2r {};
        std::atomic<float>* e3a {}; std::atomic<float>* e3d {}; std::atomic<float>* e3s {}; std::atomic<float>* e3r {};
        std::atomic<float>* aliasing {};
        std::atomic<float>* glide {};
        std::atomic<float>* sineLevel {};
        std::atomic<float>* lfo1Wave {}; std::atomic<float>* lfo1Rate {}; std::atomic<float>* lfo1Fade {};
        std::atomic<float>* lfo2Wave {}; std::atomic<float>* lfo2Rate {}; std::atomic<float>* lfo2Fade {};
        std::atomic<float>* analog {};
        std::atomic<float>* unison {};
        // Effects
        std::atomic<float>* distType {};
        std::atomic<float>* distDrive {};
        std::atomic<float>* distTone {};
        std::atomic<float>* modType {};
        std::atomic<float>* modIntensity {};
        std::atomic<float>* modSpeed {};
        // Router slots
        struct ModSlotCache { std::atomic<float>* target {}; std::atomic<float>* source {}; std::atomic<float>* via {}; std::atomic<float>* amount {}; };
        ModSlotCache modSlots[10];
        std::atomic<float>* vectorEnabled {};
    } cache;

    es3::AudioEngine engine;
    es3::EngineParams engineParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ES3Processor)
};
