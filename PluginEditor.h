#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ─── Spectrum Analyser Component ───────────────────────────────────────────
class SpectrumAnalyser : public juce::Component,
                          private juce::Timer
{
public:
    SpectrumAnalyser(SaturaceAudioProcessor& p);
    ~SpectrumAnalyser() override { stopTimer(); }
    void paint(juce::Graphics& g) override;
    void resized() override {}

private:
    void timerCallback() override;
    void drawFrame(juce::Graphics& g);

    SaturaceAudioProcessor& proc;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    static constexpr int fftOrder = SaturaceAudioProcessor::fftOrder;
    static constexpr int fftSize  = SaturaceAudioProcessor::fftSize;

    std::array<float, fftSize * 2> fftData {};
    std::array<float, fftSize / 2> scopeData {};
    static constexpr int scopeSize = fftSize / 2;

    // Smoothed scope
    std::array<float, scopeSize> smoothedScope {};
};

// ─── Custom LookAndFeel ────────────────────────────────────────────────────
class FutureLAF : public juce::LookAndFeel_V4
{
public:
    FutureLAF();

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int w, int h,
                          float pos, float startAngle, float endAngle,
                          juce::Slider&) override;

    void drawLinearSlider(juce::Graphics& g,
                          int x, int y, int w, int h,
                          float pos, float minPos, float maxPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                               const juce::Colour& bgColour,
                               bool hover, bool down) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                        bool hover, bool down) override;
};

// ─── Main Editor ──────────────────────────────────────────────────────────
class SaturaceAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit SaturaceAudioProcessorEditor(SaturaceAudioProcessor&);
    ~SaturaceAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawLevelMeter(juce::Graphics& g, juce::Rectangle<float> b,
                        float levelL, float levelR, bool clip);
    void drawHexGrid(juce::Graphics& g);

    SaturaceAudioProcessor& audioProcessor;
    FutureLAF laf;

    // Main knob
    juce::Slider driveKnob, outputKnob, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        driveAttach, outputAttach, mixAttach;

    // Mode buttons
    juce::TextButton modeButtons[6];
    juce::Label      driveLabel, outputLabel, mixLabel;

    // Bypass
    juce::TextButton bypassBtn { "BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;

    // Spectrum
    SpectrumAnalyser spectrum;

    // Meter data
    float inputLevelL  { 0.f }, inputLevelR  { 0.f };
    float outputLevelL { 0.f }, outputLevelR { 0.f };
    bool  clipState    { false };

    // Animated glow phase
    float glowPhase { 0.f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturaceAudioProcessorEditor)
};
