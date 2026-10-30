#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ES3LookAndFeel.h"

namespace es3::ui
{

class TriangleMixComponent : public juce::Component
{
public:
    TriangleMixComponent (juce::RangedAudioParameter* paramX, juce::RangedAudioParameter* paramY)
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
        auto bounds = getLocalBounds().toFloat().reduced (8.0f);

        // Vértices do triângulo (apontando para a direita, como no ES2)
        // V1 (Osc 1): topo esquerdo
        // V2 (Osc 2): ponta direita
        // V3 (Osc 3): base esquerda
        v1 = juce::Point<float> (bounds.getX(), bounds.getY());
        v2 = juce::Point<float> (bounds.getRight(), bounds.getCentreY());
        v3 = juce::Point<float> (bounds.getX(), bounds.getBottom());

        juce::Path trianglePath;
        trianglePath.startNewSubPath (v1);
        trianglePath.lineTo (v2);
        trianglePath.lineTo (v3);
        trianglePath.closeSubPath();

        // 1. Fundo do triângulo com gradiente escuro esverdeado/azulado
        juce::ColourGradient bgGrad (juce::Colour (0xff0a1c22), bounds.getCentreX(), bounds.getY(),
                                     juce::Colour (0xff040e12), bounds.getCentreX(), bounds.getBottom(), false);
        g.setGradientFill (bgGrad);
        g.fillPath (trianglePath);

        // 2. Linhas de grade sutis
        g.setColour (Palette::cyanDim.withAlpha (0.18f));
        g.drawLine (v1.x, v1.y, (v2.x + v3.x) * 0.5f, (v2.y + v3.y) * 0.5f, 1.0f);
        g.drawLine (v2.x, v2.y, (v1.x + v3.x) * 0.5f, (v1.y + v3.y) * 0.5f, 1.0f);
        g.drawLine (v3.x, v3.y, (v1.x + v2.x) * 0.5f, (v1.y + v2.y) * 0.5f, 1.0f);

        // 3. Borda do triângulo
        g.setColour (Palette::cyanDim);
        g.strokePath (trianglePath, juce::PathStrokeType (1.5f));

        // 4. Indicadores dos osciladores nos vértices
        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.setColour (Palette::cyan);
        g.drawText ("1", (int) v1.x + 3, (int) v1.y + 1, 14, 14, juce::Justification::centred);
        g.drawText ("2", (int) v2.x - 14, (int) v2.y - 7, 14, 14, juce::Justification::centred);
        g.drawText ("3", (int) v3.x + 3, (int) v3.y - 15, 14, 14, juce::Justification::centred);

        // 5. Posição atual do ponto no triângulo
        // Mapeamento: x vai de 0 a 1 (de v1/v3 até v2), y vai de 0 a 1 (de v1 até v3)
        juce::Point<float> puckPos = normToPoint (normX, normY);

        // Retículo / Mira do cursor
        g.setColour (Palette::cyan.withAlpha (0.75f));
        g.drawRect (puckPos.x - 5.0f, puckPos.y - 5.0f, 10.0f, 10.0f, 1.0f);
        g.setColour (Palette::orange);
        g.fillEllipse (puckPos.x - 2.0f, puckPos.y - 2.0f, 4.0f, 4.0f);
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
        // Centraliza
        setValuesAndNotify (0.333f, 0.5f);
    }

private:
    juce::Point<float> normToPoint (float nx, float ny) const
    {
        // Interpolação dentro da caixa delimitadora do triângulo
        float px = v1.x + nx * (v2.x - v1.x);
        float py = v1.y + ny * (v3.y - v1.y);
        return { px, py };
    }

    void updateFromMouse (juce::Point<float> pt)
    {
        if (v2.x <= v1.x || v3.y <= v1.y) return;
        float nx = std::clamp ((pt.x - v1.x) / (v2.x - v1.x), 0.0f, 1.0f);
        float ny = std::clamp ((pt.y - v1.y) / (v3.y - v1.y), 0.0f, 1.0f);
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
    float normX = 0.0f;
    float normY = 0.0f;
    juce::Point<float> v1, v2, v3;
};

} // namespace es3::ui
