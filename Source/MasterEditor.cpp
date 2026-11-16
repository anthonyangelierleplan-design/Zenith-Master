#include "MasterEditor.h"

using namespace tn;
using namespace ui;

// ================================================================ petits composants
void Plate::paint (juce::Graphics& g)
{
    drawPlate (g, getLocalBounds().toFloat());
    g.setColour (look::print.withAlpha (0.95f));
    g.setFont (font (11.f));
    g.drawText (title, 12, 5, getWidth() - 24, 16, juce::Justification::centredLeft, false);
    g.setColour (look::print.withAlpha (0.35f));
    g.fillRect (12, 24, getWidth() - 24, 1);
}

void Plate::paintOverChildren (juce::Graphics& g)
{
    g.setColour (look::print.withAlpha (0.85f));
    g.setFont (font (9.f));
    for (const auto& l : labels) g.drawText (l.first, l.second, juce::Justification::centred, false);
    drawSpecks (g, getLocalBounds());
}

void LedBar::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float capH = caption.isNotEmpty() ? 12.f : 0.f;
    const auto bar = b.withTrimmedBottom (capH);
    g.setColour (juce::Colour (0xff060607));
    g.fillRoundedRectangle (bar, 3.f);
    g.setColour (juce::Colour (0xff6a6a70).withAlpha (0.7f));
    g.drawRoundedRectangle (bar, 3.f, 1.f);
    const int n = juce::jmax (8, (int) (bar.getHeight() / 6.f));
    const float segH = (bar.getHeight() - 6.f) / (float) n;
    const float frac = juce::jlimit (0.f, 1.f, (value - lo_) / (hi_ - lo_));
    for (int i = 0; i < n; ++i)
    {
        const float t = (float) i / (float) (n - 1);                         // 0 = extremite de depart
        const float y = mode == Reduction ? bar.getY() + 3.f + (float) i * segH : bar.getBottom() - 3.f - (float) (i + 1) * segH;
        const bool lit = (float) i / (float) n < frac;
        juce::Colour c;
        if (mode == Level)          c = t < 0.65f ? juce::Colour (0xff4fd067) : (t < 0.88f ? juce::Colour (0xffe3b23a) : juce::Colour (0xffe0362a));
        else if (mode == Reduction) c = juce::Colour (0xffff7a1a);
        else                        c = juce::Colour (0xff4fd0b8);
        g.setColour (lit ? c : c.darker (3.2f).withAlpha (0.55f));
        g.fillRect (bar.getX() + 3.f, y + 0.5f, bar.getWidth() - 6.f, juce::jmax (1.f, segH - 1.5f));
    }
    if (capH > 0.f)
    {
        g.setColour (look::print.withAlpha (0.85f));
        g.setFont (font (8.5f));
        g.drawText (caption, b.withTrimmedTop (b.getHeight() - capH).expanded (6.f, 0.f), juce::Justification::centred, false);
    }
}

void Lcd::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (1.f);
    g.setColour (juce::Colour (0xff070605));
    g.fillRoundedRectangle (b, 3.f);
    g.setColour (juce::Colour (0xff6a6a70).withAlpha (0.7f));
    g.drawRoundedRectangle (b, 3.f, 1.f);
    g.setColour (look::print.withAlpha (0.7f));
    g.setFont (font (8.f));
    g.drawText (label, b.withTrimmedTop (2.f).withHeight (10.f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xffffa63a));
    g.setFont (juce::Font (juce::FontOptions (large ? 22.f : 14.f, juce::Font::bold)));
    g.drawText (text, b.withTrimmedTop (10.f), juce::Justification::centred, false);
}

// ================================================================ contenu (taille de conception fixe)
ZenithContent::ZenithContent (ZenithProcessor& p) : proc (p)
{
    setLookAndFeel (&laf);
    setSize (designW, designH);
    buildPanelTex();
    for (auto* pl : { &inPlate, &ottPlate, &macroPlate, &eqPlate, &cmpPlate, &limPlate, &outPlate }) addAndMakeVisible (pl);

    // ---- en-tete
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn }) addAndMakeVisible (*b);
    addAndMakeVisible (nameBtn);
    prevBtn.setBounds (designW - 400, 40, 30, 28);
    nameBtn.setBounds (designW - 364, 40, 220, 28);
    nextBtn.setBounds (designW - 140, 40, 30, 28);
    saveBtn.setBounds (designW - 104, 40, 70, 28);
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    nameBtn.onClick = [this] { presetMenu(); };
    saveBtn.onClick = [this] { saveDialog(); };
    nameBtn.setButtonText (proc.presetName);
    {
        auto* by = toggle (*this, p_bypass, "BYPASS");  by->setBounds (designW - 400, 76, 110, 24);
        auto* mt = toggle (*this, p_match, "LEVEL MATCH"); mt->setBounds (designW - 284, 76, 130, 24);
    }

    // ---- INPUT
    inPlate.setBounds (10, 118, 130, 350);
    knob (inPlate, p_in, "INPUT", "-12", "+12")->setBounds (14, 32, 102, 100);
    inBar = new LedBar (LedBar::Level, -60.f, 0.f, "dBFS"); owned.emplace_back (inBar); inPlate.addAndMakeVisible (inBar);
    inBar->setBounds (44, 140, 42, 160);
    lcdInS = new Lcd ("LUFS  SHORT"); owned.emplace_back (lcdInS); inPlate.addAndMakeVisible (lcdInS);
    lcdInS->setBounds (14, 312, 102, 40);

    // ---- OTT
    ottPlate.setBounds (150, 118, 860, 350);
    {
        auto* on = toggle (ottPlate, p_ottOn, "ON");  on->setBounds (14, 32, 66, 24);
        auto* ms = toggle (ottPlate, p_ottMS, "M/S"); ms->setBounds (14, 62, 66, 24);
        const int kw = 78, y0 = 30, h = 98; int x = 92;
        knob (ottPlate, p_ottAtk, "ATTACK", "FAST", "SLOW")->setBounds (x, y0, kw, h); x += kw + 6;
        knob (ottPlate, p_ottRel, "RELEASE", "FAST", "SLOW")->setBounds (x, y0, kw, h); x += kw + 6;
        knob (ottPlate, p_ottDepth, "DEPTH", "0", "100")->setBounds (x, y0, kw, h); x += kw + 22;
        knob (ottPlate, p_xo1, "LOW / MID", "40", "400")->setBounds (x, y0, kw, h); x += kw + 6;
        knob (ottPlate, p_xo2, "MID / HIGH", "1k", "8k")->setBounds (x, y0, kw, h);
    }
    {
        static const char* names[3] { "LOW", "MID", "HIGH" };
        static const P ids[3][5] { { p_tL, p_fL, p_gL, p_dL, p_uL }, { p_tM, p_fM, p_gM, p_dM, p_uM }, { p_tH, p_fH, p_gH, p_dH, p_uH } };
        const int colW = 276, x0 = 12, y0 = 134;
        for (int b = 0; b < 3; ++b)
        {
            const int cx = x0 + b * (colW + 8);
            ottPlate.labels.push_back ({ juce::String (names[b]) + (b == 0 ? "  BAND" : "  BAND"), { cx + 30, y0 - 4, colW - 30, 14 } });
            bandDown[b] = new LedBar (LedBar::Reduction, 0.f, 18.f, "DOWN"); owned.emplace_back (bandDown[b]); ottPlate.addAndMakeVisible (bandDown[b]);
            bandUp[b]   = new LedBar (LedBar::Boost, 0.f, 24.f, "UP");     owned.emplace_back (bandUp[b]);   ottPlate.addAndMakeVisible (bandUp[b]);
            bandDown[b]->setBounds (cx, y0 + 12, 22, 200);
            bandUp[b]->setBounds (cx + 26, y0 + 12, 22, 200);
            const int kx = cx + 54, kw = 74, kh = 100;
            knob (ottPlate, ids[b][0], "THRESH", "-50", "-6")->setBounds (kx, y0 + 12, kw, kh);
            knob (ottPlate, ids[b][1], "FLOOR", "-70", "-20")->setBounds (kx + kw, y0 + 12, kw, kh);
            knob (ottPlate, ids[b][2], "GAIN", "-12", "+12")->setBounds (kx + 2 * kw, y0 + 12, kw, kh);
            knob (ottPlate, ids[b][3], "DOWN", "0", "10")->setBounds (kx, y0 + 12 + kh, kw, kh);
            knob (ottPlate, ids[b][4], "UP", "0", "10")->setBounds (kx + kw, y0 + 12 + kh, kw, kh);
            lcdBand[b] = new Lcd ("DOWN / UP dB"); owned.emplace_back (lcdBand[b]); ottPlate.addAndMakeVisible (lcdBand[b]);
            lcdBand[b]->setBounds (kx + 2 * kw + 4, y0 + 12 + kh + 22, kw - 8, 46);
        }
    }

    // ---- INTENSITE + MACROS
    macroPlate.setBounds (1020, 118, 370, 350);
    {
        auto* k = knob (macroPlate, p_intensity, "INTENSITY", "0", "MAX");
        k->setBounds (110, 30, 150, 160);
        k->slider.getProperties().set ("big", true);
        const int mw = 120, my = 200;
        knob (macroPlate, p_mDyn, "DYNAMICS", "SQUEEZE", "PUNCH", 0.5)->setBounds (6, my, mw, 142);
        knob (macroPlate, p_mBri, "BRILLIANCE", "DARK", "BRIGHT", 0.5)->setBounds (125, my, mw, 142);
        knob (macroPlate, p_mSq, "SNAPPY / SQUASHY", "SNAPPY", "SQUASHY", 0.5)->setBounds (244, my, mw, 142);
    }

    // ---- EQ
    eqPlate.setBounds (10, 478, 330, 372);
    {
        const int kw = 100, kh = 104, x0 = 12, y0 = 30;
        knob (eqPlate, p_eqHpf, "HPF", "20", "250")->setBounds (x0, y0, kw, kh);
        knob (eqPlate, p_lsG, "LOW GAIN", "-12", "+12")->setBounds (x0 + kw, y0, kw, kh);
        knob (eqPlate, p_lsF, "LOW FREQ", "40", "400")->setBounds (x0 + 2 * kw, y0, kw, kh);
        knob (eqPlate, p_mdG, "MID GAIN", "-12", "+12")->setBounds (x0, y0 + kh, kw, kh);
        knob (eqPlate, p_mdF, "MID FREQ", "200", "8k")->setBounds (x0 + kw, y0 + kh, kw, kh);
        knob (eqPlate, p_mdQ, "MID Q", "WIDE", "NARROW")->setBounds (x0 + 2 * kw, y0 + kh, kw, kh);
        knob (eqPlate, p_hsG, "HIGH GAIN", "-12", "+12")->setBounds (x0, y0 + 2 * kh, kw, kh);
        knob (eqPlate, p_hsF, "HIGH FREQ", "2k", "14k")->setBounds (x0 + kw, y0 + 2 * kh, kw, kh);
        toggle (eqPlate, p_eqOn, "EQ ON")->setBounds (x0 + 2 * kw + 6, y0 + 2 * kh + 36, 88, 26);
    }

    // ---- COMPRESSEUR : 4 modeles (CLEAN, VARI-MU type 660, FET type 1176, DIODE type 33609)
    cmpPlate.setBounds (350, 478, 410, 372);
    {
        cmpPlate.addAndMakeVisible (vu);
        vu.setBounds (14, 30, 382, 96);
        selector (cmpPlate, p_cmpModel)->setBounds (12, 132, 280, 30);
        toggle (cmpPlate, p_cmpOn, "COMP ON")->setBounds (300, 134, 96, 26);
        toggle (cmpPlate, p_cmpMS, "MID / SIDE")->setBounds (300, 334, 96, 26);
        const int kw = 76, kh = 96, y1 = 168, y2 = 266;
        auto add = [this] (int m, juce::Component* c) { modelGroup[m].push_back (c); };
        // CLEAN
        add (0, knob (cmpPlate, p_cmpThr, "THRESH", "-40", "0"));          modelGroup[0].back()->setBounds (14, y1, kw, kh);
        add (0, knob (cmpPlate, p_cmpRat, "RATIO", "1.5", "20"));          modelGroup[0].back()->setBounds (14 + kw, y1, kw, kh);
        add (0, knob (cmpPlate, p_cmpAtk, "ATTACK", "0.1", "100"));        modelGroup[0].back()->setBounds (14 + 2 * kw, y1, kw, kh);
        add (0, knob (cmpPlate, p_cmpRel, "RELEASE", "20", "1200"));       modelGroup[0].back()->setBounds (14 + 3 * kw, y1, kw, kh);
        add (0, knob (cmpPlate, p_cmpKnee, "KNEE", "HARD", "SOFT"));       modelGroup[0].back()->setBounds (14 + 4 * kw, y1, kw, kh);
        add (0, knob (cmpPlate, p_cmpSc, "SC HPF", "20", "400"));          modelGroup[0].back()->setBounds (14, y2, kw, kh);
        add (0, knob (cmpPlate, p_cmpMk, "MAKEUP", "-6", "+12"));          modelGroup[0].back()->setBounds (14 + kw, y2, kw, kh);
        add (0, toggle (cmpPlate, p_cmpAuto, "AUTO GAIN"));                modelGroup[0].back()->setBounds (300, 302, 96, 26);
        // VARI-MU (tubes 6386, type 660)
        add (1, knob (cmpPlate, p_vmIn, "INPUT", "-12", "+12"));           modelGroup[1].back()->setBounds (14, y1, kw, kh);
        add (1, knob (cmpPlate, p_vmThr, "THRESHOLD", "0", "10"));         modelGroup[1].back()->setBounds (14 + kw, y1, kw, kh);
        add (1, knob (cmpPlate, p_vmBal, "BALANCE", "-", "+"));            modelGroup[1].back()->setBounds (14 + 2 * kw, y1, kw, kh);
        add (1, knob (cmpPlate, p_cmpMk, "OUTPUT", "-6", "+12"));          modelGroup[1].back()->setBounds (14 + 3 * kw, y1, kw, kh);
        add (1, caption (cmpPlate, "TIME CONSTANT", { 14, y2 + 4, 276, 12 }));
        add (1, selector (cmpPlate, p_vmTc));                              modelGroup[1].back()->setBounds (12, y2 + 18, 280, 32);
        add (1, caption (cmpPlate, "1-4 : FIXED   /   5-6 : PROGRAM DEPENDENT", { 14, y2 + 56, 276, 12 }));
        // FET (type 1176LN)
        add (2, knob (cmpPlate, p_fetIn, "INPUT", "-12", "+24"));          modelGroup[2].back()->setBounds (14, y1, kw, kh);
        add (2, knob (cmpPlate, p_cmpMk, "OUTPUT", "-6", "+12"));          modelGroup[2].back()->setBounds (14 + kw, y1, kw, kh);
        add (2, knob (cmpPlate, p_fetAtk, "ATTACK", "SLOW", "FAST"));      modelGroup[2].back()->setBounds (14 + 2 * kw, y1, kw, kh);
        add (2, knob (cmpPlate, p_fetRel, "RELEASE", "SLOW", "FAST"));     modelGroup[2].back()->setBounds (14 + 3 * kw, y1, kw, kh);
        add (2, caption (cmpPlate, "RATIO", { 14, y2 + 4, 276, 12 }));
        add (2, selector (cmpPlate, p_fetRat));                            modelGroup[2].back()->setBounds (12, y2 + 18, 280, 32);
        add (2, caption (cmpPlate, "ALL = ALL BUTTONS IN", { 14, y2 + 56, 276, 12 }));
        // DIODE (type 33609)
        add (3, knob (cmpPlate, p_dcThr, "THRESH dBu", "-20", "+10"));     modelGroup[3].back()->setBounds (14, y1, kw, kh);
        add (3, knob (cmpPlate, p_cmpMk, "GAIN", "-6", "+12"));            modelGroup[3].back()->setBounds (14 + kw, y1, kw, kh);
        add (3, caption (cmpPlate, "RATIO", { 14, y2 - 2, 276, 12 }));
        add (3, selector (cmpPlate, p_dcRat));                             modelGroup[3].back()->setBounds (12, y2 + 10, 280, 30);
        add (3, caption (cmpPlate, "RECOVERY ms", { 14, y2 + 44, 276, 12 }));
        add (3, selector (cmpPlate, p_dcRec));                             modelGroup[3].back()->setBounds (12, y2 + 56, 280, 30);
        showModel ((int) proc.apvts.getRawParameterValue ("cmpModel")->load());
    }

    // ---- LIMITEUR : limiteur a diodes (type 33609, colore) puis limiteur a anticipation (securite)
    limPlate.setBounds (770, 478, 300, 372);
    {
        const int kw = 100, kh = 96;
        knob (limPlate, p_limPush, "PUSH", "0", "+18")->setBounds (12, 28, kw, kh);
        knob (limPlate, p_limCeil, "CEILING", "-6", "0")->setBounds (12 + kw, 28, kw, kh);
        knob (limPlate, p_limRel, "RELEASE", "FAST", "SLOW")->setBounds (12, 124, kw, kh);
        knob (limPlate, p_limClip, "CLIP", "OFF", "MAX")->setBounds (12 + kw, 124, kw, kh);
        toggle (limPlate, p_limOn, "LIMITER ON")->setBounds (14, 224, 198, 22);
        limPlate.labels.push_back ({ "DIODE LIMITER  (BEFORE BRICKWALL)", { 10, 252, 236, 12 } });
        toggle (limPlate, p_dlOn, "DIODE ON")->setBounds (14, 266, 96, 22);
        toggle (limPlate, p_dlFast, "FAST ATK")->setBounds (116, 266, 96, 22);
        knob (limPlate, p_dlThr, "LEVEL dBu", "+4", "+15")->setBounds (8, 292, 92, 78);
        limPlate.labels.push_back ({ "RECOVERY ms", { 104, 298, 140, 12 } });
        selector (limPlate, p_dlRec)->setBounds (102, 312, 144, 30);
        limBar = new LedBar (LedBar::Reduction, 0.f, 12.f, "GR"); owned.emplace_back (limBar); limPlate.addAndMakeVisible (limBar);
        limBar->setBounds (254, 32, 36, 326);
    }

    // ---- SORTIE + LOUDNESS
    outPlate.setBounds (1080, 478, 310, 372);
    {
        knob (outPlate, p_out, "OUTPUT", "-12", "+6")->setBounds (14, 32, 110, 126);
        outBar = new LedBar (LedBar::Level, -60.f, 0.f, "dBFS"); owned.emplace_back (outBar); outPlate.addAndMakeVisible (outBar);
        outBar->setBounds (254, 36, 40, 240);
        lcdOutM = new Lcd ("MOMENTARY LUFS"); lcdOutS = new Lcd ("SHORT-TERM LUFS"); lcdOutI = new Lcd ("INTEGRATED LUFS", true);
        lcdMatch = new Lcd ("MATCH TRIM dB");
        for (auto* l : { lcdOutM, lcdOutS, lcdOutI, lcdMatch }) { owned.emplace_back (l); outPlate.addAndMakeVisible (l); }
        lcdOutI->setBounds (14, 168, 222, 52);
        lcdOutM->setBounds (14, 226, 108, 44);
        lcdOutS->setBounds (128, 226, 108, 44);
        lcdMatch->setBounds (14, 278, 108, 44);
        outPlate.addAndMakeVisible (resetLufs);
        resetLufs.setBounds (128, 290, 108, 26);
        resetLufs.onClick = [this] { proc.master.resetLufsFlag.store (true); };
    }

    nameBtn.setButtonText (proc.presetName);
    startTimerHz (30);
}

ZenithContent::~ZenithContent() { stopTimer(); setLookAndFeel (nullptr); }

KnobUnit* ZenithContent::knob (juce::Component& parent, P id, const juce::String& text, const char* lo, const char* hi, double detent)
{
    auto k = std::make_unique<KnobUnit> (proc.apvts, paramDefs()[id].id, text, lo, hi);
    if (detent > -1e29) k->slider.detent = detent;
    parent.addAndMakeVisible (*k);
    byId[id] = k.get();
    knobs.push_back (std::move (k));
    return knobs.back().get();
}

LedToggle* ZenithContent::toggle (juce::Component& parent, P id, const juce::String& text)
{
    auto t = std::make_unique<LedToggle>();
    t->setButtonText (text);
    parent.addAndMakeVisible (*t);
    att.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, paramDefs()[id].id, *t));
    toggles.push_back (std::move (t));
    return toggles.back().get();
}

SegmentSelector* ZenithContent::selector (juce::Component& parent, P id)
{
    juce::StringArray names;
    for (auto* n : choiceNames (id)) names.add (n);
    auto* sel = new SegmentSelector (proc.apvts, paramDefs()[id].id, names);
    owned.emplace_back (sel);
    parent.addAndMakeVisible (sel);
    return sel;
}

juce::Label* ZenithContent::caption (juce::Component& parent, const juce::String& text, juce::Rectangle<int> r)
{
    auto* l = new juce::Label ({}, text);
    owned.emplace_back (l);
    l->setFont (font (9.f));
    l->setColour (juce::Label::textColourId, look::print.withAlpha (0.85f));
    l->setJustificationType (juce::Justification::centredLeft);
    l->setInterceptsMouseClicks (false, false);
    l->setBorderSize ({});
    l->setBounds (r);
    parent.addAndMakeVisible (l);
    return l;
}

void ZenithContent::showModel (int m)
{
    m = juce::jlimit (0, 3, m);
    if (m == shownModel) return;
    shownModel = m;
    for (int g = 0; g < 4; ++g)
        for (auto* c : modelGroup[g]) c->setVisible (g == m);
}

// ---------------------------------------------------------------- facade
void ZenithContent::buildPanelTex()
{
    const int W = designW, H = designH;
    panelTex = juce::Image (juce::Image::RGB, W, H, false);
    juce::Random rr (4242);
    {
        juce::Image::BitmapData bd (panelTex, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < H; ++y)
        {
            const int streak = rr.nextInt (3);
            for (int x = 0; x < W; ++x)
            {
                const int v = 14 + rr.nextInt (6) + streak;
                bd.setPixelColour (x, y, juce::Colour::fromRGB ((juce::uint8) v, (juce::uint8) v, (juce::uint8) (v + 1)));
            }
        }
    }
    juce::Graphics g (panelTex);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), 0.f, 0.f, juce::Colours::transparentWhite, 0.f, (float) H * 0.5f, false));
    g.fillRect (0, 0, W, H);
    for (int i = 0; i < 9; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (0.018f));
        g.fillEllipse (rr.nextFloat() * (float) W, rr.nextFloat() * (float) H, 40.f + rr.nextFloat() * 60.f, 30.f + rr.nextFloat() * 40.f);
    }
    for (int i = 0; i < 320; ++i)
    {
        const float x = rr.nextFloat() * (float) W, y = rr.nextFloat() * (float) H;
        const float a = (rr.nextFloat() - 0.5f) * (rr.nextInt (4) == 0 ? 3.1f : 0.7f), len = 12.f + rr.nextFloat() * 130.f;
        g.setColour (juce::Colour::fromRGBA (205, 205, 210, (juce::uint8) (8 + rr.nextInt (28))));
        g.drawLine (x, y, x + len * std::cos (a), y + len * std::sin (a), 0.5f + rr.nextFloat() * 0.7f);
    }
    g.setColour (juce::Colour (0xff5a5b60));
    g.drawRect (0, 0, W, H, 5);
    g.setColour (juce::Colour (0xff1b1b1d));
    g.drawRect (5, 5, W - 10, H - 10, 1);

    g.setColour (look::print.withAlpha (0.93f));
    g.setFont (font (50.f));
    g.drawText ("ZENITH", 30, 6, 340, 58, juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xffc8281c));
    g.fillRect (32, 66, 180, 4);
    g.setColour (look::print.withAlpha (0.85f));
    g.setFont (font (10.5f));
    g.drawText ("MASTER BUS  -  OTT / EQ / COMPRESSOR / LIMITER", 32, 76, 420, 14, juce::Justification::centredLeft, false);
    g.setFont (font (9.f, false));
    g.drawText ("FOUDRE AUDIO  -  DENSE. PUNCHY. DIRTY.", 32, 91, 360, 12, juce::Justification::centredLeft, false);
    g.setFont (font (9.5f));
    g.drawText ("PRESET", W - 400, 24, 80, 13, juce::Justification::centredLeft, false);

    const juce::Point<float> screws[] = { { 16.f, 16.f }, { (float) W - 16.f, 16.f }, { 16.f, (float) H - 16.f }, { (float) W - 16.f, (float) H - 16.f } };
    for (auto p : screws)
    {
        g.setColour (juce::Colour (0xff8a8a90)); g.fillEllipse (p.x - 5.f, p.y - 5.f, 10.f, 10.f);
        g.setColour (juce::Colour (0xff2a2a2d)); g.drawLine (p.x - 3.5f, p.y - 1.f, p.x + 3.5f, p.y + 1.5f, 1.4f);
    }
    for (int i = 0; i < 12000; ++i)
    {
        g.setColour (juce::Colour (0xff121213).withAlpha (0.92f));
        g.fillRect (rr.nextInt (W), rr.nextInt (H), 1 + rr.nextInt (2), 1 + rr.nextInt (2));
    }
    for (int i = 0; i < 240; ++i)
    {
        const bool edge = rr.nextInt (2) == 0;
        const int x = edge ? (rr.nextBool() ? rr.nextInt (14) : W - 1 - rr.nextInt (14)) : rr.nextInt (W);
        const int y = edge ? rr.nextInt (H) : (rr.nextBool() ? rr.nextInt (14) : H - 1 - rr.nextInt (14));
        g.setColour (juce::Colour (0xffb5b5ba).withAlpha (0.25f + 0.4f * rr.nextFloat()));
        g.fillRect (x, y, 1 + rr.nextInt (3), 1);
    }
}

void ZenithContent::drawBolt (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to)
{
    juce::Path p;
    p.startNewSubPath (from);
    const int seg = 7;
    for (int i = 1; i < seg; ++i)
    {
        const float t = (float) i / (float) seg;
        p.lineTo (from + (to - from) * t + juce::Point<float> ((rnd.nextFloat() - 0.5f) * 16.f, (rnd.nextFloat() - 0.5f) * 10.f));
    }
    p.lineTo (to);
    g.setColour (juce::Colour (0xff8fb8ff).withAlpha (0.35f));
    g.strokePath (p, juce::PathStrokeType (5.f));
    g.setColour (juce::Colour (0xfff2f7ff));
    g.strokePath (p, juce::PathStrokeType (1.6f));
}

void ZenithContent::drawTube (juce::Graphics& g, juce::Rectangle<float> r, float glow, float arc)
{
    const auto base = r.removeFromBottom (r.getHeight() * 0.13f);
    const auto glass = r.withTrimmedBottom (-8.f);
    const auto col = heatColour (glow);
    if (glow > 0.02f)
    {
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.55f * glow), glass.getCentreX(), glass.getCentreY() + 4.f,
                                                 col.withAlpha (0.f), glass.getCentreX(), glass.getCentreY() - glass.getHeight() * 0.95f, true));
        g.fillEllipse (glass.expanded (glass.getWidth() * 0.9f));
    }
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (glass, glass.getWidth() * 0.5f);
    auto plate = glass.reduced (glass.getWidth() * 0.22f, 0.f).withTrimmedTop (glass.getHeight() * 0.22f).withTrimmedBottom (glass.getHeight() * 0.18f);
    g.setColour (col);
    g.fillRoundedRectangle (plate, 3.f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (float x = plate.getX() + 3.f; x < plate.getRight(); x += 5.f)
        g.drawVerticalLine ((int) x, plate.getY(), plate.getBottom());
    g.setColour (juce::Colour (0xffff9a3a).withAlpha (0.25f + 0.7f * glow));
    g.fillEllipse (glass.getCentreX() - 4.f, glass.getBottom() - glass.getHeight() * 0.14f, 8.f, 6.f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f), glass.getX(), glass.getY(),
                                             juce::Colours::white.withAlpha (0.f), glass.getX() + glass.getWidth() * 0.45f, glass.getY(), false));
    g.fillRoundedRectangle (glass.reduced (2.f), glass.getWidth() * 0.45f);
    g.setColour (juce::Colour (0xff8d8d94).withAlpha (0.8f));
    g.drawRoundedRectangle (glass, glass.getWidth() * 0.5f, 1.4f);
    if (arc > 0.2f)
        drawBolt (g, { glass.getCentreX(), glass.getY() + 4.f }, { plate.getCentreX() + (rnd.nextFloat() - 0.5f) * 10.f, plate.getBottom() });
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6c6c72), base.getX(), base.getY(),
                                             juce::Colour (0xff1a1a1c), base.getX(), base.getBottom(), false));
    g.fillRoundedRectangle (base.expanded (3.f, 0.f), 3.f);
}

void ZenithContent::paint (juce::Graphics& g)
{
    g.drawImageAt (panelTex, 0, 0);
    const juce::Rectangle<float> win (440.f, 6.f, 400.f, 98.f);
    g.setColour (juce::Colour (0xff040404));
    g.fillRoundedRectangle (win, 6.f);
    g.setColour (juce::Colour (0xff8d8d92).withAlpha (0.8f));
    g.drawRoundedRectangle (win, 6.f, 2.f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (win.reduced (3.f), 5.f, 2.f);

    const float h = heatShown;
    struct T { float x, w, hgt, thr; };
    const T tubes[] = { { 490.f, 34.f, 70.f, 0.00f }, { 570.f, 34.f, 70.f, 0.12f }, { 655.f, 46.f, 88.f, 0.25f }, { 745.f, 46.f, 88.f, 0.40f } };
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (win.reduced (3.f).toNearestInt());
        for (const auto& t : tubes)
            drawTube (g, { t.x, 100.f - t.hgt, t.w, t.hgt }, juce::jlimit (0.f, 1.f, (h - t.thr) / (1.f - t.thr)), arcShown);
        if (arcShown > 0.35f)
            drawBolt (g, { 510.f + rnd.nextFloat() * 40.f, 22.f }, { 800.f - rnd.nextFloat() * 40.f, 26.f });
    }
    const float lamp = juce::jlimit (0.f, 1.f, 0.30f + 0.70f * h) * (arcShown > 0.3f ? 1.f : 0.92f + 0.08f * rnd.nextFloat());
    const juce::Point<float> c (385.f, 54.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff2a1a).withAlpha (0.55f * lamp), c.x, c.y,
                                             juce::Colour (0xffff2a1a).withAlpha (0.f), c.x + 40.f, c.y, true));
    g.fillEllipse (c.x - 40.f, c.y - 40.f, 80.f, 80.f);
    g.setColour (juce::Colour (0xff9a9aa0));
    g.fillEllipse (c.x - 18.f, c.y - 18.f, 36.f, 36.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffb09a).interpolatedWith (juce::Colour (0xffffe9b0), h * lamp), c.x - 4.f, c.y - 6.f,
                                             juce::Colour (0xff4a0804).interpolatedWith (juce::Colour (0xffd01808), lamp), c.x + 12.f, c.y + 14.f, true));
    g.fillEllipse (c.x - 14.f, c.y - 14.f, 28.f, 28.f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillEllipse (c.x - 8.f, c.y - 10.f, 9.f, 5.f);
    g.setColour (look::print.withAlpha (0.9f));
    g.setFont (font (10.f));
    g.drawText ("HEAT", (int) c.x - 30, (int) c.y + 24, 60, 14, juce::Justification::centred, false);
}

// ---------------------------------------------------------------- animation
void ZenithContent::timerCallback()
{
    auto& m = proc.master;
    const float cg = m.cmpGr.load(), lg = m.limGr.load();
    float ott = 0.f;
    for (int b = 0; b < 3; ++b) ott = std::max (ott, m.bandDown[b].load());
    const float heat = juce::jlimit (0.f, 1.f, 0.45f * cg / 16.f + 0.35f * lg / 8.f + 0.2f * ott / 14.f);
    heatShown += 0.2f * (heat - heatShown);
    arcShown = juce::jmax (arcShown * 0.82f, lg > 6.f ? 1.f : 0.f);
    repaint (0, 0, designW, 110);

    vu.setValue (cg); vu.tick();
    inBar->set (linToDb (m.inPk.load()));
    outBar->set (linToDb (m.outPk.load()));
    limBar->set (lg);
    for (int b = 0; b < 3; ++b)
    {
        const float d = m.bandDown[b].load(), u = m.bandUp[b].load();
        bandDown[b]->set (d); bandUp[b]->set (u);
        lcdBand[b]->set (juce::String (d, 1) + " / " + juce::String (u, 1));
    }
    auto lu = [] (float v) { return v <= -69.f ? juce::String ("-inf") : juce::String (v, 1); };
    lcdInS->set (lu (m.lufsIn.shortTerm.load()));
    lcdOutM->set (lu (m.lufsOut.momentary.load()));
    lcdOutS->set (lu (m.lufsOut.shortTerm.load()));
    lcdOutI->set (lu (m.lufsOut.integrated.load()));
    lcdMatch->set (juce::String (m.matchDb.load(), 1));

    showModel ((int) std::lround (proc.apvts.getRawParameterValue ("cmpModel")->load()));

    // macros : chaque potard deplace montre un arc orange jusqu'a la valeur reellement appliquee
    {
        float v[kNumP], e[kNumP];
        for (int i = 0; i < kNumP; ++i) v[i] = proc.apvts.getRawParameterValue (paramDefs()[i].id)->load();
        Master::applyMacros (v, e);
        for (int i = 0; i < kNumP; ++i)
        {
            if (byId[i] == nullptr || i == p_mDyn || i == p_mBri || i == p_mSq) continue;
            const auto& d = paramDefs()[i];
            const bool moved = std::abs (e[i] - v[i]) > (d.mx - d.mn) * 0.004f;
            byId[i]->setMacroValue (moved, e[i]);
            byId[i]->setCvActive (moved);
        }
    }
}

// ---------------------------------------------------------------- presets
void ZenithContent::afterPreset() { nameBtn.setButtonText (proc.presetName); }

void ZenithContent::stepPreset (int dir)
{
    juce::StringArray all;
    const int nf = ZenithProcessor::getNumFactoryPresets();
    for (int i = 0; i < nf; ++i) all.add (ZenithProcessor::getFactoryPresetName (i));
    all.addArray (proc.getUserPresetNames());
    int cur = all.indexOf (proc.presetName);
    cur = cur < 0 ? (dir > 0 ? -1 : 0) : cur;
    const int next = (cur + dir + all.size()) % juce::jmax (1, all.size());
    if (next < nf) proc.loadFactoryPreset (next); else proc.loadUserPreset (all[next]);
    afterPreset();
}

void ZenithContent::presetMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("FACTORY");
    for (int i = 0; i < ZenithProcessor::getNumFactoryPresets(); ++i)
        m.addItem (100 + i, ZenithProcessor::getFactoryPresetName (i), true, ZenithProcessor::getFactoryPresetName (i) == proc.presetName);
    const auto user = proc.getUserPresetNames();
    if (user.size() > 0)
    {
        m.addSeparator();
        m.addSectionHeader ("USER");
        for (int i = 0; i < user.size(); ++i) m.addItem (1000 + i, user[i], true, user[i] == proc.presetName);
    }
    m.addSeparator();
    m.addItem (1, "Save as...");
    if (user.contains (proc.presetName))
    {
        m.addItem (2, "Overwrite \"" + proc.presetName + "\"");
        m.addItem (3, "Delete \"" + proc.presetName + "\"");
    }
    m.addItem (4, "Open preset folder");

    juce::Component::SafePointer<ZenithContent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameBtn), [safe, user] (int id)
    {
        if (safe == nullptr || id == 0) return;
        auto& pr = safe->proc;
        if (id >= 1000)      { pr.loadUserPreset (user[id - 1000]); safe->afterPreset(); }
        else if (id >= 100)  { pr.loadFactoryPreset (id - 100); safe->afterPreset(); }
        else if (id == 1)    safe->saveDialog();
        else if (id == 2)    { pr.saveUserPreset (pr.presetName); safe->afterPreset(); }
        else if (id == 3)
        {
            const auto name = pr.presetName;
            safe->confirm ("Supprimer le preset", "Supprimer definitivement \"" + name + "\" ?", [safe, name]
            {
                if (safe != nullptr) { safe->proc.deleteUserPreset (name); safe->proc.presetName = "Init"; safe->afterPreset(); }
            });
        }
        else if (id == 4)    { pr.getPresetDir().createDirectory(); pr.getPresetDir().revealToUser(); }
    });
}

void ZenithContent::showDialog (const juce::String& title, const juce::String& message, bool askText, const juce::String& initial,
                                const juce::String& okText, std::function<void (bool, const juce::String&)> cb)
{
    dialog = std::make_unique<OrageDialog> (title, message, askText, initial, okText, std::move (cb));
    juce::Component::SafePointer<ZenithContent> safe (this);
    dialog->onFinished = [safe] { juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->dialog.reset(); }); };
    addAndMakeVisible (*dialog);
    dialog->setBounds (getLocalBounds());
    dialog->toFront (true);
    dialog->focus();
}

void ZenithContent::saveDialog()
{
    juce::Component::SafePointer<ZenithContent> safe (this);
    showDialog ("SAUVEGARDER LE PRESET", "Nom du preset :", true, proc.presetName, "SAUVEGARDER", [safe] (bool ok, const juce::String& text)
    {
        if (ok && safe != nullptr) { safe->proc.saveUserPreset (text); safe->afterPreset(); }
    });
}

void ZenithContent::confirm (const juce::String& title, const juce::String& message, std::function<void()> action)
{
    showDialog (title, message, false, {}, "OK", [action] (bool ok, const juce::String&) { if (ok) action(); });
}

// ================================================================ fenetre redimensionnable
ZenithEditor::ZenithEditor (ZenithProcessor& p) : AudioProcessorEditor (&p), content (p)
{
    addAndMakeVisible (content);
    const int dw = ZenithContent::designW, dh = ZenithContent::designH;
    float s = 1.f;                                                  // taille initiale : tient dans l'ecran
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        s = juce::jmin (1.f, (float) (d->userArea.getHeight() - 90) / (float) dh, (float) (d->userArea.getWidth() - 20) / (float) dw);
    s = juce::jmax (0.5f, s);
    constrainer.setFixedAspectRatio ((double) dw / (double) dh);
    constrainer.setSizeLimits (dw / 2, dh / 2, dw * 3 / 2, dh * 3 / 2);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (juce::roundToInt ((float) dw * s), juce::roundToInt ((float) dh * s));
}

void ZenithEditor::resized()
{
    content.setTransform ({});
    content.setBounds (0, 0, ZenithContent::designW, ZenithContent::designH);
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) ZenithContent::designW));
}
