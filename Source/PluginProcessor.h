#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

class VocalEnhanceProAudioProcessor : public juce::AudioProcessor
{
public:
    VocalEnhanceProAudioProcessor();
    ~VocalEnhanceProAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VocalEnhancePro"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> inLevel { 0.0f }, outLevel { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters (bool force);

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeff  = juce::dsp::IIR::Coefficients<float>;

    Filter hp1[2], hp2[2], mud[2], warm[2], pres[2], air[2], deHP[2];
    juce::dsp::Compressor<float> compressor;
    juce::AudioBuffer<float> dry;
    juce::SmoothedValue<float> mixSm, outSm;

    std::atomic<float> *pClean, *pLowCut, *pDeess, *pComp, *pWarmth, *pPresence, *pAir, *pMix, *pOutput;

    double sr = 44100.0;
    float lastLc = -1, lastClean = -1, lastWarm = -100, lastPres = -100, lastAir = -100;
    float gateEnv = 0.0f, gateGain = 1.0f, deEnv = 0.0f, lastMakeup = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalEnhanceProAudioProcessor)
};
