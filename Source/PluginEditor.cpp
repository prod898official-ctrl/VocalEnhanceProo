#include "PluginEditor.h"

namespace
{
    const char* kIds[9]   = { "clean", "lowcut", "deess", "comp", "warmth", "presence", "air", "mix", "output" };
    const char* kNames[9] = { "CLEAN", "LOW CUT", "DE-ESS", "COMPRESS", "WARMTH", "PRESENCE", "AIR", "MIX", "OUTPUT" };
    const char* kPanels[4] = { "CLEAN UP", "DYNAMICS", "TONE", "OUTPUT" };
    const int   kCounts[4] = { 3, 1, 3, 2 };
}

VocalEnhanceProAudioProcessorEditor::VocalEnhanceProAudioProcessorEditor (VocalEnhanceProAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < 9; ++i)
    {
        auto& k = knobs[(size_t) i];

        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        k.slider.setLookAndFeel (&laf);
        k.slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe9e4ff));
        k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (k.slider);

        k.label.setText (kNames[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
        k.label.setColour (juce::Label::textColourId, juce::Colour (0xffb9a7ff));
        addAndMakeVisible (k.label);

        k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kIds[i], k.slider);

        if (auto* par = proc.apvts.getParameter (kIds[i]))
            k.slider.setDoubleClickReturnValue (true, par->convertFrom0to1 (par->getDefaultValue()));
    }

    setSize (960, 400);
    startTimerHz (30);
}

VocalEnhanceProAudioProcessorEditor::~VocalEnhanceProAudioProcessorEditor()
{
    stopTimer();
    for (auto& k : knobs) k.slider.setLookAndFeel (nullptr);
}

void VocalEnhanceProAudioProcessorEditor::timerCallback()
{
    inL  = juce::jmax (proc.inLevel.exchange (0.0f),  inL  * 0.86f);
    outL = juce::jmax (proc.outLevel.exchange (0.0f), outL * 0.86f);
    repaint (meterArea);
}

void VocalEnhanceProAudioProcessorEditor::drawMeter (juce::Graphics& g, juce::Rectangle<float> r, float level, const juce::String& name)
{
    g.setColour (juce::Colour (0xff9b8cff));
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText (name, r.removeFromLeft (34.0f), juce::Justification::centredLeft);

    auto bar = r.withSizeKeepingCentre (r.getWidth(), 10.0f);
    g.setColour (juce::Colour (0xff0e1028));
    g.fillRoundedRectangle (bar, 5.0f);

    const float db = juce::Decibels::gainToDecibels (level, -60.0f);
    const float norm = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f));
    auto fill = bar.withWidth (bar.getWidth() * norm);
    if (fill.getWidth() > 1.0f)
    {
        juce::ColourGradient gr (juce::Colour (0xff5b2cff), bar.getX(), 0.0f,
                                 juce::Colour (0xffff3cf0), bar.getRight(), 0.0f, false);
        g.setColour (juce::Colour (0xffb14dff).withAlpha (0.25f));
        g.fillRoundedRectangle (fill.expanded (0.0f, 3.0f), 6.0f);
        g.setGradientFill (gr);
        g.fillRoundedRectangle (fill, 5.0f);
    }
}

void VocalEnhanceProAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (juce::Colour (0xff0d1236), 0.0f, 0.0f,
                             juce::Colour (0xff03030b), 0.0f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    // fioletowa poswiata w rogu
    juce::ColourGradient glow (juce::Colour (0x449b3dff), (float) getWidth(), 0.0f,
                               juce::Colours::transparentBlack, (float) getWidth() * 0.45f, 220.0f, true);
    g.setGradientFill (glow);
    g.fillAll();

    // naglowek
    const auto f = juce::Font (juce::FontOptions (30.0f, juce::Font::bold));
    juce::GlyphArrangement ga;
    ga.addLineOfText (f, "VOCAL ENHANCE ", 0.0f, 0.0f);
    const float w1 = ga.getBoundingBox (0, -1, true).getWidth();

    g.setFont (f);
    g.setColour (juce::Colours::white);
    g.drawText ("VOCAL ENHANCE", 26, 16, 400, 40, juce::Justification::centredLeft);

    for (int i = 4; i >= 1; --i)
    {
        g.setColour (NeonLookAndFeel::neon().withAlpha (0.08f));
        g.drawText ("PRO", 26 + (int) w1 - i, 16 - i, 200, 40 + 2 * i, juce::Justification::centredLeft);
    }
    g.setColour (NeonLookAndFeel::neon());
    g.drawText ("PRO", 26 + (int) w1, 16, 200, 40, juce::Justification::centredLeft);

    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.setColour (juce::Colour (0xff7f78c8));
    g.drawText ("VOCAL CLEANER  |  VST3", 28, 52, 300, 16, juce::Justification::centredLeft);

    // neonowa linia
    juce::ColourGradient line (juce::Colour (0xff3a2cff), 20.0f, 0.0f,
                               juce::Colour (0xffff3cf0), (float) getWidth() - 20.0f, 0.0f, false);
    g.setColour (juce::Colour (0xff9b3dff).withAlpha (0.25f));
    g.fillRect (20.0f, 74.0f, (float) getWidth() - 40.0f, 5.0f);
    g.setGradientFill (line);
    g.fillRect (20.0f, 75.5f, (float) getWidth() - 40.0f, 2.0f);

    // panele
    for (int i = 0; i < 4; ++i)
    {
        auto r = panels[i].toFloat();
        g.setColour (juce::Colour (0x40101440));
        g.fillRoundedRectangle (r, 12.0f);
        g.setColour (juce::Colour (0xff7a4dff).withAlpha (0.35f));
        g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.2f);

        g.setColour (juce::Colour (0xff8f7bff));
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText (kPanels[i], panels[i].withHeight (26).reduced (12, 0), juce::Justification::centredLeft);
    }

    // mierniki
    auto m = meterArea.toFloat();
    drawMeter (g, m.removeFromLeft (m.getWidth() * 0.5f - 10.0f), inL, "IN");
    m.removeFromLeft (20.0f);
    drawMeter (g, m, outL, "OUT");
}

void VocalEnhanceProAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (20);
    r.removeFromTop (66);
    meterArea = r.removeFromBottom (36);
    auto area = r.reduced (0, 6);

    const int gap = 14;
    const int cellW = (area.getWidth() - 3 * gap) / 9;

    int idx = 0, x = area.getX();
    for (int p = 0; p < 4; ++p)
    {
        const int w = kCounts[p] * cellW;
        panels[p] = { x, area.getY(), w, area.getHeight() };

        for (int k = 0; k < kCounts[p]; ++k)
        {
            juce::Rectangle<int> cell (x + k * cellW, area.getY() + 28, cellW, area.getHeight() - 32);
            auto& kn = knobs[(size_t) idx++];
            kn.label.setBounds (cell.removeFromTop (20));
            kn.slider.setBounds (cell);
        }
        x += w + gap;
    }
}
