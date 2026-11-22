#pragma once
#include "PluginProcessor.h"

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static juce::Colour neon()  { return juce::Colour (0xffb14dff); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
        const float radius = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const float arcR = radius - 2.0f;

        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xff1b1e3a));
        g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (neon().withAlpha (0.07f * (float) (4 - i)));
            g.strokePath (val, juce::PathStrokeType (5.0f + (float) i * 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (neon());
        g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float br = radius - 12.0f;
        juce::ColourGradient grad (juce::Colour (0xff2c3262), c.x - br * 0.5f, c.y - br * 0.7f,
                                   juce::Colour (0xff060716), c.x + br * 0.5f, c.y + br, false);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
        g.setColour (juce::Colour (0xff6a3df0).withAlpha (0.6f));
        g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.5f);

        juce::Path p;
        p.addRoundedRectangle (-1.8f, -br + 4.0f, 3.6f, br * 0.42f, 1.5f);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
        g.setColour (juce::Colour (0xfff0e2ff));
        g.fillPath (p);
    }
};

class VocalEnhanceProAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VocalEnhanceProAudioProcessorEditor (VocalEnhanceProAudioProcessor&);
    ~VocalEnhanceProAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawMeter (juce::Graphics&, juce::Rectangle<float>, float level, const juce::String& name);

    VocalEnhanceProAudioProcessor& proc;
    NeonLookAndFeel laf;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    std::array<Knob, 9> knobs;

    juce::Rectangle<int> panels[4];
    juce::Rectangle<int> meterArea;
    float inL = 0.0f, outL = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalEnhanceProAudioProcessorEditor)
};
