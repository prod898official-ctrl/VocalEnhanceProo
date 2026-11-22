#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout VocalEnhanceProAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pct = [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; };
    auto db  = [] (float v, int) { return String (v, 1) + " dB"; };
    auto hz  = [] (float v, int) { return String (roundToInt (v)) + " Hz"; };

    auto add = [&] (const char* id, const char* name, NormalisableRange<float> r, float def,
                    std::function<String (float, int)> fn)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                     AudioParameterFloatAttributes().withStringFromValueFunction (fn)));
    };

    add ("clean",    "Clean",    { 0.0f, 1.0f, 0.001f }, 0.40f, pct);
    add ("lowcut",   "Low Cut",  { 20.0f, 300.0f, 1.0f, 0.5f }, 80.0f, hz);
    add ("deess",    "De-Ess",   { 0.0f, 1.0f, 0.001f }, 0.30f, pct);
    add ("comp",     "Compress", { 0.0f, 1.0f, 0.001f }, 0.40f, pct);
    add ("warmth",   "Warmth",   { -6.0f, 6.0f, 0.1f },  0.0f,  db);
    add ("presence", "Presence", { -6.0f, 6.0f, 0.1f },  1.5f,  db);
    add ("air",      "Air",      { 0.0f, 8.0f, 0.1f },   2.0f,  db);
    add ("mix",      "Mix",      { 0.0f, 1.0f, 0.001f }, 1.0f,  pct);
    add ("output",   "Output",   { -12.0f, 12.0f, 0.1f }, 0.0f, db);

    return { p.begin(), p.end() };
}

VocalEnhanceProAudioProcessor::VocalEnhanceProAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pClean    = apvts.getRawParameterValue ("clean");
    pLowCut   = apvts.getRawParameterValue ("lowcut");
    pDeess    = apvts.getRawParameterValue ("deess");
    pComp     = apvts.getRawParameterValue ("comp");
    pWarmth   = apvts.getRawParameterValue ("warmth");
    pPresence = apvts.getRawParameterValue ("presence");
    pAir      = apvts.getRawParameterValue ("air");
    pMix      = apvts.getRawParameterValue ("mix");
    pOutput   = apvts.getRawParameterValue ("output");
}

bool VocalEnhanceProAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet();
    const auto out = l.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void VocalEnhanceProAudioProcessor::updateFilters (bool force)
{
    const float lc = pLowCut->load(), clean = pClean->load(), wm = pWarmth->load(),
                pr = pPresence->load(), ai = pAir->load();
    const float nyq = (float) (sr * 0.45);

    if (force || lc != lastLc)
    {
        auto c1 = Coeff::makeHighPass (sr, juce::jmin (lc, nyq), 0.5412f);
        auto c2 = Coeff::makeHighPass (sr, juce::jmin (lc, nyq), 1.3066f);
        for (int c = 0; c < 2; ++c) { hp1[c].coefficients = c1; hp2[c].coefficients = c2; }
        lastLc = lc;
    }
    if (force || clean != lastClean)
    {
        auto c = Coeff::makePeakFilter (sr, 280.0f, 0.9f, juce::Decibels::decibelsToGain (-3.0f * clean));
        for (int ch = 0; ch < 2; ++ch) mud[ch].coefficients = c;
        lastClean = clean;
    }
    if (force || wm != lastWarm)
    {
        auto c = Coeff::makeLowShelf (sr, 200.0f, 0.7f, juce::Decibels::decibelsToGain (wm));
        for (int ch = 0; ch < 2; ++ch) warm[ch].coefficients = c;
        lastWarm = wm;
    }
    if (force || pr != lastPres)
    {
        auto c = Coeff::makePeakFilter (sr, 3800.0f, 0.8f, juce::Decibels::decibelsToGain (pr));
        for (int ch = 0; ch < 2; ++ch) pres[ch].coefficients = c;
        lastPres = pr;
    }
    if (force || ai != lastAir)
    {
        auto c = Coeff::makeHighShelf (sr, juce::jmin (11000.0f, nyq), 0.7f, juce::Decibels::decibelsToGain (ai));
        for (int ch = 0; ch < 2; ++ch) air[ch].coefficients = c;
        lastAir = ai;
    }
}

void VocalEnhanceProAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    auto dc = Coeff::makeHighPass (sr, juce::jmin (6000.0f, (float) (sr * 0.45)), 0.7071f);
    for (int c = 0; c < 2; ++c)
    {
        hp1[c].reset(); hp2[c].reset(); mud[c].reset(); warm[c].reset();
        pres[c].reset(); air[c].reset(); deHP[c].reset();
        deHP[c].coefficients = dc;
    }
    updateFilters (true);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 2 };
    compressor.prepare (spec);
    compressor.reset();

    dry.setSize (2, juce::jmax (1, samplesPerBlock));
    mixSm.reset (sr, 0.03);  mixSm.setCurrentAndTargetValue (pMix->load());
    outSm.reset (sr, 0.03);  outSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));

    gateEnv = 0.0f; gateGain = 1.0f; deEnv = 0.0f; lastMakeup = 1.0f;
}

void VocalEnhanceProAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (2, getTotalNumInputChannels());
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);
    if (numCh <= 0 || n == 0) return;

    const float clean = pClean->load(), deess = pDeess->load(), compAmt = pComp->load();
    const float mix = pMix->load();
    const float outGain = juce::Decibels::decibelsToGain (pOutput->load());

    // meter wejscia + kopia dry
    float inPeak = 0.0f;
    dry.setSize (numCh, n, false, false, true);
    for (int c = 0; c < numCh; ++c)
    {
        inPeak = juce::jmax (inPeak, buffer.getMagnitude (c, 0, n));
        dry.copyFrom (c, 0, buffer, c, 0, n);
    }
    if (inPeak > inLevel.load()) inLevel.store (inPeak);

    updateFilters (false);

    float* d[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    // --- wspolczynniki ---
    const float envAtk  = std::exp (-1.0f / (0.001f * (float) sr));
    const float envRel  = std::exp (-1.0f / (0.040f * (float) sr));
    const float openC   = 1.0f - std::exp (-1.0f / (0.002f * (float) sr));
    const float closeC  = 1.0f - std::exp (-1.0f / (0.060f * (float) sr));
    const float deAtk   = std::exp (-1.0f / (0.0005f * (float) sr));
    const float deRel   = std::exp (-1.0f / (0.050f * (float) sr));

    const bool  gateOn  = clean > 0.01f;
    const float gateThrDb  = -75.0f + clean * 35.0f;
    const float gateThrLin = juce::Decibels::decibelsToGain (gateThrDb);
    const float gateMaxRed = 6.0f + clean * 24.0f;

    const bool  deOn    = deess > 0.01f;
    const float deThrDb = -20.0f - deess * 14.0f;
    const float deThrLin = juce::Decibels::decibelsToGain (deThrDb);

    for (int i = 0; i < n; ++i)
    {
        float x[2] = { 0.0f, 0.0f };
        float det = 0.0f;

        for (int c = 0; c < numCh; ++c)
        {
            float v = d[c][i];
            v = hp1[c].processSample (v);
            v = hp2[c].processSample (v);
            v = mud[c].processSample (v);
            x[c] = v;
            det = juce::jmax (det, std::abs (v));
        }

        // --- Clean: expander / gate ---
        if (gateOn)
        {
            const float k = det > gateEnv ? envAtk : envRel;
            gateEnv = det + k * (gateEnv - det);

            float target = 1.0f;
            if (gateEnv < gateThrLin)
            {
                const float envDb = juce::Decibels::gainToDecibels (gateEnv, -120.0f);
                const float red = juce::jmin (gateMaxRed, (gateThrDb - envDb) * 2.0f);
                target = juce::Decibels::decibelsToGain (-red);
            }
            gateGain += (target - gateGain) * (target < gateGain ? closeC : openC);
            for (int c = 0; c < numCh; ++c) x[c] *= gateGain;
        }

        // --- De-esser ---
        if (deOn)
        {
            float h[2] = { 0.0f, 0.0f };
            float hd = 0.0f;
            for (int c = 0; c < numCh; ++c)
            {
                h[c] = deHP[c].processSample (x[c]);
                hd = juce::jmax (hd, std::abs (h[c]));
            }
            const float k = hd > deEnv ? deAtk : deRel;
            deEnv = hd + k * (deEnv - hd);

            float g = 1.0f;
            if (deEnv > deThrLin)
            {
                const float envDb = juce::Decibels::gainToDecibels (deEnv, -120.0f);
                const float red = juce::jmin (12.0f, (envDb - deThrDb) * 0.75f);
                g = juce::Decibels::decibelsToGain (-red);
            }
            for (int c = 0; c < numCh; ++c) x[c] = (x[c] - h[c]) + h[c] * g;
        }

        for (int c = 0; c < numCh; ++c) d[c][i] = x[c];
    }

    // --- Kompresor ---
    float makeup = 1.0f;
    if (compAmt > 0.01f)
    {
        compressor.setThreshold (-4.0f - compAmt * 20.0f);
        compressor.setRatio (1.5f + compAmt * 3.5f);
        compressor.setAttack (8.0f);
        compressor.setRelease (140.0f);

        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        compressor.process (ctx);

        makeup = juce::Decibels::decibelsToGain (compAmt * 5.0f);
    }
    buffer.applyGainRamp (0, n, lastMakeup, makeup);
    lastMakeup = makeup;

    // --- Ton: warmth / presence / air ---
    for (int c = 0; c < numCh; ++c)
    {
        float* s = d[c];
        for (int i = 0; i < n; ++i)
        {
            float v = s[i];
            v = warm[c].processSample (v);
            v = pres[c].processSample (v);
            v = air[c].processSample (v);
            s[i] = v;
        }
    }

    // --- Mix + Output + miekki limiter ---
    mixSm.setTargetValue (mix);
    outSm.setTargetValue (outGain);
    float outPeak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float m = mixSm.getNextValue();
        const float o = outSm.getNextValue();
        for (int c = 0; c < numCh; ++c)
        {
            float y = (d[c][i] * m + dry.getReadPointer (c)[i] * (1.0f - m)) * o;
            const float a = std::abs (y);
            if (a > 0.95f)
                y = std::copysign (0.95f + 0.05f * std::tanh ((a - 0.95f) / 0.05f), y);
            d[c][i] = y;
            outPeak = juce::jmax (outPeak, std::abs (y));
        }
    }
    if (outPeak > outLevel.load()) outLevel.store (outPeak);
}

juce::AudioProcessorEditor* VocalEnhanceProAudioProcessor::createEditor()
{
    return new VocalEnhanceProAudioProcessorEditor (*this);
}

void VocalEnhanceProAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalEnhanceProAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalEnhanceProAudioProcessor();
}
