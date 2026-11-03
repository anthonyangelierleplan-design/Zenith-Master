#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Master.h"

class ZenithProcessor : public juce::AudioProcessor
{
public:
    ZenithProcessor();
    ~ZenithProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Zenith"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // presets
    static int getNumFactoryPresets();
    static juce::String getFactoryPresetName (int);
    void loadFactoryPreset (int);
    juce::File getPresetDir() const;
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String&);
    bool loadUserPreset (const juce::String&);
    bool deleteUserPreset (const juce::String&);

    juce::AudioProcessorValueTreeState apvts;
    tn::Master master;
    juce::String presetName { "Init" };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    juce::ValueTree captureState();
    void applyState (const juce::ValueTree&);
    std::atomic<float>* raw[tn::kNumP] {};
    float values[tn::kNumP] {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZenithProcessor)
};
