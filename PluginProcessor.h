#pragma once
#include <JuceHeader.h>

class SaturaceAudioProcessor : public juce::AudioProcessor,
                                public juce::AudioProcessorValueTreeState::Listener
{
public:
    SaturaceAudioProcessor();
    ~SaturaceAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                             { return 1; }
    int getCurrentProgram() override                          { return 0; }
    void setCurrentProgram(int) override                      {}
    const juce::String getProgramName(int) override           { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;

    // Metering
    std::atomic<float> inputLevelL  { 0.f };
    std::atomic<float> inputLevelR  { 0.f };
    std::atomic<float> outputLevelL { 0.f };
    std::atomic<float> outputLevelR { 0.f };
    std::atomic<bool>  clipping     { false };

    // Spectrum data (FFT)
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder; // 2048
    std::array<float, fftSize> fftDataIn  {};
    std::array<float, fftSize> fftDataOut {};
    std::atomic<bool> fftDataReady { false };
    juce::AbstractFifo fifo { fftSize };
    std::array<float, fftSize> fifoBuffer {};
    int fifoIndex { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float saturate(float x, int mode, float drive) noexcept;
    void  pushSampleToFifo(float sample) noexcept;

    float dcPrevIn[2]  { 0.f, 0.f };
    float dcPrevOut[2] { 0.f, 0.f };

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturaceAudioProcessor)
};
