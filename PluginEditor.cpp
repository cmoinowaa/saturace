#include "PluginEditor.h"

// ─── Palette ────────────────────────────────────────────────────────────────
static const juce::Colour kBg        { 0xff0a0a12 };
static const juce::Colour kPanel     { 0xff0f0f1e };
static const juce::Colour kCard      { 0xff14142a };
static const juce::Colour kCyan      { 0xff00ffe0 };
static const juce::Colour kPurple    { 0xffaa44ff };
static const juce::Colour kPink      { 0xffff44aa };
static const juce::Colour kOrange    { 0xffff8800 };
static const juce::Colour kText      { 0xffd0d8ff };
static const juce::Colour kTextMuted { 0xff5566aa };
static const juce::Colour kLedGreen  { 0xff00ffaa };
static const juce::Colour kLedRed    { 0xffff2244 };

// ─── SpectrumAnalyser ────────────────────────────────────────────────────────
SpectrumAnalyser::SpectrumAnalyser(SaturaceAudioProcessor& p)
    : proc(p),
      fft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    smoothedScope.fill(0.f);
    startTimerHz(30);
}

void SpectrumAnalyser::timerCallback()
{
    if (proc.fftDataReady.load())
    {
        std::copy(proc.fftDataIn.begin(), proc.fftDataIn.end(), fftData.begin());
        std::fill(fftData.begin() + fftSize, fftData.end(), 0.f);
        proc.fftDataReady.store(false);

        window.multiplyWithWindowingTable(fftData.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        for (int i = 0; i < scopeSize; ++i)
        {
            float level = juce::jmap(juce::Decibels::gainToDecibels(fftData[(size_t)i], -100.f),
                                     -100.f, 0.f, 0.f, 1.f);
            level = juce::jlimit(0.f, 1.f, level);
            smoothedScope[(size_t)i] = smoothedScope[(size_t)i] * 0.8f + level * 0.2f;
        }

        repaint();
    }
}

void SpectrumAnalyser::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // Background
    g.setColour(juce::Colour(0xff080816));
    g.fillRoundedRectangle(b, 6.f);

    // Grid lines
    g.setColour(kTextMuted.withAlpha(0.2f));
    for (int i = 1; i < 4; ++i)
    {
        float y = b.getY() + b.getHeight() * i / 4.f;
        g.drawHorizontalLine((int)y, b.getX(), b.getX()  + b.getWidth());
    }

    // Spectrum fill
    juce::Path fillPath;
    const float w = b.getWidth();
    const float h = b.getHeight();

    fillPath.startNewSubPath(b.getX(), b.getY() + b.getHeight());

    for (int i = 0; i < scopeSize; ++i)
    {
        float x = b.getX() + (float)i / scopeSize * w;
        float y = b.getY() + b.getHeight() - smoothedScope[(size_t)i] * h;
        if (i == 0) fillPath.lineTo(x, y);
        else        fillPath.lineTo(x, y);
    }

    fillPath.lineTo(b.getX()  + b.getWidth(), b.getY() + b.getHeight());
    fillPath.closeSubPath();

    juce::ColourGradient grad(kCyan.withAlpha(0.5f),    b.getX(), b.getY(),
                               kPurple.withAlpha(0.2f), b.getX(), b.getY() + b.getHeight(), false);
    g.setGradientFill(grad);
    g.fillPath(fillPath);

    // Spectrum line
    juce::Path linePath;
    for (int i = 0; i < scopeSize; ++i)
    {
        float x = b.getX() + (float)i / scopeSize * w;
        float y = b.getY() + b.getHeight() - smoothedScope[(size_t)i] * h;
        if (i == 0) linePath.startNewSubPath(x, y);
        else        linePath.lineTo(x, y);
    }

    juce::ColourGradient lineGrad(kCyan,   b.getX(),     0.f,
                                   kPurple, b.getX() + b.getWidth(), 0.f, false);
    g.setGradientFill(lineGrad);
    g.strokePath(linePath, juce::PathStrokeType(1.5f));

    // Border glow
    g.setColour(kCyan.withAlpha(0.3f));
    g.drawRoundedRectangle(b, 6.f, 1.f);
}

// ─── FutureLAF ───────────────────────────────────────────────────────────────
FutureLAF::FutureLAF()
{
    setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff111128));
    setColour(juce::TextButton::buttonOnColourId,  kCyan.withAlpha(0.2f));
    setColour(juce::TextButton::textColourOffId,   kTextMuted);
    setColour(juce::TextButton::textColourOnId,    kCyan);
}

void FutureLAF::drawRotarySlider(juce::Graphics& g,
    int x, int y, int w, int h,
    float pos, float startAngle, float endAngle, juce::Slider&)
{
    const float cx  = x + w * 0.5f;
    const float cy  = y + h * 0.5f;
    const float r   = std::min(w, h) * 0.5f - 6.f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Outer glow ring
    for (int i = 4; i >= 1; --i)
    {
        float alpha = 0.04f * i;
        g.setColour(kCyan.withAlpha(alpha));
        g.drawEllipse(cx - r - i, cy - r - i, (r + i) * 2.f, (r + i) * 2.f, 1.f);
    }

    // Track ring bg
    {
        juce::Path track;
        track.addArc(cx - r * 0.78f, cy - r * 0.78f, r * 1.56f, r * 1.56f,
                     startAngle, endAngle, true);
        g.setColour(juce::Colour(0xff1a1a3a));
        g.strokePath(track, juce::PathStrokeType(4.f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    // Track fill (cyan → purple)
    {
        juce::Path fill;
        fill.addArc(cx - r * 0.78f, cy - r * 0.78f, r * 1.56f, r * 1.56f,
                    startAngle, angle, true);
        juce::ColourGradient grad(kCyan, cx - r, cy, kPurple, cx + r, cy, false);
        g.setGradientFill(grad);
        g.strokePath(fill, juce::PathStrokeType(4.f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }

    // Knob body
    {
        juce::ColourGradient grad(juce::Colour(0xff1e1e3e), cx - r * 0.4f, cy - r * 0.4f,
                                   juce::Colour(0xff0e0e1e), cx + r * 0.4f, cy + r * 0.4f, true);
        g.setGradientFill(grad);
        g.fillEllipse(cx - r, cy - r, r * 2.f, r * 2.f);
    }

    // Knob rim
    g.setColour(kCyan.withAlpha(0.4f));
    g.drawEllipse(cx - r, cy - r, r * 2.f, r * 2.f, 1.f);

    // Tick lines (like Softube style)
    for (int i = 0; i < 11; ++i)
    {
        float tickAngle = startAngle + (float)i / 10.f * (endAngle - startAngle);
        float inner = r * 0.85f, outer = r * 0.97f;
        float tx1 = cx + inner * std::sin(tickAngle);
        float ty1 = cy - inner * std::cos(tickAngle);
        float tx2 = cx + outer * std::sin(tickAngle);
        float ty2 = cy - outer * std::cos(tickAngle);
        float alpha = (i == 0 || i == 5 || i == 10) ? 0.7f : 0.25f;
        g.setColour(kCyan.withAlpha(alpha));
        g.drawLine(tx1, ty1, tx2, ty2, 1.f);
    }

    // Pointer dot
    {
        float px = cx + r * 0.62f * std::sin(angle);
        float py = cy - r * 0.62f * std::cos(angle);
        g.setColour(kCyan);
        g.fillEllipse(px - 4.f, py - 4.f, 8.f, 8.f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(px - 2.f, py - 2.f, 4.f, 4.f);

        // Pointer glow
        g.setColour(kCyan.withAlpha(0.4f));
        g.fillEllipse(px - 6.f, py - 6.f, 12.f, 12.f);
    }

    // Inner highlight
    g.setColour(juce::Colours::white.withAlpha(0.04f));
    g.fillEllipse(cx - r * 0.5f, cy - r * 0.65f, r * 0.7f, r * 0.4f);
}

void FutureLAF::drawLinearSlider(juce::Graphics& g,
    int x, int y, int w, int h,
    float pos, float, float,
    juce::Slider::SliderStyle, juce::Slider&)
{
    const float trackH = 3.f;
    const float trackY = y + h * 0.5f - trackH * 0.5f;

    // Track bg
    g.setColour(juce::Colour(0xff1a1a3a));
    g.fillRoundedRectangle((float)x, trackY, (float)w, trackH, 2.f);

    // Track fill
    juce::ColourGradient grad(kCyan, (float)x, 0.f, kPurple, pos, 0.f, false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle((float)x, trackY, pos - x, trackH, 2.f);

    // Thumb
    g.setColour(kCard);
    g.fillEllipse(pos - 8.f, y + h * 0.5f - 8.f, 16.f, 16.f);
    g.setColour(kCyan);
    g.drawEllipse(pos - 8.f, y + h * 0.5f - 8.f, 16.f, 16.f, 1.5f);
    g.setColour(kCyan.withAlpha(0.8f));
    g.fillEllipse(pos - 3.f, y + h * 0.5f - 3.f, 6.f, 6.f);
}

void FutureLAF::drawButtonBackground(juce::Graphics& g, juce::Button& btn,
    const juce::Colour&, bool hover, bool)
{
    auto b = btn.getLocalBounds().toFloat().reduced(0.5f);
    bool on = btn.getToggleState();

    if (on)
    {
        g.setColour(kCyan.withAlpha(0.15f));
        g.fillRoundedRectangle(b, 4.f);
        g.setColour(kCyan.withAlpha(0.8f));
        g.drawRoundedRectangle(b, 4.f, 1.f);
    }
    else
    {
        g.setColour(hover ? juce::Colour(0xff1a1a3a) : juce::Colour(0xff111128));
        g.fillRoundedRectangle(b, 4.f);
        g.setColour(kTextMuted.withAlpha(0.4f));
        g.drawRoundedRectangle(b, 4.f, 1.f);
    }
}

void FutureLAF::drawButtonText(juce::Graphics& g, juce::TextButton& btn,
    bool, bool)
{
    bool on = btn.getToggleState();
    g.setColour(on ? kCyan : kTextMuted);
    g.setFont(juce::FontOptions(9.f).withStyle("Bold"));
    g.drawText(btn.getButtonText(), btn.getLocalBounds(),
               juce::Justification::centred, false);
}

// ─── Editor ──────────────────────────────────────────────────────────────────
SaturaceAudioProcessorEditor::SaturaceAudioProcessorEditor(SaturaceAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), spectrum(p)
{
    setLookAndFeel(&laf);
    setSize(560, 380);
    setResizable(false, false);

    // Drive knob
    driveKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    driveKnob.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(driveKnob);
    driveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "DRIVE", driveKnob);

    // Output knob
    outputKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    outputKnob.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(outputKnob);
    outputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "OUTPUT", outputKnob);

    // Mix knob
    mixKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    mixKnob.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(mixKnob);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "MIX", mixKnob);

    // Mode buttons
    const juce::String modeNames[] = { "KEEP HI", "NEUTRAL", "KEEP LO", "TAPE", "TUBE", "CLIP" };
    for (int i = 0; i < 6; ++i)
    {
        modeButtons[i].setButtonText(modeNames[i]);
        modeButtons[i].setClickingTogglesState(false);
        modeButtons[i].onClick = [this, i]()
        {
            audioProcessor.apvts.getParameter("MODE")->setValueNotifyingHost(
                audioProcessor.apvts.getParameter("MODE")->convertTo0to1((float)i));
        };
        addAndMakeVisible(modeButtons[i]);
    }

    // Bypass
    bypassBtn.setClickingTogglesState(true);
    addAndMakeVisible(bypassBtn);
    bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "BYPASS", bypassBtn);

    // Labels
    auto setupLabel = [&](juce::Label& lbl, const juce::String& text)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(juce::FontOptions(9.f).withStyle("Bold"));
        lbl.setColour(juce::Label::textColourId, kTextMuted);
        lbl.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(lbl);
    };
    setupLabel(driveLabel,  "SATURATION");
    setupLabel(outputLabel, "OUTPUT");
    setupLabel(mixLabel,    "DRY / WET");

    addAndMakeVisible(spectrum);

    startTimerHz(30);
}

SaturaceAudioProcessorEditor::~SaturaceAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void SaturaceAudioProcessorEditor::timerCallback()
{
    inputLevelL  = audioProcessor.inputLevelL .load();
    inputLevelR  = audioProcessor.inputLevelR .load();
    outputLevelL = audioProcessor.outputLevelL.load();
    outputLevelR = audioProcessor.outputLevelR.load();
    clipState    = audioProcessor.clipping    .load();

    const int mode = (int)audioProcessor.apvts.getRawParameterValue("MODE")->load();
    for (int i = 0; i < 6; ++i)
        modeButtons[i].setToggleState(mode == i, juce::dontSendNotification);

    glowPhase += 0.04f;
    if (glowPhase > juce::MathConstants<float>::twoPi) glowPhase -= juce::MathConstants<float>::twoPi;

    repaint();
}

void SaturaceAudioProcessorEditor::drawLevelMeter(juce::Graphics& g,
    juce::Rectangle<float> b, float levelL, float levelR, bool clip)
{
    const float barW = (b.getWidth() - 3.f) * 0.5f;

    for (int ch = 0; ch < 2; ++ch)
    {
        float level = ch == 0 ? levelL : levelR;
        float dB    = juce::Decibels::gainToDecibels(level, -60.f);
        float norm  = juce::jlimit(0.f, 1.f, juce::jmap(dB, -60.f, 0.f, 0.f, 1.f));

        float bx = b.getX() + ch * (barW + 3.f);
        float by = b.getY();
        float bh = b.getHeight();

        // BG
        g.setColour(juce::Colour(0xff080816));
        g.fillRoundedRectangle(bx, by, barW, bh, 2.f);

        // Fill
        float fillH = bh * norm;
        juce::ColourGradient grad(kLedRed,   bx, by,
                                   kLedGreen, bx, by + bh, false);
        grad.addColour(0.7, juce::Colour(0xffffff00));
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bx, by + bh - fillH, barW, fillH, 2.f);

        if (clip)
        {
            g.setColour(kLedRed.withAlpha(0.9f));
            g.fillRoundedRectangle(bx, by, barW, 4.f, 1.f);
        }
    }
}

void SaturaceAudioProcessorEditor::drawHexGrid(juce::Graphics& g)
{
    const float hexR = 20.f;
    const float hexW = hexR * std::sqrt(3.f);
    const float hexH = hexR * 2.f;

    g.setColour(kCyan.withAlpha(0.025f));

    for (float row = -hexH; row < getHeight() + hexH; row += hexH * 0.75f)
    {
        float offset = (int)(row / (hexH * 0.75f)) % 2 == 0 ? 0.f : hexW * 0.5f;
        for (float col = -hexW + offset; col < getWidth() + hexW; col += hexW)
        {
            juce::Path hex;
            for (int i = 0; i < 6; ++i)
            {
                float angle = juce::MathConstants<float>::pi / 3.f * i - juce::MathConstants<float>::pi / 6.f;
                float px = col + hexR * std::cos(angle);
                float py = row + hexR * std::sin(angle);
                if (i == 0) hex.startNewSubPath(px, py);
                else        hex.lineTo(px, py);
            }
            hex.closeSubPath();
            g.strokePath(hex, juce::PathStrokeType(0.5f));
        }
    }
}

void SaturaceAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto W = (float)getWidth();
    const auto H = (float)getHeight();

    // Background
    {
        juce::ColourGradient grad(juce::Colour(0xff0c0c1e), 0.f, 0.f,
                                   juce::Colour(0xff060610), 0.f, H, false);
        g.setGradientFill(grad);
        g.fillAll();
    }

    // Hex grid background
    drawHexGrid(g);

    // Animated multi-colour corner glows
    {
        float phase2 = glowPhase + juce::MathConstants<float>::twoPi / 3.f;
        float phase3 = glowPhase + juce::MathConstants<float>::twoPi * 2.f / 3.f;
        juce::Colour col1((uint8_t)(80 + 60 * std::sin(glowPhase)), (uint8_t)220, (uint8_t)255, (uint8_t)255);
        juce::Colour col2((uint8_t)180, (uint8_t)(60 + 60 * std::sin(phase2)), (uint8_t)255, (uint8_t)255);
        juce::Colour col3((uint8_t)255, (uint8_t)(60 + 60 * std::sin(phase3)), (uint8_t)180, (uint8_t)255);

        juce::ColourGradient g1(col1.withAlpha(0.07f), 0.f, H,
                                 juce::Colours::transparentBlack, W * 0.5f, H * 0.4f, true);
        g.setGradientFill(g1); g.fillAll();

        juce::ColourGradient g2(col2.withAlpha(0.06f), W, 0.f,
                                 juce::Colours::transparentBlack, W * 0.4f, H * 0.6f, true);
        g.setGradientFill(g2); g.fillAll();

        juce::ColourGradient g3(col3.withAlpha(0.04f), W, H,
                                 juce::Colours::transparentBlack, W * 0.4f, H * 0.4f, true);
        g.setGradientFill(g3); g.fillAll();
    }

    // Scanlines overlay
    {
        for (int y = 0; y < (int)H; y += 3)
        {
            g.setColour(juce::Colours::black.withAlpha(0.08f));
            g.fillRect(0.f, (float)y, W, 1.f);
        }
    }

    // Top header
    {
        juce::ColourGradient grad(juce::Colour(0xff111130), 0.f, 0.f,
                                   juce::Colour(0xff0a0a20), 0.f, 48.f, false);
        g.setGradientFill(grad);
        g.fillRect(0.f, 0.f, W, 52.f);

        // Header bottom line glow
        for (int i = 0; i < 3; ++i)
        {
            g.setColour(kCyan.withAlpha(0.15f - i * 0.04f));
            g.fillRect(0.f, 51.f + i, W, 1.f);
        }
    }

    // Brand name
    {
        // SATURACE — big white bold title
        g.setFont(juce::FontOptions(26.f).withStyle("Bold"));
        g.setColour(juce::Colours::white);
        g.drawText("SATURACE", 18, 5, 200, 36, juce::Justification::left);

        // "by Nowaa" — RGB animated gradient text effect via overlay
        float r1 = glowPhase;
        float r2 = glowPhase + juce::MathConstants<float>::twoPi / 3.f;
        float r3 = glowPhase + juce::MathConstants<float>::twoPi * 2.f / 3.f;
        juce::Colour rgbCol(
            (uint8_t)(127 + 127 * std::sin(r1)),
            (uint8_t)(127 + 127 * std::sin(r2)),
            (uint8_t)(127 + 127 * std::sin(r3)),
            (uint8_t)255
        );

        // Glow shadow behind "by Nowaa"
        g.setColour(rgbCol.withAlpha(0.25f));
        g.setFont(juce::FontOptions(15.f).withStyle("Bold"));
        g.drawText("by Nowaa", 17, 27, 130, 18, juce::Justification::left);
        g.drawText("by Nowaa", 19, 27, 130, 18, juce::Justification::left);

        // Main "by Nowaa" text
        g.setColour(rgbCol);
        g.setFont(juce::FontOptions(14.f).withStyle("Bold"));
        g.drawText("by Nowaa", 18, 27, 130, 18, juce::Justification::left);

        // Animated RGB dot (right side)
        float dotGlow = 0.5f + 0.5f * std::sin(glowPhase * 2.f);
        g.setColour(rgbCol.withAlpha(dotGlow));
        g.fillEllipse(W - 24.f, 16.f, 12.f, 12.f);
        g.setColour(rgbCol.withAlpha(0.25f * dotGlow));
        g.fillEllipse(W - 30.f, 10.f, 24.f, 24.f);
        g.setColour(rgbCol.withAlpha(0.08f * dotGlow));
        g.fillEllipse(W - 36.f, 4.f, 36.f, 36.f);

        // Version tag
        g.setFont(juce::FontOptions(8.f));
        g.setColour(kTextMuted.withAlpha(0.5f));
        g.drawText("v2.0", (int)W - 58, 34, 36, 10, juce::Justification::centred);
    }

    // Futuristic corner accents
    {
        float cornerLen = 16.f;
        float cornerW   = 1.5f;
        g.setColour(kCyan.withAlpha(0.7f));
        // Top-left
        g.fillRect(2.f, 2.f, cornerLen, cornerW);
        g.fillRect(2.f, 2.f, cornerW, cornerLen);
        // Top-right
        g.fillRect(W - cornerLen - 2.f, 2.f, cornerLen, cornerW);
        g.fillRect(W - cornerW - 2.f, 2.f, cornerW, cornerLen);
        // Bottom-left
        g.fillRect(2.f, H - cornerW - 2.f, cornerLen, cornerW);
        g.fillRect(2.f, H - cornerLen - 2.f, cornerW, cornerLen);
        // Bottom-right
        g.fillRect(W - cornerLen - 2.f, H - cornerW - 2.f, cornerLen, cornerW);
        g.fillRect(W - cornerW - 2.f, H - cornerLen - 2.f, cornerW, cornerLen);
    }

    // Left panel border
    g.setColour(kCyan.withAlpha(0.05f));
    g.fillRect(0.f, 52.f, 340.f, H - 52.f);

    // Divider
    g.setColour(kCyan.withAlpha(0.15f));
    g.fillRect(340.f, 56.f, 1.f, H - 66.f);

    // Section labels
    g.setFont(juce::FontOptions(8.f).withStyle("Bold"));
    g.setColour(kTextMuted);
    g.drawText("SATURATION MODE", 350, 56, 180, 12, juce::Justification::left);
    g.drawText("SPECTRUM", 350, 160, 180, 12, juce::Justification::left);

    // Meter labels
    g.setFont(juce::FontOptions(8.f).withStyle("Bold"));
    g.setColour(kTextMuted);
    g.drawText("IN",  (int)W - 50, 52, 20, 10, juce::Justification::centred);
    g.drawText("OUT", (int)W - 28, 52, 22, 10, juce::Justification::centred);

    // Input meters
    drawLevelMeter(g, { W - 52.f, 64.f, 22.f, H - 80.f }, inputLevelL,  inputLevelR,  false);
    drawLevelMeter(g, { W - 28.f, 64.f, 22.f, H - 80.f }, outputLevelL, outputLevelR, clipState);

    if (clipState)
    {
        g.setColour(kLedRed);
        g.setFont(juce::FontOptions(8.f).withStyle("Bold"));
        g.drawText("CLIP", (int)W - 52, (int)H - 14, 44, 10, juce::Justification::centred);
    }

    // Bypass indicator
    bool bypassed = audioProcessor.apvts.getRawParameterValue("BYPASS")->load() > 0.5f;
    if (bypassed)
    {
        g.setColour(kOrange.withAlpha(0.9f));
        g.setFont(juce::FontOptions(9.f).withStyle("Bold"));
        g.drawText("BYPASSED", 20, 54, 100, 12, juce::Justification::left);
    }
}

void SaturaceAudioProcessorEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    // Big drive knob — left section center
    const int knobSize = 150;
    const int knobX    = (240 - knobSize) / 2 + 10;
    const int knobY    = (H - 48 - knobSize) / 2 + 48 - 10;
    driveKnob.setBounds(knobX, knobY, knobSize, knobSize);
    driveLabel.setBounds(knobX, knobY + knobSize + 2, knobSize, 13);

    // Output + Mix knobs — side by side right of big knob
    const int smallKnob = 64;
    const int smallY = knobY + 20;
    outputKnob.setBounds(248, smallY, smallKnob, smallKnob);
    outputLabel.setBounds(248, smallY + smallKnob + 2, smallKnob, 12);

    mixKnob.setBounds(248, smallY + smallKnob + 22, smallKnob, smallKnob);
    mixLabel.setBounds(248, smallY + smallKnob * 2 + 24, smallKnob, 12);

    // Bypass button
    bypassBtn.setBounds(254, H - 44, 52, 24);

    // Mode buttons — right panel (2 rows of 3)
    const int btnW = 68, btnH = 26, btnGapX = 4, btnGapY = 4;
    const int btnStartX = 348, btnStartY = 72;
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 3;
        int row = i / 3;
        modeButtons[i].setBounds(btnStartX + col * (btnW + btnGapX),
                                  btnStartY + row * (btnH + btnGapY),
                                  btnW, btnH);
    }

    // Spectrum analyser
    spectrum.setBounds(348, 175, W - 348 - 58, H - 190);
}
