#include "PluginProcessor.h"
#include "PluginEditor.h"

ES3Processor::ES3Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ES3_STATE", ES3::createLayout())
{
    auto raw = [this] (const juce::String& id) { auto* p = apvts.getRawParameterValue (id); jassert (p != nullptr); return p; };
    cache.tune = raw (ES3::ID::tune);       cache.bendRange = raw (ES3::ID::bendRange);
    cache.voices = raw (ES3::ID::voices);   cache.voiceMode = raw (ES3::ID::voiceMode);
    cache.mixX = raw (ES3::ID::mixX);       cache.mixY = raw (ES3::ID::mixY);
    cache.ampA = raw (ES3::ID::ampAttack);  cache.ampD = raw (ES3::ID::ampDecay);
    cache.ampS = raw (ES3::ID::ampSustain); cache.ampR = raw (ES3::ID::ampRelease);
    cache.volume = raw (ES3::ID::volume);
    cache.fm = raw (ES3::ID::osc (1, "fm"));  cache.color = raw (ES3::ID::osc (3, "color"));
    cache.oscStart = raw (ES3::ID::oscStart);
    cache.f1Type = raw (ES3::ID::flt (1, "type")); cache.f1Cut = raw (ES3::ID::flt (1, "cutoff")); cache.f1Res = raw (ES3::ID::flt (1, "res"));
    cache.f2Slope = raw (ES3::ID::flt (2, "slope")); cache.f2Cut = raw (ES3::ID::flt (2, "cutoff")); cache.f2Res = raw (ES3::ID::flt (2, "res"));
    cache.blend = raw (ES3::ID::fltBlend); cache.routing = raw (ES3::ID::fltRouting); cache.drive = raw (ES3::ID::fltDrive);
    cache.fltOn = raw (ES3::ID::fltOn); cache.fltReset = raw (ES3::ID::fltReset);
    cache.envAmt = raw (ES3::ID::fltEnvAmt); cache.key = raw (ES3::ID::fltKey);
    cache.e2a = raw (ES3::ID::env2Attack); cache.e2d = raw (ES3::ID::env2Decay); cache.e2s = raw (ES3::ID::env2Sustain); cache.e2r = raw (ES3::ID::env2Release);
    cache.e3a = raw (ES3::ID::env3Attack); cache.e3d = raw (ES3::ID::env3Decay); cache.e3s = raw (ES3::ID::env3Sustain); cache.e3r = raw (ES3::ID::env3Release);
    cache.aliasing = raw (ES3::ID::aliasing);
    cache.glide = raw (ES3::ID::glide);
    cache.sineLevel = raw (ES3::ID::sineLevel);
    cache.analog = raw (ES3::ID::analog);
    cache.unison = raw (ES3::ID::unison);
    cache.distType = raw (ES3::ID::distType);
    cache.distDrive = raw (ES3::ID::distDrive);
    cache.distTone = raw (ES3::ID::distTone);
    cache.modType = raw (ES3::ID::modType);
    cache.modIntensity = raw (ES3::ID::modIntensity);
    cache.modSpeed = raw (ES3::ID::modSpeed);
    cache.lfo1Wave = raw (ES3::ID::lfo1Wave); cache.lfo1Rate = raw (ES3::ID::lfo1Rate); cache.lfo1Fade = raw (ES3::ID::lfo1Fade);
    cache.lfo2Wave = raw (ES3::ID::lfo2Wave); cache.lfo2Rate = raw (ES3::ID::lfo2Rate); cache.lfo2Fade = raw (ES3::ID::lfo2Fade);
    cache.vectorEnabled = raw (ES3::ID::vectorEnabled);

    for (int i = 0; i < 3; ++i)
    {
        cache.wave[i]   = raw (ES3::ID::osc (i + 1, "wave"));
        cache.coarse[i] = raw (ES3::ID::osc (i + 1, "coarse"));
        cache.fine[i]   = raw (ES3::ID::osc (i + 1, "fine"));
        cache.pw[i]     = raw (ES3::ID::osc (i + 1, "pw"));
    }

    for (int i = 0; i < 10; ++i)
    {
        cache.modSlots[i].target = raw (ES3::ID::mod (i + 1, "target"));
        cache.modSlots[i].source = raw (ES3::ID::mod (i + 1, "source"));
        cache.modSlots[i].via    = raw (ES3::ID::mod (i + 1, "via"));
        cache.modSlots[i].amount = raw (ES3::ID::mod (i + 1, "amount"));
    }

    updateEngineParams();
}

void ES3Processor::prepareToPlay (double sampleRate, int)
{
    updateEngineParams();
    engine.setParams (engineParams);
    engine.prepare (sampleRate);
}

bool ES3Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ES3Processor::updateEngineParams()
{
    auto& p = engineParams;
    p.tuneCents = cache.tune->load();
    p.bendRange = cache.bendRange->load();
    p.maxVoices = (int) cache.voices->load();
    p.voiceMode = (int) cache.voiceMode->load();
    p.mixX = cache.mixX->load();   p.mixY = cache.mixY->load();
    p.ampAttack = cache.ampA->load(); p.ampDecay = cache.ampD->load();
    p.ampSustain = cache.ampS->load(); p.ampRelease = cache.ampR->load();
    p.volumeDb = cache.volume->load();
    p.oscStart = (int) cache.oscStart->load();
    p.env2Attack = cache.e2a->load(); p.env2Decay = cache.e2d->load(); p.env2Sustain = cache.e2s->load(); p.env2Release = cache.e2r->load();
    p.env3Attack = cache.e3a->load(); p.env3Decay = cache.e3d->load(); p.env3Sustain = cache.e3s->load(); p.env3Release = cache.e3r->load();
    p.aliasing = cache.aliasing->load();
    p.glideTime = cache.glide->load();
    p.sineLevel = cache.sineLevel->load();

    // LFOs
    p.lfo1.wave = (es3::LfoWave) (int) cache.lfo1Wave->load();
    p.lfo1.rate = cache.lfo1Rate->load();
    p.lfo1.fadeSeconds = cache.lfo1Fade->load();
    p.lfo2.wave = (es3::LfoWave) (int) cache.lfo2Wave->load();
    p.lfo2.rate = cache.lfo2Rate->load();
    p.lfo2.fadeSeconds = cache.lfo2Fade->load();

    // Filter
    auto& f = p.flt;
    f.on = cache.fltOn->load() > 0.5f; f.reset = cache.fltReset->load() > 0.5f; f.series = (int) cache.routing->load() == 1;
    f.f1Mode = (es3::F1Mode) (int) cache.f1Type->load(); f.f2Slope = (es3::F2Slope) (int) cache.f2Slope->load();
    f.f1Cut = cache.f1Cut->load(); f.f1Res = cache.f1Res->load(); f.f2Cut = cache.f2Cut->load(); f.f2Res = cache.f2Res->load();
    f.blend = cache.blend->load(); f.drive = cache.drive->load(); f.envAmt = cache.envAmt->load(); f.keyTrack = cache.key->load();

    // Oscillators
    p.osc[0].fm = cache.fm->load();
    p.osc[2].noiseColor = cache.color->load();
    for (int i = 0; i < 3; ++i)
    {
        p.osc[(size_t) i].wave   = (es3::Wave) (int) cache.wave[i]->load();
        p.osc[(size_t) i].coarse = (int) cache.coarse[i]->load();
        p.osc[(size_t) i].fine   = cache.fine[i]->load();
        p.osc[(size_t) i].pw     = cache.pw[i]->load();
    }

    // Router
    for (int i = 0; i < 10; ++i)
    {
        p.modSlots[(size_t) i].target = (es3::ModTarget) (int) cache.modSlots[i].target->load();
        p.modSlots[(size_t) i].source = (es3::ModSource) (int) cache.modSlots[i].source->load();
        p.modSlots[(size_t) i].via    = (es3::ModSource) (int) cache.modSlots[i].via->load();
        p.modSlots[(size_t) i].amount = cache.modSlots[i].amount->load();
    }

    // Vector
    p.vectorEnv.enabled = cache.vectorEnabled->load() > 0.5f;

    // Analog & Unison
    p.analog = cache.analog->load();
    p.unison = cache.unison->load() > 0.5f;

    // Efeitos (Parte 6)
    const int dt = (int) cache.distType->load();
    p.effects.distType = (cache.distDrive->load() <= 0.001f) ? es3::DistType::Off : (dt == 0 ? es3::DistType::Soft : es3::DistType::Hard);
    p.effects.distDrive = cache.distDrive->load();
    p.effects.distTone = cache.distTone->load();

    p.effects.modType = (es3::ModFxType) (int) cache.modType->load();
    p.effects.modIntensity = cache.modIntensity->load();
    p.effects.modSpeed = 0.05f * std::pow (300.0f, cache.modSpeed->load());
}

void ES3Processor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())            engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())      engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())   engine.pitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.f);
    else if (m.isAllNotesOff())  engine.allNotesOff();
    else if (m.isAllSoundOff())  engine.allSoundOff();
    else if (m.isAftertouch())   engine.setAftertouch ((float) m.getAfterTouchValue() / 127.f);
    else if (m.isChannelPressure()) engine.setAftertouch ((float) m.getChannelPressureValue() / 127.f);
    else if (m.isController())
    {
        const int cc = m.getControllerNumber(), v = m.getControllerValue();
        if (cc == 64) engine.sustainPedal (v >= 64);
        else if (cc == 1) engine.setModWheel ((float) v / 127.f);
    }
}

void ES3Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    if (buffer.getNumChannels() < 2) return;

    updateEngineParams();
    engine.setParams (engineParams);

    const int n = buffer.getNumSamples();
    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (pos, n, meta.samplePosition);
        if (t > pos) { engine.process (l + pos, r + pos, t - pos); pos = t; }
        handleMidi (meta.getMessage());
    }
    if (pos < n) engine.process (l + pos, r + pos, n - pos);
}

juce::AudioProcessorEditor* ES3Processor::createEditor() { return new ES3Editor (*this); }

void ES3Processor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void ES3Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ES3Processor(); }
