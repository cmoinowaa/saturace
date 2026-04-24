#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout SaturaceAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("DRIVE", 1), "Drive",
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f), 0.f));

    // Mode: 0=Keep High 1=Neutral 2=Keep Low 3=Tape 4=Tube 5=Clipper
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("MODE", 1), "Mode",
        juce::StringArray{ "Keep High", "Neutral", "Keep Low", "Tape", "Tube", "Clip" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("OUTPUT", 1), "Output",
        juce::NormalisableRange<float>(-24.f, 6.f, 0.1f), 0.f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("MIX", 1), "Mix",
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f), 100.f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("BYPASS", 1), "Bypass", false));

    return { params.begin(), params.end() };
}

SaturaceAudioProcessor::SaturaceAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createParameterLayout()),
      fft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    apvts.addParameterListener("DRIVE",  this);
    apvts.addParameterListener("MODE",   this);
    apvts.addParameterListener("OUTPUT", this);
    apvts.addParameterListener("MIX",    this);
    apvts.addParameterListener("BYPASS", this);
}

SaturaceAudioProcessor::~SaturaceAudioProcessor()
{
    apvts.removeParameterListener("DRIVE",  this);
    apvts.removeParameterListener("MODE",   this);
    apvts.removeParameterListener("OUTPUT", this);
    apvts.removeParameterListener("MIX",    this);
    apvts.removeParameterListener("BYPASS", this);
}

void SaturaceAudioProcessor::prepareToPlay(double, int)
{
    std::fill(std::begin(dcPrevIn),  std::end(dcPrevIn),  0.f);
    std::fill(std::begin(dcPrevOut), std::end(dcPrevOut), 0.f);
    clipping.store(false);
    fifoIndex = 0;
    fftDataReady.store(false);
}

void SaturaceAudioProcessor::releaseResources() {}

bool SaturaceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

// ── Saturation algos ──────────────────────────────────────────────────────────
float SaturaceAudioProcessor::saturate(float x, int mode, float drive) noexcept
{
    if (drive < 0.001f) return x;
    const float gain = 1.f + drive * 0.19f;
    const float wet  = x * gain;
    const float comp = 1.f / std::sqrt(gain);
    float s;

    switch (mode)
    {
        case 0: // Keep High — soft on positives, harder on negatives
            s = (wet >= 0.f) ? std::tanh(wet) : std::tanh(wet * 1.5f) / 1.5f;
            break;
        case 2: // Keep Low — harder on positives
            s = (wet >= 0.f) ? std::tanh(wet * 1.5f) / 1.5f : std::tanh(wet);
            break;
        case 3: // Tape — smooth saturation with slight compression feel
            s = wet / (1.f + std::abs(wet * 0.5f));
            break;
        case 4: // Tube — even harmonics asymmetric
            s = std::tanh(wet) + 0.1f * wet * wet * (wet > 0.f ? 1.f : -1.f);
            s = juce::jlimit(-1.f, 1.f, s);
            break;
        case 5: // Hard Clip
            s = juce::jlimit(-0.8f, 0.8f, wet) / 0.8f;
            break;
        default: // Neutral tanh
            s = std::tanh(wet);
            break;
    }

    // DC blocker
    return s * comp;
}

void SaturaceAudioProcessor::pushSampleToFifo(float sample) noexcept
{
    if (fifoIndex == fftSize)
    {
        if (!fftDataReady.load())
        {
            std::copy(fifoBuffer.begin(), fifoBuffer.end(), fftDataIn.begin());
            fftDataReady.store(true);
        }
        fifoIndex = 0;
    }
    fifoBuffer[(size_t)fifoIndex++] = sample;
}

void SaturaceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int   numChannels = buffer.getNumChannels();
    const int   numSamples  = buffer.getNumSamples();
    const float drive       = apvts.getRawParameterValue("DRIVE") ->load();
    const int   mode        = (int)apvts.getRawParameterValue("MODE")->load();
    const float outputGain  = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("OUTPUT")->load());
    const float mix         = apvts.getRawParameterValue("MIX")->load() / 100.f;
    const bool  bypass      = apvts.getRawParameterValue("BYPASS")->load() > 0.5f;

    // Input level
    float inL = 0.f, inR = 0.f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getReadPointer(ch);
        float peak = 0.f;
        for (int i = 0; i < numSamples; ++i) peak = std::max(peak, std::abs(data[i]));
        if (ch == 0) inL = peak; else inR = peak;
    }
    inputLevelL.store(inL);
    inputLevelR.store(inR);

    bool isClipping = false;

    if (!bypass)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const float dry = data[i];
                float s = saturate(dry, mode, drive);

                const float dcAlpha = 0.9995f;
                float dcOut = s - dcPrevIn[ch] + dcAlpha * dcPrevOut[ch];
                dcPrevIn[ch]  = s;
                dcPrevOut[ch] = dcOut;

                data[i] = (dry * (1.f - mix) + dcOut * mix) * outputGain;
                if (std::abs(data[i]) >= 0.999f) isClipping = true;
            }
        }
    }

    // FFT — push mono mix
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.f;
        for (int ch = 0; ch < numChannels; ++ch) mono += buffer.getReadPointer(ch)[i];
        pushSampleToFifo(mono / numChannels);
    }

    // Output level
    float outL = 0.f, outR = 0.f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getReadPointer(ch);
        float peak = 0.f;
        for (int i = 0; i < numSamples; ++i) peak = std::max(peak, std::abs(data[i]));
        if (ch == 0) outL = peak; else outR = peak;
    }
    outputLevelL.store(outL);
    outputLevelR.store(outR);
    clipping.store(isClipping);
}

juce::AudioProcessorEditor* SaturaceAudioProcessor::createEditor()
{
    return new SaturaceAudioProcessorEditor(*this);
}

void SaturaceAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SaturaceAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

void SaturaceAudioProcessor::parameterChanged(const juce::String&, float) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SaturaceAudioProcessor();
}
