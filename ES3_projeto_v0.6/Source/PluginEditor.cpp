#include "PluginEditor.h"

using namespace es3::ui;

ES3Editor::ES3Editor (ES3Processor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&customLookAndFeel);

    // --- DIMENSÕES E RESIZE (UX FL STUDIO) ---
    setSize (1024, 680);
    setResizable (true, true);
    setResizeLimits (800, 530, 1600, 1060);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio (1024.0 / 680.0);

    // --- 1. BARRA SUPERIOR (HEADER & PRESETS) ---
    titleLabel.setText ("ES3", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (18.0f).withStyle ("Bold")));
    titleLabel.setColour (juce::Label::textColourId, Palette::textLight);
    addAndMakeVisible (titleLabel);

    // Presets
    factoryPresets = es3::PresetManager::getFactoryPresets();
    for (int i = 0; i < (int) factoryPresets.size(); ++i)
        presetBox.addItem (factoryPresets[i].name, i + 1);
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.onChange = [this]()
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx < (int) factoryPresets.size())
            es3::PresetManager::applyPreset (factoryPresets[idx], processor.apvts);
    };
    addAndMakeVisible (presetBox);

    prevPresetBtn.onClick = [this]()
    {
        int cur = presetBox.getSelectedId();
        if (cur > 1) presetBox.setSelectedId (cur - 1);
        else presetBox.setSelectedId (presetBox.getNumItems());
    };
    addAndMakeVisible (prevPresetBtn);

    nextPresetBtn.onClick = [this]()
    {
        int cur = presetBox.getSelectedId();
        if (cur < presetBox.getNumItems()) presetBox.setSelectedId (cur + 1);
        else presetBox.setSelectedId (1);
    };
    addAndMakeVisible (nextPresetBtn);

    // Modos de Voz (Radio buttons)
    polyBtn.setRadioGroupId (101); monoBtn.setRadioGroupId (101); legatoBtn.setRadioGroupId (101);
    addAndMakeVisible (polyBtn); addAndMakeVisible (monoBtn); addAndMakeVisible (legatoBtn);

    // Conecta modos de voz ao parâmetro global_voice_mode
    if (auto* vModeParam = processor.apvts.getParameter ("global_voice_mode"))
    {
        const int m = (int) vModeParam->getValue();
        if (m == 0) polyBtn.setToggleState (true, juce::dontSendNotification);
        else if (m == 1) monoBtn.setToggleState (true, juce::dontSendNotification);
        else legatoBtn.setToggleState (true, juce::dontSendNotification);

        polyBtn.onClick = [vModeParam]() { vModeParam->setValueNotifyingHost (0.0f); };
        monoBtn.onClick = [vModeParam]() { vModeParam->setValueNotifyingHost (0.5f); };
        legatoBtn.onClick = [vModeParam]() { vModeParam->setValueNotifyingHost (1.0f); };
    }

    initButton (unisonBtn, "global_unison");
    initControl (voicesSlider, "global_voices", false, false, 8.0);
    voicesSlider.setSliderStyle (juce::Slider::IncDecButtons);
    voicesSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 36, 18);

    initCombo (oscStartBox, "osc_start", { "Free", "Soft", "Hard" });

    fltResetBtn.onClick = [this]()
    {
        if (auto* p = processor.apvts.getParameter ("flt_reset"))
            p->setValueNotifyingHost (1.0f);
    };
    addAndMakeVisible (fltResetBtn);

    // --- 2. PAINEL ESQUERDO: MASTER PITCH & OSCILADORES ---
    initControl (tuneSlider, "global_tune", true, true, 0.0);
    initControl (analogSlider, "global_analog", true, false, 0.0);
    initControl (glideSlider, "global_glide", true, false, 0.0);
    initControl (bendRangeSlider, "global_bend_range", true, false, 2.0);

    juce::StringArray waves { "Sine", "Tri", "Saw", "Square", "Noise", "Digital", "Sync Sq", "Sync Saw", "Ring" };
    for (int i = 0; i < 3; ++i)
    {
        const juce::String s (i + 1);
        initCombo (oscUI[i].waveBox, "osc" + s + "_wave", waves);
        initControl (oscUI[i].coarseSlider, "osc" + s + "_coarse", true, true, 0.0);
        initControl (oscUI[i].fineSlider, "osc" + s + "_fine", true, true, 0.0);
        initControl (oscUI[i].pwSlider, "osc" + s + "_pw", true, false, 0.0);
    }
    initControl (osc1FmSlider, "osc1_fm", true, false, 0.0);
    initControl (osc3ColorSlider, "osc3_color", true, false, 0.0);

    // Triângulo de Mix interativo
    auto* pMixX = processor.apvts.getParameter ("osc_mix_x");
    auto* pMixY = processor.apvts.getParameter ("osc_mix_y");
    triangleMix = std::make_unique<TriangleMixComponent> (pMixX, pMixY);
    addAndMakeVisible (*triangleMix);

    // --- 3. PAINEL CENTRAL: FILTROS DUPLOS ---
    initCombo (f1ModeBox, "flt1_type", { "Lo", "Hi", "Peak", "BP", "BR" });
    initControl (f1CutSlider, "flt1_cutoff", true, false, 20000.0);
    initControl (f1ResSlider, "flt1_res", true, false, 0.0);
    initControl (f1DriveSlider, "flt_drive", true, false, 0.0);

    initCombo (f2SlopeBox, "flt2_slope", { "12 dB", "18 dB", "24 dB", "Fat" });
    initControl (f2CutSlider, "flt2_cutoff", true, false, 20000.0);
    initControl (f2ResSlider, "flt2_res", true, false, 0.0);

    initControl (blendSlider, "flt_blend", true, true, 0.0);
    initButton (routingBtn, "flt_routing");
    initButton (fltOnBtn, "flt_on");
    initControl (envAmtSlider, "flt_env_amt", true, true, 0.0);
    initControl (keyTrackSlider, "flt_key_track", true, false, 0.0);

    // --- 4. PAINEL DIREITO: EFEITOS & SAÍDA & XY PAD ---
    initControl (volumeSlider, "out_volume", true, false, 0.0);
    initControl (sineLevelSlider, "out_sine_level", true, false, 0.0);

    initCombo (distTypeBox, "fx_dist_type", { "Soft", "Hard" });
    initControl (distDriveSlider, "fx_dist_drive", true, false, 0.0);
    initControl (distToneSlider, "fx_dist_tone", true, false, 0.5);

    initCombo (modTypeBox, "fx_mod_type", { "Off", "Chorus", "Flanger", "Phaser" });
    initControl (modIntensitySlider, "fx_mod_intensity", true, false, 0.5);
    initControl (modSpeedSlider, "fx_mod_speed", true, false, 0.3);

    xyPad = std::make_unique<XYPadComponent> (pMixX, pMixY);
    addAndMakeVisible (*xyPad);

    // --- 5. PAINEL CENTRAL / LCD: 10 SLOTS DO ROUTER ---
    for (int i = 0; i < 10; ++i)
    {
        routerSlots[i] = std::make_unique<RouterSlotComponent> (processor.apvts, i + 1);
        addAndMakeVisible (*routerSlots[i]);
    }
    addAndMakeVisible (routerTabBtn);
    addAndMakeVisible (vectorTabBtn);
    initButton (vectorEnabledBtn, "vec_enabled");

    // --- 6. PAINEL INFERIOR: LFOS & ENVELOPES ---
    juce::StringArray lfoWaves { "Sine", "Tri", "Saw Up", "Saw Dn", "Square", "S&H", "Smooth" };
    initCombo (lfo1WaveBox, "lfo1_wave", lfoWaves);
    initControl (lfo1RateSlider, "lfo1_rate", false, false, 5.0);
    initControl (lfo1FadeSlider, "lfo1_fade", false, false, 0.0);

    initCombo (lfo2WaveBox, "lfo2_wave", lfoWaves);
    initControl (lfo2RateSlider, "lfo2_rate", false, false, 5.0);
    initControl (lfo2FadeSlider, "lfo2_fade", false, false, 0.0);

    // ENV 1 (Filtro)
    initControl (e2AttackSlider, "env2_attack", false, false, 0.005);
    initControl (e2DecaySlider, "env2_decay", false, false, 0.3);
    initControl (e2SustainSlider, "env2_sustain", false, false, 0.0);
    initControl (e2ReleaseSlider, "env2_release", false, false, 0.2);

    // ENV 2 (Mod)
    initControl (e3AttackSlider, "env3_attack", false, false, 0.005);
    initControl (e3DecaySlider, "env3_decay", false, false, 0.3);
    initControl (e3SustainSlider, "env3_sustain", false, false, 0.0);
    initControl (e3ReleaseSlider, "env3_release", false, false, 0.2);

    // ENV 3 (Amp)
    initControl (ampAttackSlider, "amp_attack", false, false, 0.005);
    initControl (ampDecaySlider, "amp_decay", false, false, 0.3);
    initControl (ampSustainSlider, "amp_sustain", false, false, 0.8);
    initControl (ampReleaseSlider, "amp_release", false, false, 0.2);
}

ES3Editor::~ES3Editor()
{
    setLookAndFeel (nullptr);
}

void ES3Editor::initControl (juce::Slider& s, const juce::String& paramId,
                             bool isRotary, bool isBipolar, double doubleClickDefault)
{
    if (isRotary)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    }
    else
    {
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    }
    s.getProperties().set ("bipolar", isBipolar);
    s.setDoubleClickReturnValue (true, doubleClickDefault);
    s.setScrollWheelEnabled (true);
    addAndMakeVisible (s);

    sliderAttach.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramId, s));
}

void ES3Editor::initCombo (juce::ComboBox& box, const juce::String& paramId,
                           const juce::StringArray& items)
{
    if (items.size() > 0)
    {
        for (int i = 0; i < items.size(); ++i)
            box.addItem (items[i], i + 1);
    }
    addAndMakeVisible (box);

    comboAttach.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, paramId, box));
}

void ES3Editor::initButton (juce::Button& btn, const juce::String& paramId, bool isToggle)
{
    if (isToggle)
        btn.setClickingTogglesState (true);
    addAndMakeVisible (btn);

    buttonAttach.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, paramId, btn));
}

void ES3Editor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // 1. FUNDO PRINCIPAL (Aço escovado chumbo/azul do Logic Pro)
    juce::ColourGradient bgGrad (Palette::bgPanel, 0.0f, 0.0f,
                                 Palette::bgDark, 0.0f, (float) bounds.getHeight(), false);
    g.setGradientFill (bgGrad);
    g.fillAll();

    // 2. HEADER BAR
    g.setColour (Palette::bgDark.withAlpha (0.8f));
    g.fillRect (0, 0, bounds.getWidth(), 36);
    g.setColour (Palette::borderBevel);
    g.drawHorizontalLine (36, 0.0f, (float) bounds.getWidth());

    // 3. DIAL CIRCULAR METÁLICO DOS FILTROS (CENTRO DO ES2)
    const float filterCenterX = bounds.getWidth() * 0.5f;
    const float filterCenterY = 175.0f;
    const float filterRadius = 125.0f;

    // Sombra do círculo dos filtros
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (filterCenterX - filterRadius + 2.0f, filterCenterY - filterRadius + 4.0f,
                   filterRadius * 2.0f, filterRadius * 2.0f);

    // Relevo metálico do círculo
    juce::ColourGradient ringGrad (Palette::bgPanelLight, filterCenterX, filterCenterY - filterRadius,
                                   Palette::bgDark, filterCenterX, filterCenterY + filterRadius, false);
    g.setGradientFill (ringGrad);
    g.fillEllipse (filterCenterX - filterRadius, filterCenterY - filterRadius,
                   filterRadius * 2.0f, filterRadius * 2.0f);

    g.setColour (Palette::borderBevel);
    g.drawEllipse (filterCenterX - filterRadius, filterCenterY - filterRadius,
                   filterRadius * 2.0f, filterRadius * 2.0f, 2.0f);

    // Círculo interno rebaixado
    g.setColour (Palette::bgDark.withAlpha (0.4f));
    g.fillEllipse (filterCenterX - filterRadius + 8.0f, filterCenterY - filterRadius + 8.0f,
                   (filterRadius - 8.0f) * 2.0f, (filterRadius - 8.0f) * 2.0f);

    // Texto de seções com fonte técnica limpa
    g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
    g.setColour (Palette::textMuted);

    // Labels de seções
    g.drawText ("OSCILLATORS", 15, 42, 100, 14, juce::Justification::left);
    g.drawText ("FILTER 1", (int) filterCenterX - 110, 60, 60, 14, juce::Justification::centred);
    g.drawText ("FILTER 2", (int) filterCenterX + 50, 60, 60, 14, juce::Justification::centred);
    g.drawText ("EFFECTS & OUTPUT", bounds.getWidth() - 170, 42, 150, 14, juce::Justification::right);
    g.drawText ("ROUTER", 15, 305, 80, 14, juce::Justification::left);
    g.drawText ("LFO 1", 20, 480, 60, 14, juce::Justification::centred);
    g.drawText ("LFO 2", 120, 480, 60, 14, juce::Justification::centred);
    g.drawText ("ENV 1 (FLT)", 250, 480, 90, 14, juce::Justification::centred);
    g.drawText ("ENV 2 (MOD)", 500, 480, 90, 14, juce::Justification::centred);
    g.drawText ("ENV 3 (AMP)", 750, 480, 90, 14, juce::Justification::centred);
}

void ES3Editor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    // --- 1. TOP BAR ---
    titleLabel.setBounds (12, 6, 60, 24);
    prevPresetBtn.setBounds (80, 7, 24, 22);
    presetBox.setBounds (108, 7, 180, 22);
    nextPresetBtn.setBounds (292, 7, 24, 22);

    polyBtn.setBounds (340, 8, 55, 20);
    monoBtn.setBounds (400, 8, 60, 20);
    legatoBtn.setBounds (465, 8, 65, 20);
    voicesSlider.setBounds (540, 7, 65, 22);
    unisonBtn.setBounds (615, 8, 65, 20);
    oscStartBox.setBounds (690, 7, 70, 22);
    fltResetBtn.setBounds (770, 7, 75, 22);

    // --- 2. PAINEL ESQUERDO: MASTER CONTROLS & OSCILADORES ---
    // Controles superiores da esquerda
    tuneSlider.setBounds (15, 60, 36, 36);
    analogSlider.setBounds (55, 60, 36, 36);
    glideSlider.setBounds (95, 60, 36, 36);
    bendRangeSlider.setBounds (135, 60, 36, 36);

    // 3 Linhas de osciladores
    for (int i = 0; i < 3; ++i)
    {
        const int y = 105 + i * 58;
        oscUI[i].waveBox.setBounds (15, y, 75, 20);
        oscUI[i].coarseSlider.setBounds (95, y - 6, 34, 34);
        oscUI[i].fineSlider.setBounds (135, y - 6, 34, 34);
        oscUI[i].pwSlider.setBounds (175, y - 6, 34, 34);
    }
    osc1FmSlider.setBounds (215, 100, 32, 32);
    osc3ColorSlider.setBounds (215, 216, 32, 32);

    // Triângulo de Mix (apontando para a direita)
    if (triangleMix != nullptr)
        triangleMix->setBounds (255, 100, 115, 175);

    // --- 3. PAINEL CENTRAL: FILTROS CIRCULARES ---
    const int midX = w / 2;
    // Filtro 1 (esquerda do círculo)
    f1ModeBox.setBounds (midX - 110, 80, 65, 20);
    f1CutSlider.setBounds (midX - 110, 110, 50, 50);
    f1ResSlider.setBounds (midX - 60, 130, 40, 40);
    f1DriveSlider.setBounds (midX - 105, 170, 40, 40);

    // Filtro 2 (direita do círculo)
    f2SlopeBox.setBounds (midX + 45, 80, 65, 20);
    f2CutSlider.setBounds (midX + 60, 110, 50, 50);
    f2ResSlider.setBounds (midX + 20, 130, 40, 40);

    // Meio do círculo
    blendSlider.setBounds (midX - 22, 100, 44, 44);
    routingBtn.setBounds (midX - 30, 150, 60, 20);
    fltOnBtn.setBounds (midX - 35, 175, 70, 20);
    envAmtSlider.setBounds (midX - 55, 220, 36, 36);
    keyTrackSlider.setBounds (midX + 20, 220, 36, 36);

    // --- 4. PAINEL DIREITO: EFEITOS & SAÍDA & XY PAD ---
    const int rX = w - 325;
    // Volume & Sine Level
    volumeSlider.setBounds (rX + 15, 60, 44, 44);
    sineLevelSlider.setBounds (rX + 65, 60, 40, 40);

    // Distorção
    distTypeBox.setBounds (rX + 15, 115, 60, 20);
    distDriveSlider.setBounds (rX + 15, 140, 40, 40);
    distToneSlider.setBounds (rX + 60, 140, 40, 40);

    // Mod FX
    modTypeBox.setBounds (rX + 15, 190, 75, 20);
    modIntensitySlider.setBounds (rX + 15, 215, 40, 40);
    modSpeedSlider.setBounds (rX + 60, 215, 40, 40);

    // XY Pad
    if (xyPad != nullptr)
        xyPad->setBounds (w - 180, 80, 160, 160);

    // --- 5. ROUTER (STRIP LCD AZUL) ---
    const int routerY = 320;
    const int routerH = 150;
    const int slotW = (w - 140) / 10;
    for (int i = 0; i < 10; ++i)
    {
        if (routerSlots[i] != nullptr)
            routerSlots[i]->setBounds (12 + i * slotW, routerY, slotW, routerH);
    }
    routerTabBtn.setBounds (w - 110, routerY + 15, 95, 26);
    vectorTabBtn.setBounds (w - 110, routerY + 48, 95, 26);
    vectorEnabledBtn.setBounds (w - 110, routerY + 82, 95, 24);

    // --- 6. PAINEL INFERIOR: LFOS & ENVELOPES ---
    const int botY = 500;
    const int faderH = 160;

    // LFO 1
    lfo1WaveBox.setBounds (15, botY, 70, 20);
    lfo1RateSlider.setBounds (20, botY + 25, 26, faderH - 30);
    lfo1FadeSlider.setBounds (55, botY + 25, 26, faderH - 30);

    // LFO 2
    lfo2WaveBox.setBounds (115, botY, 70, 20);
    lfo2RateSlider.setBounds (120, botY + 25, 26, faderH - 30);
    lfo2FadeSlider.setBounds (155, botY + 25, 26, faderH - 30);

    // ENV 1 (Filtro)
    const int e1X = 220;
    e2AttackSlider.setBounds (e1X, botY + 10, 24, faderH - 15);
    e2DecaySlider.setBounds (e1X + 32, botY + 10, 24, faderH - 15);
    e2SustainSlider.setBounds (e1X + 64, botY + 10, 24, faderH - 15);
    e2ReleaseSlider.setBounds (e1X + 96, botY + 10, 24, faderH - 15);

    // ENV 2 (Mod)
    const int e2X = 470;
    e3AttackSlider.setBounds (e2X, botY + 10, 24, faderH - 15);
    e3DecaySlider.setBounds (e2X + 32, botY + 10, 24, faderH - 15);
    e3SustainSlider.setBounds (e2X + 64, botY + 10, 24, faderH - 15);
    e3ReleaseSlider.setBounds (e2X + 96, botY + 10, 24, faderH - 15);

    // ENV 3 (Amp)
    const int e3X = 720;
    ampAttackSlider.setBounds (e3X, botY + 10, 24, faderH - 15);
    ampDecaySlider.setBounds (e3X + 32, botY + 10, 24, faderH - 15);
    ampSustainSlider.setBounds (e3X + 64, botY + 10, 24, faderH - 15);
    ampReleaseSlider.setBounds (e3X + 96, botY + 10, 24, faderH - 15);
}
