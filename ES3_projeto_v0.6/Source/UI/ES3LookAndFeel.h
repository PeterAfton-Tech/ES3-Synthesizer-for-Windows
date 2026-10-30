#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace es3::ui
{

struct Palette
{
    static inline const juce::Colour bgDark      { 0xff182026 };
    static inline const juce::Colour bgMid       { 0xff263038 };
    static inline const juce::Colour bgPanel     { 0xff2f3b45 };
    static inline const juce::Colour bgPanelLight{ 0xff3e4c57 };
    static inline const juce::Colour borderBevel { 0xff50606d };
    static inline const juce::Colour borderDark  { 0xff10151a };
    static inline const juce::Colour orange      { 0xffff8c00 };
    static inline const juce::Colour orangeBright{ 0xffffa500 };
    static inline const juce::Colour orangeGlow  { 0x66ff8c00 };
    static inline const juce::Colour cyan        { 0xff00d4ff };
    static inline const juce::Colour cyanDim     { 0xff0088aa };
    static inline const juce::Colour lcdBg       { 0xff08141d };
    static inline const juce::Colour lcdSlot     { 0xff0e1e2b };
    static inline const juce::Colour textLight   { 0xffedf2f7 };
    static inline const juce::Colour textMuted   { 0xff8a9ba8 };
};

class ES3LookAndFeel : public juce::LookAndFeel_V4
{
public:
    ES3LookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, Palette::bgDark);
        setColour (juce::Label::textColourId, Palette::textLight);
        setColour (juce::ComboBox::backgroundColourId, Palette::lcdBg);
        setColour (juce::ComboBox::textColourId, Palette::cyan);
        setColour (juce::ComboBox::outlineColourId, Palette::borderDark);
        setColour (juce::ComboBox::arrowColourId, Palette::orange);
        setColour (juce::PopupMenu::backgroundColourId, Palette::bgPanel);
        setColour (juce::PopupMenu::textColourId, Palette::textLight);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Palette::orange);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
        setColour (juce::TextButton::buttonColourId, Palette::bgPanel);
        setColour (juce::TextButton::buttonOnColourId, Palette::orange);
        setColour (juce::TextButton::textColourOffId, Palette::textLight);
        setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    }

    // --- ROTARY SLIDERS (KNOBS ESTILO ES2) ---
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        const float radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        const float centreX = (float) x + (float) width * 0.5f;
        const float centreY = (float) y + (float) height * 0.5f;
        const float rx = centreX - radius;
        const float ry = centreY - radius;
        const float rw = radius * 2.0f;
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // 1. Sombra externa
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (rx + 1.5f, ry + 2.5f, rw, rw);

        // 2. Bezel metálico externo (chanfro)
        juce::ColourGradient bezelGrad (Palette::borderBevel, centreX, ry,
                                        Palette::borderDark, centreX, ry + rw, false);
        g.setGradientFill (bezelGrad);
        g.fillEllipse (rx, ry, rw, rw);

        // 3. Rebaixo do indicador de arco
        const float innerR = radius - 3.5f;
        g.setColour (Palette::bgDark);
        g.fillEllipse (centreX - innerR, centreY - innerR, innerR * 2.0f, innerR * 2.0f);

        // 4. Arco luminoso laranja do ES2
        const bool isBipolar = slider.getProperties().contains ("bipolar") && (bool) slider.getProperties()["bipolar"];
        const float arcRadius = innerR - 2.5f;
        juce::Path arcPath;
        if (isBipolar)
        {
            const float midAngle = (rotaryStartAngle + rotaryEndAngle) * 0.5f;
            if (angle >= midAngle)
                arcPath.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f, midAngle, angle, true);
            else
                arcPath.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f, angle, midAngle, true);
        }
        else
        {
            arcPath.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
        }

        // Glow do arco
        g.setColour (Palette::orangeGlow);
        g.strokePath (arcPath, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // Linha nítida do arco
        g.setColour (Palette::orange);
        g.strokePath (arcPath, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 5. Corpo interno do knob metálico (alumínio escovado)
        const float knobR = arcRadius - 4.5f;
        juce::ColourGradient knobGrad (Palette::bgPanelLight, centreX - knobR * 0.5f, centreY - knobR * 0.5f,
                                       Palette::bgMid, centreX + knobR, centreY + knobR, true);
        g.setGradientFill (knobGrad);
        g.fillEllipse (centreX - knobR, centreY - knobR, knobR * 2.0f, knobR * 2.0f);

        // Borda sutil do dial interno
        g.setColour (Palette::borderDark);
        g.drawEllipse (centreX - knobR, centreY - knobR, knobR * 2.0f, knobR * 2.0f, 1.0f);

        // 6. Ponteiro / entalhe do knob (needle notch)
        juce::Path pointer;
        const float pointerLength = knobR * 0.75f;
        pointer.startNewSubPath (centreX + (knobR * 0.25f) * std::sin (angle),
                                 centreY - (knobR * 0.25f) * std::cos (angle));
        pointer.lineTo (centreX + pointerLength * std::sin (angle),
                        centreY - pointerLength * std::cos (angle));

        g.setColour (juce::Colours::white);
        g.strokePath (pointer, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Centro em relevo
        g.setColour (Palette::bgDark.withAlpha (0.6f));
        g.fillEllipse (centreX - 2.5f, centreY - 2.5f, 5.0f, 5.0f);
    }

    // --- LINEAR SLIDERS (FADERS VERTICAIS ESTILO ENVELOPE / LFO) ---
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        const float trackWidth = 4.0f;
        const float trackX = (float) x + (float) width * 0.5f - trackWidth * 0.5f;
        const float trackY = (float) y + 6.0f;
        const float trackH = (float) height - 12.0f;

        // 1. Canaleta rebaixada
        g.setColour (Palette::lcdBg);
        g.fillRoundedRectangle (trackX, trackY, trackWidth, trackH, 2.0f);
        g.setColour (Palette::borderDark);
        g.drawRoundedRectangle (trackX, trackY, trackWidth, trackH, 2.0f, 1.0f);

        // 2. Linha iluminada laranja até o thumb
        const bool isBipolar = slider.getProperties().contains ("bipolar") && (bool) slider.getProperties()["bipolar"];
        const float thumbY = sliderPos;

        if (isBipolar)
        {
            const float midY = trackY + trackH * 0.5f;
            const float startY = std::min (midY, thumbY);
            const float h = std::abs (thumbY - midY);
            g.setColour (Palette::orange);
            g.fillRect (trackX + 1.0f, startY, trackWidth - 2.0f, h);
        }
        else
        {
            const float fillH = (trackY + trackH) - thumbY;
            if (fillH > 0.0f)
            {
                g.setColour (Palette::orangeGlow);
                g.fillRect (trackX - 1.0f, thumbY, trackWidth + 2.0f, fillH);
                g.setColour (Palette::orange);
                g.fillRect (trackX + 1.0f, thumbY, trackWidth - 2.0f, fillH);
            }
        }

        // 3. Thumb metálico retangular do ES2
        const float thumbW = (float) juce::jmin (width - 4, 24);
        const float thumbH = 12.0f;
        const float thumbX = (float) x + (float) width * 0.5f - thumbW * 0.5f;

        // Sombra do thumb
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (thumbX + 1.0f, thumbY - thumbH * 0.5f + 1.5f, thumbW, thumbH, 2.0f);

        // Gradiente do thumb metálico
        juce::ColourGradient thumbGrad (Palette::borderBevel, thumbX, thumbY - thumbH * 0.5f,
                                        Palette::bgMid, thumbX, thumbY + thumbH * 0.5f, false);
        g.setGradientFill (thumbGrad);
        g.fillRoundedRectangle (thumbX, thumbY - thumbH * 0.5f, thumbW, thumbH, 2.0f);

        // Borda do thumb
        g.setColour (Palette::borderDark);
        g.drawRoundedRectangle (thumbX, thumbY - thumbH * 0.5f, thumbW, thumbH, 2.0f, 1.0f);

        // Entalhe central brilhante no thumb
        g.setColour (Palette::orange);
        g.fillRect (thumbX + 2.0f, thumbY - 1.0f, thumbW - 4.0f, 2.0f);
    }

    // --- COMBOBOX (MENUS LCD CIANO) ---
    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height);
        g.setColour (Palette::lcdBg);
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (Palette::cyanDim.withAlpha (0.4f));
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

        // Seta para baixo laranja
        juce::Path arrow;
        const float ax = (float) width - 12.0f;
        const float ay = (float) height * 0.5f;
        arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
        arrow.lineTo (ax + 4.0f, ay - 2.0f);
        arrow.lineTo (ax, ay + 3.0f);
        arrow.closeSubPath();
        g.setColour (Palette::orange);
        g.fillPath (arrow);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (2, 1, box.getWidth() - 16, box.getHeight() - 2);
        label.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
        label.setColour (juce::Label::textColourId, Palette::cyan);
        label.setJustificationType (juce::Justification::centredLeft);
    }

    // --- BUTTONS / LED RADIO ---
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        const float r = 4.5f;
        const float ledX = 6.0f;
        const float ledY = (float) button.getHeight() * 0.5f - r;

        // Borda rebaixada do LED
        g.setColour (Palette::borderDark);
        g.drawEllipse (ledX - 1.0f, ledY - 1.0f, (r + 1.0f) * 2.0f, (r + 1.0f) * 2.0f, 1.0f);

        if (button.getToggleState())
        {
            // LED LIGADO (laranja vibrante com glow)
            g.setColour (Palette::orangeGlow);
            g.fillEllipse (ledX - 2.5f, ledY - 2.5f, (r + 2.5f) * 2.0f, (r + 2.5f) * 2.0f);
            g.setColour (Palette::orangeBright);
            g.fillEllipse (ledX, ledY, r * 2.0f, r * 2.0f);
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.fillEllipse (ledX + 1.5f, ledY + 1.5f, r * 0.7f, r * 0.7f);
        }
        else
        {
            // LED DESLIGADO (âmbar escuro/cinza)
            g.setColour (juce::Colour (0xff251d14));
            g.fillEllipse (ledX, ledY, r * 2.0f, r * 2.0f);
        }

        // Texto do botão
        g.setColour (button.getToggleState() ? Palette::textLight : Palette::textMuted);
        g.setFont (juce::Font (juce::FontOptions (11.0f)));
        g.drawText (button.getButtonText(), (int) (ledX + r * 2.0f + 6.0f), 0,
                    button.getWidth() - (int) (ledX + r * 2.0f + 8.0f), button.getHeight(),
                    juce::Justification::centredLeft);
    }
};

} // namespace es3::ui
