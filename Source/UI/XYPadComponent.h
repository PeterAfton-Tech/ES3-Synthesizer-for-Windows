#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ES3LookAndFeel.h"

namespace es3::ui
{

class XYPadComponent : public juce::Component
{
public:
    XYPadComponent (juce::RangedAudioParameter* paramX, juce::RangedAudioParameter* paramY)
        : pX (paramX), pY (paramY)
    {
        setRepaintsOnMouseActivity (true);
    }

    void setNormValues (float nx, float ny)
    {
        normX = std::clamp (nx, 0.0f, 1.0f);
        normY = std::clamp (ny, 0.0f, 1.0f);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);

        // Fundo do Pad LCD escuro
        g.setColour (Palette::lcdBg);
        g.fillRoundedRectangle (bounds, 4.0f);

        // Borda chanfrada
        g.setColour (Palette::borderDark);
        g.drawRoundedRectangle (bounds, 4.0f, 1.5f);

        // Grid interno
        g.setColour (Palette::cyanDim.withAlpha (0.25f));
        g.drawHorizontalLine ((int) bounds.getCentreY(), bounds.getX(), bounds.getRight());
        g.drawVerticalLine ((int) bounds.getCentreX(), bounds.getY(), bounds.getBottom());

        // Eixos x e y labels
        g.setFont (juce::Font (10.0f, juce::Font::plain));
        g.setColour (Palette::cyanDim);
        g.drawText ("x", (int) bounds.getRight() - 14, (int) bounds.getBottom() - 14, 10, 10, juce::Justification::centred);
        g.drawText ("y", (int) bounds.getX() + 4, (int) bounds.getY() + 4, 10, 10, juce::Justification::centred);

        // Ponto do cursor
        const float px = bounds.getX() + normX * bounds.getWidth();
        const float py = bounds.getY() + (1.0f - normY) * bounds.getHeight();

        // Crosshairs
        g.setColour (Palette::cyan.withAlpha (0.4f));
        g.drawHorizontalLine ((int) py, bounds.getX(), bounds.getRight());
        g.drawVerticalLine ((int) px, bounds.getY(), bounds.getBottom());

        // Puck
        g.setColour (Palette::cyan);
        g.drawRect (px - 4.0f, py - 4.0f, 8.0f, 8.0f, 1.2f);
        g.setColour (Palette::orange);
        g.fillEllipse (px - 1.5f, py - 1.5f, 3.0f, 3.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        updateFromMouse (e.position);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        updateFromMouse (e.position);
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        setValuesAndNotify (0.5f, 0.5f);
    }

private:
    void updateFromMouse (juce::Point<float> pt)
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        float nx = std::clamp ((pt.x - bounds.getX()) / bounds.getWidth(), 0.0f, 1.0f);
        float ny = 1.0f - std::clamp ((pt.y - bounds.getY()) / bounds.getHeight(), 0.0f, 1.0f);
        setValuesAndNotify (nx, ny);
    }

    void setValuesAndNotify (float nx, float ny)
    {
        normX = nx;
        normY = ny;
        if (pX != nullptr) pX->setValueNotifyingHost (pX->convertTo0to1 (normX));
        if (pY != nullptr) pY->setValueNotifyingHost (pY->convertTo0to1 (normY));
        repaint();
    }

    juce::RangedAudioParameter* pX = nullptr;
    juce::RangedAudioParameter* pY = nullptr;
    float normX = 0.5f;
    float normY = 0.5f;
};

} // namespace es3::ui
