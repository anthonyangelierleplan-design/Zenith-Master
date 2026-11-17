#include <complex>
#include "MasterEditor.h"

using namespace tn;
using namespace ui;

namespace
{
juce::File themeFile() { return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Foudre Audio").getChildFile ("zenith-theme.txt"); }
int loadThemePref() { const auto f = themeFile(); return f.existsAsFile() ? f.loadFileAsString().trim().getIntValue() : 2; }
void saveThemePref (int i) { const auto f = themeFile(); f.getParentDirectory().createDirectory(); f.replaceWithText (juce::String (i)); }
juce::Colour C (juce::uint32 c) { return juce::Colour (c); }

// libelles des modeles de compresseur
const char* kModelNames[4] { "CLEAN", "VARI-MU", "FET", "DIODE" };
const char* kModelText[4] {
    "CLEAN\nTransparent digital compressor.\nSoft knee, sidechain HPF, auto gain.",
    "VARI-MU  -  6386 PUSH-PULL\nTube limiter, output-sensing.\nTC 1-4 fixed, 5-6 program dependent.",
    "FET  -  CLASS-A LINE AMP\nFast FET limiter.\nALL = all ratio buttons in.",
    "DIODE BRIDGE  -  CLASS-A\nWide-knee bus compressor.\nRecovery A1 / A2 = auto." };

// zone de rendu des lampes dans l'en-tete
const juce::Rectangle<float> kTubeWin (452.f, 12.f, 400.f, 86.f);
}

// ================================================================ petits composants
void Plate::paint (juce::Graphics& g)
{
    drawPlate (g, getLocalBounds().toFloat());
    for (const auto& in : insets)
    {
        drawInset (g, in.second.toFloat());
        if (in.first.isNotEmpty())
        {
            g.setColour (look::print.withAlpha (0.8f));
            g.setFont (font (11.f));
            g.drawText (in.first, in.second.getX() + 12, in.second.getY() + 6, in.second.getWidth() - 24, 14, juce::Justification::centredLeft, false);
        }
    }
    g.setColour (look::print);
    g.setFont (font (13.f));
    g.drawText (title, 24, 8, getWidth() - 48, 18, juce::Justification::centredLeft, false);
    g.setColour (look::cvRing.withAlpha (0.85f));
    g.fillRect (24, 28, juce::roundToInt (font (13.f).getStringWidthFloat (title)) , 2);
}

void Plate::paintOverChildren (juce::Graphics& g)
{
    g.setColour (look::print.withAlpha (0.8f));
    g.setFont (font (10.f));
    for (const auto& l : labels) g.drawText (l.first, l.second, juce::Justification::centred, false);
}

void LedBar::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float capH = caption.isNotEmpty() && ! horizontal ? 13.f : 0.f;
    const auto bar = b.withTrimmedBottom (capH);
    g.setColour (juce::Colour (0xff050506));
    g.fillRoundedRectangle (bar, 3.f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (bar, 3.f, 1.f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine ((int) bar.getBottom(), bar.getX() + 3.f, bar.getRight() - 3.f);
    const float len = horizontal ? bar.getWidth() : bar.getHeight();
    const int n = juce::jmax (8, (int) (len / 6.f));
    const float seg = (len - 6.f) / (float) n;
    const float frac = juce::jlimit (0.f, 1.f, (value - lo_) / (hi_ - lo_));
    for (int i = 0; i < n; ++i)
    {
        const float t = (float) i / (float) (n - 1);
        const bool lit = (float) i / (float) n < frac;
        juce::Colour c;
        if (mode == Level)          c = t < 0.65f ? juce::Colour (0xff4fd067) : (t < 0.88f ? juce::Colour (0xffe3b23a) : juce::Colour (0xffe0362a));
        else if (mode == Reduction) c = look::cvRing;
        else                        c = juce::Colour (0xff4fd0b8);
        g.setColour (lit ? c : c.darker (3.f).withAlpha (0.5f));
        if (horizontal)
        {
            const float x = mode == Reduction ? bar.getRight() - 3.f - (float) (i + 1) * seg : bar.getX() + 3.f + (float) i * seg;
            g.fillRect (x + 0.5f, bar.getY() + 3.f, juce::jmax (1.f, seg - 1.5f), bar.getHeight() - 6.f);
        }
        else
        {
            const float y = mode == Reduction ? bar.getY() + 3.f + (float) i * seg : bar.getBottom() - 3.f - (float) (i + 1) * seg;
            g.fillRect (bar.getX() + 3.f, y + 0.5f, bar.getWidth() - 6.f, juce::jmax (1.f, seg - 1.5f));
        }
    }
    if (capH > 0.f)
    {
        g.setColour (look::print.withAlpha (0.85f));
        g.setFont (font (9.5f));
        g.drawText (caption, b.withTrimmedTop (b.getHeight() - capH).expanded (8.f, 0.f), juce::Justification::centred, false);
    }
}

void Lcd::paint (juce::Graphics& g)
{
    const auto& th = look::T();
    const auto b = getLocalBounds().toFloat().reduced (1.f);
    drawLcdBox (g, b);
    const juce::Colour ink (th.lcdText);
    if (multiline)
    {
        auto lines = juce::StringArray::fromLines (text);
        float y = b.getY() + 8.f;
        for (int i = 0; i < lines.size(); ++i)
        {
            g.setColour (i == 0 ? ink : ink.withAlpha (0.78f));
            g.setFont (font (i == 0 ? 13.f : 12.f, i == 0));
            g.drawText (lines[i], b.withTrimmedLeft (10.f).withY (y).withHeight (16.f), juce::Justification::centredLeft, false);
            y += i == 0 ? 22.f : 18.f;
        }
        return;
    }
    if (label.isNotEmpty())
    {
        g.setColour (ink.withAlpha (0.6f));
        g.setFont (font (9.5f));
        g.drawText (label, b.withTrimmedTop (3.f).withHeight (12.f), juce::Justification::centred, false);
    }
    g.setColour (ink.withAlpha (0.18f));                                              // halo du segment
    g.setFont (juce::Font (juce::FontOptions (large ? 34.f : 17.f, juce::Font::bold)));
    const auto tr = label.isNotEmpty() ? b.withTrimmedTop (12.f) : b;
    g.drawText (text, tr.translated (0.f, 1.f), juce::Justification::centred, false);
    g.setColour (ink);
    g.drawText (text, tr, juce::Justification::centred, false);
}

void ModuleTile::setState (bool on, const juce::String& sub, const juce::String& value, bool sel)
{
    if (on == active && sub == subText && value == valueText && sel == selected) return;
    active = on; subText = sub; valueText = value; selected = sel;
    repaint();
}

void ModuleTile::paint (juce::Graphics& g)
{
    const auto& th = look::T();
    auto b = getLocalBounds().toFloat().reduced (1.f).withTrimmedBottom (9.f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (b.translated (0.f, 2.f), 6.f);
    g.setGradientFill (juce::ColourGradient (C (th.plateTop).brighter (hover ? 0.12f : 0.05f), b.getX(), b.getY(), C (th.plateBot), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 6.f);
    if (selected)
    {
        g.setColour (look::cvRing.withAlpha (0.18f));
        g.fillRoundedRectangle (b, 6.f);
        g.setColour (look::cvRing);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.f, 2.f);
        juce::Path tri;                                                           // pointe vers le tiroir
        tri.addTriangle (b.getCentreX() - 10.f, b.getBottom(), b.getCentreX() + 10.f, b.getBottom(), b.getCentreX(), b.getBottom() + 9.f);
        g.fillPath (tri);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawHorizontalLine ((int) b.getY() + 1, b.getX() + 6.f, b.getRight() - 6.f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (b, 6.f, 1.f);
    }
    const auto badge = juce::Rectangle<float> (24.f, 24.f).withCentre ({ b.getX() + 22.f, b.getCentreY() });
    g.setColour (selected ? look::cvRing : look::print.withAlpha (0.2f));
    g.fillEllipse (badge);
    g.setColour (selected ? juce::Colours::black : look::print);
    g.setFont (font (13.f));
    g.drawText (juce::String (number), badge, juce::Justification::centred, false);
    g.setColour (look::print);
    g.setFont (font (15.f));
    g.drawText (name, (int) b.getX() + 42, (int) b.getY() + 7, (int) b.getWidth() - 130, 18, juce::Justification::centredLeft, false);
    g.setColour (look::print.withAlpha (0.65f));
    g.setFont (font (11.f, false));
    g.drawText (subText, (int) b.getX() + 42, (int) b.getY() + 26, (int) b.getWidth() - 130, 14, juce::Justification::centredLeft, false);
    drawJewel (g, { b.getRight() - 96.f, b.getCentreY() - 4.5f, 9.f, 9.f }, active, juce::Colour (th.led));
    const auto vb = juce::Rectangle<float> (b.getRight() - 80.f, b.getCentreY() - 13.f, 70.f, 26.f);
    drawLcdBox (g, vb);
    g.setColour (C (th.lcdText));
    g.setFont (font (14.f));
    g.drawText (valueText, vb, juce::Justification::centred, false);
}

void EqCurve::setValues (const float* e, bool on)
{
    const float nv[8] { e[p_eqHpf], e[p_lsF], e[p_lsG], e[p_mdF], e[p_mdG], e[p_mdQ], e[p_hsF], e[p_hsG] };
    float* cur[8] { &hpf, &lsF, &lsG, &mdF, &mdG, &mdQ, &hsF, &hsG };
    bool changed = on != enabled;
    for (int i = 0; i < 8; ++i) if (std::abs (*cur[i] - nv[i]) > 1e-3f) { *cur[i] = nv[i]; changed = true; }
    enabled = on;
    if (changed) repaint();
}

void EqCurve::paint (juce::Graphics& g)
{
    const auto& th = look::T();
    const auto b = getLocalBounds().toFloat();
    drawLcdBox (g, b);
    const auto plot = b.reduced (14.f, 16.f);
    const juce::Colour ink (th.lcdText);
    auto xOf = [&] (float f) { return plot.getX() + plot.getWidth() * std::log (f / 20.f) / std::log (1000.f); };
    auto yOf = [&] (float db) { return plot.getCentreY() - db / 15.f * plot.getHeight() * 0.5f; };
    g.setFont (font (9.5f, false));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        g.setColour (ink.withAlpha (0.1f));
        g.drawVerticalLine ((int) xOf (f), plot.getY(), plot.getBottom());
        g.setColour (ink.withAlpha (0.45f));
        g.drawText (f >= 1000.f ? juce::String (f / 1000.f, 0) + "k" : juce::String ((int) f), (int) xOf (f) - 16, (int) plot.getBottom() + 1, 32, 12, juce::Justification::centred, false);
    }
    for (float db : { -12.f, -6.f, 0.f, 6.f, 12.f })
    {
        g.setColour (ink.withAlpha (db == 0.f ? 0.25f : 0.1f));
        g.drawHorizontalLine ((int) yOf (db), plot.getX(), plot.getRight());
        g.setColour (ink.withAlpha (0.45f));
        g.drawText ((db > 0 ? "+" : "") + juce::String ((int) db), (int) plot.getX() - 14, (int) yOf (db) - 6, 26, 12, juce::Justification::centredLeft, false);
    }
    Biquad bq[4];
    const float fs = 48000.f;
    bq[0].highpass (hpf, fs); bq[1].lowshelf (lsF, fs, lsG); bq[2].peak (mdF, fs, mdG, mdQ); bq[3].highshelf (std::min (hsF, 0.45f * fs), fs, hsG);
    juce::Path p;
    const int n = (int) plot.getWidth();
    for (int i = 0; i <= n; ++i)
    {
        const float f = 20.f * std::pow (1000.f, (float) i / (float) n), w = 2.f * kPi * f / fs;
        const std::complex<float> z1 = std::polar (1.f, -w), z2 = z1 * z1;
        float db = 0.f;
        for (auto& q : bq)
        {
            const auto h = (q.b0 + q.b1 * z1 + q.b2 * z2) / (1.f + q.a1 * z1 + q.a2 * z2);
            db += 20.f * std::log10 (std::max (1e-6f, std::abs (h)));
        }
        const float y = yOf (juce::jlimit (-16.f, 16.f, enabled ? db : 0.f));
        if (i == 0) p.startNewSubPath (plot.getX(), y); else p.lineTo (plot.getX() + (float) i, y);
    }
    const juce::Colour col = enabled ? look::cvRing : ink.withAlpha (0.4f);
    juce::Path fill (p);
    fill.lineTo (plot.getRight(), yOf (0.f)); fill.lineTo (plot.getX(), yOf (0.f)); fill.closeSubPath();
    g.setColour (col.withAlpha (0.12f)); g.fillPath (fill);
    g.setColour (col.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (5.f, juce::PathStrokeType::curved));
    g.setColour (col);                   g.strokePath (p, juce::PathStrokeType (2.f, juce::PathStrokeType::curved));
    g.setColour (ink.withAlpha (0.6f));
    g.setFont (font (10.f));
    g.drawText (enabled ? "EQ RESPONSE" : "EQ OFF", b.reduced (12.f, 6.f), juce::Justification::topRight, false);
}

void ModelFace::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    if (model == 0) { drawInset (g, b); return; }
    static const juce::uint32 top[4] { 0, 0xff454b49, 0xff1c1c1e, 0xff52606e }, bot[4] { 0, 0xff2c312f, 0xff0c0c0d, 0xff36414c };
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (b.translated (0.f, 2.f), 5.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (top[model]), 0.f, b.getY(), juce::Colour (bot[model]), 0.f, b.getBottom(), false));
    g.fillRoundedRectangle (b, 5.f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (b.reduced (2.f).toNearestInt());
        juce::Random rr (77 + model);
        if (model == 1)                                                               // peinture martelee
            for (int k = 0; k < 900; ++k)
            {
                const float x = rr.nextFloat() * b.getWidth(), y = rr.nextFloat() * b.getHeight(), r = 3.f + rr.nextFloat() * 9.f;
                g.setColour ((rr.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.025f + 0.03f * rr.nextFloat()));
                g.fillEllipse (x, y, r, r * 0.8f);
            }
        else                                                                          // brossage
            for (float y = 2.f; y < b.getHeight(); y += 1.5f)
            {
                g.setColour ((rr.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.015f + 0.03f * rr.nextFloat()));
                g.drawHorizontalLine ((int) y, 0.f, b.getWidth());
            }
        if (model == 2)                                                               // bandeau alu en haut (facon rack noir et argent)
        {
            const auto band = b.withHeight (28.f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd9dce1), 0.f, band.getY(), juce::Colour (0xff8a8f97), 0.f, band.getBottom(), false));
            g.fillRect (band);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.drawHorizontalLine ((int) band.getBottom(), 0.f, b.getWidth());
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), 0.f, 0.f, juce::Colours::transparentWhite, b.getWidth() * 0.5f, b.getHeight(), false));
        g.fillRect (b);
    }
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawHorizontalLine (1, 6.f, b.getWidth() - 6.f);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawRoundedRectangle (b.reduced (0.5f), 5.f, 1.2f);
    int k = 0;
    for (auto p : { juce::Point<float> (10.f, 10.f), { b.getWidth() - 10.f, 10.f }, { 10.f, b.getHeight() - 10.f }, { b.getWidth() - 10.f, b.getHeight() - 10.f } })
        drawScrew (g, p, 4.f, 11 + k++);
}

// ================================================================ contenu (taille de conception fixe)
ZenithContent::ZenithContent (ZenithProcessor& p) : proc (p)
{
    look::applyTheme (loadThemePref());
    laf.refresh();
    setLookAndFeel (&laf);
    setSize (designW, designH);
    buildPanelTex();

    // ---- en-tete : presets, bypass, comparaison, theme
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &nameBtn, &themeBtn }) addAndMakeVisible (*b);
    prevBtn.setBounds (992, 28, 32, 30);
    nameBtn.setBounds (1028, 28, 220, 30);
    nextBtn.setBounds (1252, 28, 32, 30);
    saveBtn.setBounds (1288, 28, 70, 30);
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    nameBtn.onClick = [this] { presetMenu(); };
    saveBtn.onClick = [this] { saveDialog(); };
    themeBtn.onClick = [this] { cycleTheme(); };
    themeBtn.setButtonText (look::T().name);
    themeBtn.setTooltip ("Theme");
    toggle (*this, p_bypass, "BYPASS")->setBounds (990, 64, 114, 30);
    toggle (*this, p_match, "LEVEL MATCH")->setBounds (1106, 64, 144, 30);
    themeBtn.setBounds (1254, 66, 104, 28);

    // ---- cockpit : activite | intensite + macros | sortie + loudness
    for (auto* pl : { &actPlate, &macroPlate, &outPlate }) addAndMakeVisible (pl);
    actPlate.setBounds (34, 110, 424, 338);
    {
        actPlate.addAndMakeVisible (vu);
        vu.setBounds (16, 36, 392, 178);
        static const char* names[4] { "OTT LOW", "OTT MID", "OTT HIGH", "LIMITER" };
        for (int i = 0; i < 4; ++i)
        {
            const int y = 224 + i * 27;
            actPlate.labels.push_back ({ names[i], { 14, y, 86, 22 } });
            actBar[i] = new LedBar (LedBar::Reduction, 0.f, 18.f, {}, true); owned.emplace_back (actBar[i]); actPlate.addAndMakeVisible (actBar[i]);
            actBar[i]->setBounds (104, y + 3, 226, 16);
            lcdAct[i] = new Lcd ({}); owned.emplace_back (lcdAct[i]); actPlate.addAndMakeVisible (lcdAct[i]);
            lcdAct[i]->setBounds (338, y, 70, 23);
        }
    }
    macroPlate.setBounds (466, 110, 476, 338);
    {
        auto* k = knob (macroPlate, p_intensity, "INTENSITY", "0", "MAX");
        k->setBounds (146, 26, 184, 186);
        knob (macroPlate, p_mDyn, "DYNAMICS", "SQUEEZE", "PUNCH", 0.5)->setBounds (16, 218, 144, 116);
        knob (macroPlate, p_mBri, "BRILLIANCE", "DARK", "BRIGHT", 0.5)->setBounds (166, 218, 144, 116);
        knob (macroPlate, p_mSq, "SNAPPY / SQUASHY", "SNAPPY", "SQUASHY", 0.5)->setBounds (316, 218, 144, 116);
    }
    outPlate.setBounds (950, 110, 416, 338);
    {
        lcdOutI = new Lcd ("INTEGRATED LUFS", true); lcdOutS = new Lcd ("SHORT-TERM"); lcdOutM = new Lcd ("MOMENTARY");
        lcdMatch = new Lcd ("MATCH TRIM dB"); lcdDelta = new Lcd ({});
        for (auto* l : { lcdOutI, lcdOutS, lcdOutM, lcdMatch, lcdDelta }) { owned.emplace_back (l); outPlate.addAndMakeVisible (l); }
        lcdOutI->setBounds (16, 38, 232, 80);
        lcdOutS->setBounds (256, 38, 144, 38);
        lcdOutM->setBounds (256, 80, 144, 38);
        outPlate.labels.push_back ({ "LOUDNESS TARGET", { 16, 124, 384, 14 } });
        selector (outPlate, p_lufsTgt)->setBounds (14, 140, 388, 32);
        lcdDelta->setBounds (16, 176, 384, 26);
        knob (outPlate, p_out, "OUTPUT", "-12", "+6")->setBounds (10, 206, 124, 126);
        outBar = new LedBar (LedBar::Level, -60.f, 0.f, {}, true); owned.emplace_back (outBar); outPlate.addAndMakeVisible (outBar);
        outPlate.labels.push_back ({ "OUTPUT PEAK  dBFS", { 144, 210, 256, 14 } });
        outBar->setBounds (144, 226, 256, 18);
        lcdMatch->setBounds (144, 256, 124, 44);
        outPlate.addAndMakeVisible (resetLufs);
        resetLufs.setBounds (276, 262, 124, 32);
        resetLufs.onClick = [this] { proc.master.resetLufsFlag.store (true); };
    }

    // ---- chaine : une tuile par module, ouvre son tiroir
    for (int i = 0; i < 5; ++i)
    {
        addAndMakeVisible (tiles[i]);
        tiles[i].setBounds (34 + i * 270, 456, 252, 66);
        tiles[i].onClick = [this, i] { showTab (i); };
    }

    // ---- tiroirs
    for (auto& d : drawer) { addChildComponent (d); d.setBounds (34, 528, 1332, 322); }

    // 1 : INPUT
    {
        auto& d = drawer[0];
        d.insets.push_back ({ "INPUT TRIM", { 16, 38, 640, 270 } });
        d.insets.push_back ({ "SIGNAL CHAIN", { 668, 38, 648, 270 } });
        knob (d, p_in, "INPUT", "-12", "+12")->setBounds (40, 66, 180, 230);
        inBar = new LedBar (LedBar::Level, -60.f, 0.f, {}, true); owned.emplace_back (inBar); d.addAndMakeVisible (inBar);
        d.labels.push_back ({ "INPUT PEAK  dBFS", { 250, 82, 380, 14 } });
        inBar->setBounds (250, 100, 380, 20);
        lcdInS = new Lcd ("INPUT LUFS  SHORT-TERM", true); owned.emplace_back (lcdInS); d.addAndMakeVisible (lcdInS);
        lcdInS->setBounds (250, 140, 230, 80);
        caption (d, "Trim before the chain. LEVEL MATCH compares in / out at equal loudness.", { 250, 236, 390, 40 }, 12.f)->setJustificationType (juce::Justification::topLeft);
        caption (d, fr ("INPUT  \xe2\x86\x92  OTT  \xe2\x86\x92  EQ  \xe2\x86\x92  COMPRESSOR  \xe2\x86\x92  DIODE LIMITER  \xe2\x86\x92  CLIP  \xe2\x86\x92  BRICKWALL  \xe2\x86\x92  OUTPUT"),
                 { 690, 90, 610, 24 }, 13.f);
        caption (d, "INTENSITY scales the whole chain: at 0 the signal passes untouched.\nMacros move the real settings: watch the white arcs on the knobs.\nThe tubes in the header warm up with the processing.",
                 { 690, 136, 610, 90 }, 12.f)->setJustificationType (juce::Justification::topLeft);
    }
    // 2 : OTT
    {
        auto& d = drawer[1];
        d.insets.push_back ({ "GLOBAL", { 16, 38, 290, 270 } });
        toggle (d, p_ottOn, "OTT ON")->setBounds (28, 62, 128, 30);
        toggle (d, p_ottMS, "MID / SIDE")->setBounds (164, 62, 130, 30);
        knob (d, p_ottAtk, "ATTACK", "FAST", "SLOW")->setBounds (22, 98, 92, 100);
        knob (d, p_ottRel, "RELEASE", "FAST", "SLOW")->setBounds (114, 98, 92, 100);
        knob (d, p_ottDepth, "DEPTH", "0", "100")->setBounds (206, 98, 92, 100);
        knob (d, p_xo1, "LOW / MID", "40", "400")->setBounds (40, 202, 104, 102);
        knob (d, p_xo2, "MID / HIGH", "1k", "8k")->setBounds (160, 202, 104, 102);
        static const char* names[3] { "LOW BAND", "MID BAND", "HIGH BAND" };
        static const P ids[3][5] { { p_tL, p_fL, p_gL, p_dL, p_uL }, { p_tM, p_fM, p_gM, p_dM, p_uM }, { p_tH, p_fH, p_gH, p_dH, p_uH } };
        for (int b = 0; b < 3; ++b)
        {
            const int bx = 318 + b * 336;
            d.insets.push_back ({ names[b], { bx, 38, 326, 270 } });
            bandDown[b] = new LedBar (LedBar::Reduction, 0.f, 18.f, "DN"); owned.emplace_back (bandDown[b]); d.addAndMakeVisible (bandDown[b]);
            bandUp[b]   = new LedBar (LedBar::Boost, 0.f, 24.f, "UP");     owned.emplace_back (bandUp[b]);   d.addAndMakeVisible (bandUp[b]);
            bandDown[b]->setBounds (bx + 12, 64, 20, 236);
            bandUp[b]->setBounds (bx + 36, 64, 20, 236);
            knob (d, ids[b][0], "THRESH", "-50", "-6")->setBounds (bx + 62, 58, 86, 116);
            knob (d, ids[b][1], "FLOOR", "-70", "-20")->setBounds (bx + 148, 58, 86, 116);
            knob (d, ids[b][2], "GAIN", "-12", "+12")->setBounds (bx + 234, 58, 86, 116);
            knob (d, ids[b][3], "DOWN", "0", "10")->setBounds (bx + 62, 182, 86, 116);
            knob (d, ids[b][4], "UP", "0", "10")->setBounds (bx + 148, 182, 86, 116);
            lcdBand[b] = new Lcd ("DOWN / UP dB"); owned.emplace_back (lcdBand[b]); d.addAndMakeVisible (lcdBand[b]);
            lcdBand[b]->setBounds (bx + 238, 214, 78, 48);
        }
    }
    // 3 : EQ
    {
        auto& d = drawer[2];
        d.addAndMakeVisible (eqCurve);
        eqCurve.setBounds (16, 38, 600, 270);
        d.insets.push_back ({ {}, { 628, 38, 688, 270 } });
        toggle (d, p_eqOn, "EQ ON")->setBounds (1186, 6, 130, 28);
        const struct { P id; const char* t; const char* lo; const char* hi; } ks[8] {
            { p_eqHpf, "HPF", "20", "250" }, { p_lsG, "LOW GAIN", "-12", "+12" }, { p_lsF, "LOW FREQ", "40", "400" }, { p_mdG, "MID GAIN", "-12", "+12" },
            { p_mdF, "MID FREQ", "200", "8k" }, { p_mdQ, "MID Q", "WIDE", "NARROW" }, { p_hsG, "HIGH GAIN", "-12", "+12" }, { p_hsF, "HIGH FREQ", "2k", "14k" } };
        for (int i = 0; i < 8; ++i)
            knob (d, ks[i].id, ks[i].t, ks[i].lo, ks[i].hi)->setBounds (636 + (i % 4) * 170 + 18, 46 + (i / 4) * 130, 134, 128);
    }
    // 4 : COMPRESSEUR (4 modeles)
    {
        auto& d = drawer[3];
        d.insets.push_back ({ "MODEL", { 16, 38, 290, 270 } });
        d.addAndMakeVisible (modelFace);
        modelFace.setBounds (318, 38, 940, 270);
        auto* ms = selector (d, p_cmpModel); ms->columns = 2; ms->setBounds (24, 60, 274, 74);
        lcdModel = new Lcd ({}, false, true); owned.emplace_back (lcdModel); d.addAndMakeVisible (lcdModel);
        lcdModel->setBounds (24, 140, 274, 80);
        toggle (d, p_cmpOn, "COMP ON")->setBounds (24, 228, 134, 32);
        toggle (d, p_cmpMS, "MID / SIDE")->setBounds (164, 228, 134, 32);
        cmpBar = new LedBar (LedBar::Reduction, 0.f, 24.f, "GR"); owned.emplace_back (cmpBar); d.addAndMakeVisible (cmpBar);
        cmpBar->setBounds (1272, 42, 40, 266);
        auto add = [this] (int m, juce::Component* c) { modelGroup[m].push_back (c); };
        auto row = [&] (int m, std::vector<juce::Component*> items)
        {
            const int n = (int) items.size(), w = juce::jmin (176, 920 / n), x0 = 318 + (940 - w * n) / 2;
            const int kw = juce::jmin (160, w - 14), kh = kw + 44, ky = 70 + (230 - kh) / 2;
            for (int i = 0; i < n; ++i) { items[(size_t) i]->setBounds (x0 + i * w + (w - kw) / 2, ky, kw, kh); add (m, items[(size_t) i]); }
        };
        add (0, toggle (d, p_cmpAuto, "AUTO GAIN"));                        modelGroup[0].back()->setBounds (24, 268, 134, 32);
        row (0, { knob (d, p_cmpThr, "THRESH", "-40", "0"), knob (d, p_cmpRat, "RATIO", "1.5", "20"), knob (d, p_cmpAtk, "ATTACK", "0.1", "100"),
                  knob (d, p_cmpRel, "RELEASE", "20", "1200"), knob (d, p_cmpKnee, "KNEE", "HARD", "SOFT"), knob (d, p_cmpSc, "SC HPF", "20", "400"),
                  knob (d, p_cmpMk, "MAKEUP", "-6", "+12") });
        row (1, { knob (d, p_vmIn, "INPUT", "-12", "+12"), knob (d, p_vmThr, "THRESHOLD", "0", "10"), rotary (d, p_vmTc, "TIME CONSTANT"),
                  knob (d, p_vmBal, "BALANCE", "-", "+"), knob (d, p_cmpMk, "OUTPUT", "-6", "+12") });
        row (2, { knob (d, p_fetIn, "INPUT", "-12", "+24"), knob (d, p_cmpMk, "OUTPUT", "-6", "+12"), knob (d, p_fetAtk, "ATTACK", "SLOW", "FAST"),
                  knob (d, p_fetRel, "RELEASE", "SLOW", "FAST"), rotary (d, p_fetRat, "RATIO") });
        row (3, { knob (d, p_dcThr, "THRESH dBu", "-20", "+10"), rotary (d, p_dcRat, "RATIO"), rotary (d, p_dcRec, "RECOVERY ms"),
                  knob (d, p_cmpMk, "GAIN", "-6", "+12") });
        for (int m = 0; m < 4; ++m)
            add (m, caption (d, juce::String (kModelNames[m]) + (m == 0 ? "  -  DIGITAL" : m == 1 ? "  -  TUBE LIMITER" : m == 2 ? "  -  FET LIMITER" : "  -  DIODE BRIDGE"),
                             { 348, 45, 600, 16 }, 12.f));
        if (auto* l = dynamic_cast<juce::Label*> (modelGroup[2].back())) l->setColour (juce::Label::textColourId, juce::Colour (0xff15171a));
        showModel ((int) proc.apvts.getRawParameterValue ("cmpModel")->load());
    }
    // 5 : LIMITEUR
    {
        auto& d = drawer[4];
        d.insets.push_back ({ "BRICKWALL  (LOOK-AHEAD, LAST STAGE)", { 16, 38, 640, 270 } });
        d.insets.push_back ({ "DIODE LIMITER  (BEFORE THE BRICKWALL)", { 668, 38, 590, 270 } });
        toggle (d, p_limOn, "LIMITER ON")->setBounds (500, 42, 146, 30);
        knob (d, p_limPush, "PUSH", "0", "+18")->setBounds (30, 84, 150, 200);
        knob (d, p_limCeil, "CEILING", "-6", "0")->setBounds (184, 84, 150, 200);
        knob (d, p_limRel, "RELEASE", "FAST", "SLOW")->setBounds (338, 84, 150, 200);
        knob (d, p_limClip, "CLIP", "OFF", "MAX")->setBounds (492, 84, 150, 200);
        toggle (d, p_dlOn, "DIODE ON")->setBounds (960, 42, 140, 30);
        toggle (d, p_dlFast, "FAST ATK")->setBounds (1106, 42, 140, 30);
        knob (d, p_dlThr, "LEVEL dBu", "+4", "+15")->setBounds (720, 84, 170, 200);
        rotary (d, p_dlRec, "RECOVERY ms")->setBounds (960, 84, 220, 200);
        limBar = new LedBar (LedBar::Reduction, 0.f, 12.f, "GR"); owned.emplace_back (limBar); d.addAndMakeVisible (limBar);
        limBar->setBounds (1272, 42, 40, 266);
    }

    tab = juce::jlimit (0, 4, (int) proc.apvts.state.getProperty ("uiTab", 3));
    showTab (tab);
    nameBtn.setButtonText (proc.presetName);
    startTimerHz (30);
}

ZenithContent::~ZenithContent() { stopTimer(); setLookAndFeel (nullptr); }

KnobUnit* ZenithContent::knob (juce::Component& parent, P id, const juce::String& text, const char* lo, const char* hi, double detent)
{
    auto k = std::make_unique<KnobUnit> (proc.apvts, paramDefs()[id].id, text, lo, hi);
    if (detent > -1e29) k->slider.detent = detent;
    parent.addAndMakeVisible (*k);
    if (byId[id] != nullptr) extra.push_back ({ k.get(), (int) id }); else byId[id] = k.get();
    knobs.push_back (std::move (k));
    return knobs.back().get();
}

KnobUnit* ZenithContent::rotary (juce::Component& parent, P id, const juce::String& text)
{
    auto* k = knob (parent, id, text);
    juce::StringArray names;
    for (auto* n : choiceNames (id)) names.add (n);
    k->setChoices (names);
    return k;
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

juce::Label* ZenithContent::caption (juce::Component& parent, const juce::String& text, juce::Rectangle<int> r, float size)
{
    auto* l = new juce::Label ({}, text);
    owned.emplace_back (l);
    l->setFont (font (size, size < 12.5f ? true : true));
    l->setColour (juce::Label::textColourId, look::print.withAlpha (0.85f));
    l->setJustificationType (juce::Justification::centredLeft);
    l->setInterceptsMouseClicks (false, false);
    l->setBorderSize ({});
    l->setMinimumHorizontalScale (1.f);
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
    if (lcdModel != nullptr) lcdModel->set (kModelText[m]);
    modelFace.setModel (m);
}

void ZenithContent::showTab (int t)
{
    tab = juce::jlimit (0, 4, t);
    for (int i = 0; i < 5; ++i) drawer[i].setVisible (i == tab);
    proc.apvts.state.setProperty ("uiTab", tab, nullptr);
}

void ZenithContent::cycleTheme()
{
    look::applyTheme ((look::themeIdx + 1) % look::kNumThemes);
    saveThemePref (look::themeIdx);
    laf.refresh();
    themeBtn.setButtonText (look::T().name);
    buildPanelTex();
    for (auto* c : getChildren()) { c->sendLookAndFeelChange(); }
    for (auto& o : owned) if (auto* l = dynamic_cast<juce::Label*> (o.get())) if (l->findColour (juce::Label::textColourId) != juce::Colour (0xff15171a)) l->setColour (juce::Label::textColourId, look::print.withAlpha (0.85f));
    repaint();
}

// ---------------------------------------------------------------- facade : panneau, flancs en bois, logo, grilles
void ZenithContent::buildPanelTex()
{
    const auto& th = look::T();
    const int W = designW, H = designH, cheek = 26;
    panelTex = juce::Image (juce::Image::RGB, W, H, false);
    juce::Graphics g (panelTex);
    g.setGradientFill (juce::ColourGradient (C (th.panelTop), 0.f, 0.f, C (th.panelBot), 0.f, (float) H, false));
    g.fillAll();
    juce::Random rr (4242);
    for (int y = 0; y < H; ++y)                                                       // brossage tres fin
    {
        const bool light = rr.nextFloat() < 0.5f;
        g.setColour ((light ? juce::Colours::white : juce::Colours::black).withAlpha (0.01f + 0.012f * rr.nextFloat()));
        g.drawHorizontalLine (y, 0.f, (float) W);
    }
    for (int k = 0; k < 3; ++k)                                                       // grandes bandes de lumiere
    {
        const float x0 = (float) W * (0.12f + 0.33f * (float) k);
        juce::Path band;
        band.addQuadrilateral (x0, 0.f, x0 + 200.f, 0.f, x0 + 60.f, (float) H, x0 - 140.f, (float) H);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.022f), x0, 0.f, juce::Colours::white.withAlpha (0.f), x0 + 200.f, 0.f, false));
        g.fillPath (band);
    }
    // flancs en bois
    drawWood (g, { 0, 0, cheek, H }, true);
    drawWood (g, { W - cheek, 0, cheek, H }, false);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRect (cheek, 0, 2, H); g.fillRect (W - cheek - 2, 0, 2, H);
    // vis de facade
    int k = 0;
    for (auto p : { juce::Point<float> (40.f, 16.f), { (float) W - 40.f, 16.f }, { 40.f, (float) H - 12.f }, { (float) W - 40.f, (float) H - 12.f } })
        drawScrew (g, p, 4.6f, k++ * 3);
    // logo chrome
    {
        const auto lr = juce::Rectangle<float> (52.f, 12.f, 270.f, 58.f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.setFont (font (54.f));
        g.drawText ("ZENITH", lr.translated (1.5f, 2.5f), juce::Justification::centredLeft, false);
        juce::ColourGradient chrome (juce::Colour (0xfffafbfd), 0.f, lr.getY() + 8.f, juce::Colour (0xff6d727b), 0.f, lr.getBottom() - 8.f, false);
        chrome.addColour (0.48, juce::Colour (0xffd5d9df));
        chrome.addColour (0.52, juce::Colour (0xff8b9099));
        chrome.addColour (0.8, juce::Colour (0xffc9cdd4));
        g.setGradientFill (chrome);
        g.drawText ("ZENITH", lr, juce::Justification::centredLeft, false);
        g.setColour (C (th.logo));
        g.fillRect (54, 72, 196, 3);
        g.setColour (look::print.withAlpha (0.85f));
        g.setFont (font (11.f));
        g.drawText (fr ("FOUDRE AUDIO  \xc2\xb7  MASTER BUS PROCESSOR"), 54, 80, 300, 14, juce::Justification::centredLeft, false);
        g.setColour (look::print.withAlpha (0.55f));
        g.setFont (font (9.5f, false));
        g.drawText ("OTT  /  EQ  /  VARI-MU  -  FET  -  DIODE  /  LIMITER", 54, 94, 340, 12, juce::Justification::centredLeft, false);
    }
    // grilles d'aeration
    drawVent (g, { 332.f, 24.f, 106.f, 66.f });
    drawVent (g, { 866.f, 24.f, 106.f, 66.f });
    // fenetre des lampes : lunette + fond (les lampes sont dessinees en direct)
    {
        const auto win = kTubeWin;
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (win.expanded (4.f).translated (0.f, 2.f), 10.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9ccd2), win.getX(), win.getY(), juce::Colour (0xff3e4146), win.getX(), win.getBottom(), false));
        g.fillRoundedRectangle (win.expanded (4.f), 9.f);
    }
    g.setColour (look::print.withAlpha (0.8f));
    g.setFont (font (10.f));
    g.drawText ("PRESET", 994, 12, 80, 14, juce::Justification::centredLeft, false);
    // filet sous l'en-tete
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRect (34, 103, W - 68, 2);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRect (34, 105, W - 68, 1);
    // fleches de la chaine entre les tuiles
    for (int i = 0; i < 4; ++i)
    {
        const float x = 34.f + (float) i * 270.f + 252.f + 9.f, y = 456.f + 28.f;
        juce::Path a;
        a.startNewSubPath (x - 4.f, y - 7.f); a.lineTo (x + 3.f, y); a.lineTo (x - 4.f, y + 7.f);
        g.setColour (look::cvRing.withAlpha (0.9f));
        g.strokePath (a, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void ZenithContent::paint (juce::Graphics& g)
{
    g.drawImageAt (panelTex, 0, 0);
    const auto win = kTubeWin;
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (win, 7.f);
        g.reduceClipRegion (clip);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff15110e), win.getCentreX(), win.getBottom(), juce::Colour (0xff030303), win.getCentreX(), win.getY(), false));
        g.fillRect (win);
        const float h = heatShown;
        g.setGradientFill (juce::ColourGradient (heatColour (0.6f).withAlpha (0.08f + 0.25f * h), win.getCentreX(), win.getBottom(),       // lueur sur le fond
                                                 juce::Colours::transparentBlack, win.getCentreX(), win.getY(), true));
        g.fillRect (win);
        struct Tb { float x, w, hgt, thr; bool big; };
        const Tb tubes[] = { { 488.f, 36.f, 74.f, 0.0f, false }, { 568.f, 36.f, 74.f, 0.1f, false }, { 650.f, 48.f, 84.f, 0.2f, true }, { 748.f, 48.f, 84.f, 0.32f, true } };
        int s = 0;
        for (const auto& t : tubes)
        {
            const float th = juce::jlimit (0.f, 1.f, (h - t.thr) / (1.f - t.thr));
            const float idle = 0.12f + 0.18f * proc.apvts.getRawParameterValue ("intensity")->load();
            drawTube (g, { t.x, win.getBottom() - t.hgt + 2.f, t.w, t.hgt }, juce::jmax (idle, th) * (1.f - 0.04f * flicker), t.big, s++);
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), win.getX(), win.getY(), juce::Colours::transparentWhite, win.getX(), win.getCentreY(), false));
        g.fillRect (win);                                                              // vitre
    }
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (win, 7.f, 1.5f);
}

// ---------------------------------------------------------------- animation
void ZenithContent::timerCallback()
{
    auto& m = proc.master;
    const float cg = m.cmpGr.load(), lg = m.limGr.load();
    float ott = 0.f;
    for (int b = 0; b < 3; ++b) ott = std::max (ott, m.bandDown[b].load());
    const float heat = juce::jlimit (0.f, 1.f, 0.45f * cg / 14.f + 0.3f * lg / 8.f + 0.25f * ott / 12.f);
    heatShown += (heat > heatShown ? 0.08f : 0.025f) * (heat - heatShown);           // une lampe chauffe et refroidit lentement
    flicker = rnd.nextFloat();
    repaint (kTubeWin.expanded (40.f, 10.f).toNearestInt());

    vu.setValue (cg); vu.tick();
    inBar->set (linToDb (m.inPk.load()));
    outBar->set (linToDb (m.outPk.load()));
    limBar->set (lg);
    cmpBar->set (cg);
    for (int b = 0; b < 3; ++b)
    {
        const float d = m.bandDown[b].load(), u = m.bandUp[b].load();
        bandDown[b]->set (d); bandUp[b]->set (u);
        lcdBand[b]->set (juce::String (d, 1) + " / " + juce::String (u, 1));
        actBar[b]->set (d);
        lcdAct[b]->set (d > 0.05f ? "-" + juce::String (d, 1) : "0.0");
    }
    actBar[3]->set (lg);
    lcdAct[3]->set (lg > 0.05f ? "-" + juce::String (lg, 1) : "0.0");
    auto lu = [] (float v) { return v <= -69.f ? juce::String ("-inf") : juce::String (v, 1); };
    lcdInS->set (lu (m.lufsIn.shortTerm.load()));
    lcdOutM->set (lu (m.lufsOut.momentary.load()));
    lcdOutS->set (lu (m.lufsOut.shortTerm.load()));
    const float li = m.lufsOut.integrated.load(), ls = m.lufsOut.shortTerm.load();
    lcdOutI->set (lu (li));
    lcdMatch->set (juce::String (m.matchDb.load(), 1));
    {
        static const float targets[4] { -14.f, -11.f, -9.f, -6.f };
        const float tgt = targets[juce::jlimit (0, 3, (int) std::lround (proc.apvts.getRawParameterValue ("lufsTgt")->load()))];
        const float ref = li > -69.f ? li : ls;
        if (ref <= -69.f) lcdDelta->set ("WAITING FOR SIGNAL");
        else
        {
            const float dlt = ref - tgt;
            lcdDelta->set (std::abs (dlt) < 0.5f ? juce::String ("ON TARGET")
                                                 : (dlt > 0 ? "+" : "") + juce::String (dlt, 1) + (dlt > 0 ? " LU  ABOVE TARGET" : " LU  BELOW TARGET"));
        }
    }

    const int model = (int) std::lround (proc.apvts.getRawParameterValue ("cmpModel")->load());
    showModel (model);

    // tuiles de la chaine
    {
        auto pv = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
        auto grs = [] (float v) { return v > 0.05f ? "-" + juce::String (v, 1) : juce::String ("0.0"); };
        const float inDb = pv ("in");
        tiles[0].setState (true, juce::String ("TRIM ") + (inDb > 0.05f ? "+" : "") + juce::String (inDb, 1) + " dB", lu (m.lufsIn.shortTerm.load()), tab == 0);
        tiles[1].setState (pv ("ottOn") > 0.5f, "DEPTH " + juce::String (juce::roundToInt (pv ("ottDepth") * 100.f)) + " %" + (pv ("ottMS") > 0.5f ? "  -  M/S" : ""), grs (ott), tab == 1);
        tiles[2].setState (pv ("eqOn") > 0.5f, "HPF " + juce::String (juce::roundToInt (pv ("eqHpf"))) + " Hz", "-", tab == 2);
        tiles[3].name = juce::String ("COMP  -  ") + kModelNames[juce::jlimit (0, 3, model)];
        tiles[3].setState (pv ("cmpOn") > 0.5f, pv ("cmpMS") > 0.5f ? "MID / SIDE" : "STEREO", grs (cg), tab == 3);
        tiles[4].setState (pv ("limOn") > 0.5f || pv ("dlOn") > 0.5f,
                           juce::String (pv ("dlOn") > 0.5f ? "DIODE + " : "") + "CEIL " + juce::String (pv ("limCeil"), 1) + " dB", grs (lg), tab == 4);
        tiles[3].repaint();
    }

    // macros : chaque potard deplace montre un arc jusqu'a la valeur reellement appliquee
    {
        float v[kNumP], e[kNumP];
        for (int i = 0; i < kNumP; ++i) v[i] = proc.apvts.getRawParameterValue (paramDefs()[i].id)->load();
        Master::applyMacros (v, e);
        eqCurve.setValues (e, v[p_eqOn] > 0.5f);
        auto upd = [&] (KnobUnit* k, int i)
        {
            const auto& d = paramDefs()[i];
            const bool moved = std::abs (e[i] - v[i]) > (d.mx - d.mn) * 0.004f;
            k->setMacroValue (moved, e[i]);
            k->setCvActive (moved);
        };
        for (int i = 0; i < kNumP; ++i)
        {
            if (byId[i] == nullptr || i == p_mDyn || i == p_mBri || i == p_mSq || paramDefs()[i].kind == 2) continue;
            upd (byId[i], i);
        }
        for (auto& [k, i] : extra)
            if (paramDefs()[i].kind != 2 && i != p_mDyn && i != p_mBri && i != p_mSq) upd (k, i);
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
    m.setLookAndFeel (&laf);
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
    float s = 1.f;
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
