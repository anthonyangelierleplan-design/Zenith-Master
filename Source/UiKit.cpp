#include "UiKit.h"

namespace ui
{
juce::String fr (const char* utf8) { return juce::String::fromUTF8 (utf8); }
juce::Font font (float h, bool bold) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }
static juce::Colour C (juce::uint32 c) { return juce::Colour (c); }

juce::Colour heatColour (float t)
{
    t = juce::jlimit (0.f, 1.f, t);
    const juce::Colour a (0xff1a1412), b (0xffa11a08), c (0xffff7a1a), d (0xffffe9b0);
    if (t < 0.4f)  return a.interpolatedWith (b, t / 0.4f);
    if (t < 0.75f) return b.interpolatedWith (c, (t - 0.4f) / 0.35f);
    return c.interpolatedWith (d, (t - 0.75f) / 0.25f);
}

// petite vis cruciforme polie
void drawScrew (juce::Graphics& g, juce::Point<float> p, float r, int seed)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (p.x - r - 0.5f, p.y - r + 0.9f, r * 2.f + 1.f, r * 2.f + 1.f);
    juce::ColourGradient gr (juce::Colour (0xfff2f3f5), p.x - r * 0.7f, p.y - r * 0.8f, juce::Colour (0xff55585e), p.x + r * 0.8f, p.y + r * 0.9f, false);
    gr.addColour (0.55, juce::Colour (0xffa9adb4));
    g.setGradientFill (gr);
    g.fillEllipse (p.x - r, p.y - r, r * 2.f, r * 2.f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawEllipse (p.x - r, p.y - r, r * 2.f, r * 2.f, 0.7f);
    const float a = 0.35f + (float) (seed % 7) * 0.31f, s = std::sin (a) * r * 0.62f, c = std::cos (a) * r * 0.62f;
    g.setColour (juce::Colour (0xff24272c).withAlpha (0.9f));
    g.drawLine (p.x - c, p.y - s, p.x + c, p.y + s, 1.1f);
    g.drawLine (p.x + s, p.y - c, p.x - s, p.y + c, 1.1f);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.fillEllipse (p.x - r * 0.55f, p.y - r * 0.7f, r * 0.5f, r * 0.35f);
}

// plaque de module vissee : metal brosse fin, biseau, reflet, quatre vis
void drawPlate (juce::Graphics& g, juce::Rectangle<float> b, bool screws)
{
    const auto& th = look::T();
    const auto r = b.reduced (1.f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));                                         // ombre portee
    g.fillRoundedRectangle (r.translated (0.f, 3.f), 6.f);
    g.setGradientFill (juce::ColourGradient (C (th.plateTop), r.getX(), r.getY(), C (th.plateBot), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 6.f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (r.reduced (2.f).toNearestInt());
        juce::Random rr ((juce::int64) (r.getWidth() * 7 + r.getHeight()));
        for (float y = r.getY() + 2.f; y < r.getBottom() - 2.f; y += 1.5f)                        // brossage horizontal
        {
            const bool light = rr.nextFloat() < 0.5f;
            g.setColour ((light ? juce::Colours::white : juce::Colours::black).withAlpha (0.012f + 0.022f * rr.nextFloat()));
            g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(),                // reflet doux
                                                 juce::Colours::white.withAlpha (0.f), r.getX() + r.getWidth() * 0.5f, r.getBottom(), false));
        g.fillRect (r);
    }
    g.setColour (juce::Colours::white.withAlpha (0.13f));                                         // biseau
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 6.f, r.getRight() - 6.f);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (r, 6.f, 1.2f);
    if (screws && r.getWidth() > 60.f && r.getHeight() > 40.f)
    {
        int k = 0;
        for (auto p : { juce::Point<float> (r.getX() + 9.f, r.getY() + 9.f), { r.getRight() - 9.f, r.getY() + 9.f },
                        { r.getX() + 9.f, r.getBottom() - 9.f }, { r.getRight() - 9.f, r.getBottom() - 9.f } })
            drawScrew (g, p, 3.8f, (int) (r.getX() + r.getY()) + k++);
    }
}

void drawInset (juce::Graphics& g, juce::Rectangle<float> b)
{
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.fillRoundedRectangle (b, 5.f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.45f), 0.f, b.getY(), juce::Colours::transparentBlack, 0.f, b.getY() + 7.f, false));
    g.fillRoundedRectangle (b, 5.f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine ((int) b.getBottom() - 1, b.getX() + 5.f, b.getRight() - 5.f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (b, 5.f, 1.f);
}

// grille d'aeration : fentes embouties
void drawVent (juce::Graphics& g, juce::Rectangle<float> b)
{
    const int rows = juce::jmax (2, (int) (b.getHeight() / 9.f));
    const float h = 4.2f, gap = (b.getHeight() - (float) rows * h) / (float) (rows - 1);
    for (int i = 0; i < rows; ++i)
    {
        const auto s = juce::Rectangle<float> (b.getX(), b.getY() + (float) i * (h + gap), b.getWidth(), h);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRoundedRectangle (s.translated (0.f, 1.2f), h * 0.5f);
        g.setColour (juce::Colour (0xff030303));
        g.fillRoundedRectangle (s, h * 0.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black, 0.f, s.getY(), juce::Colour (0xff1c1c1e), 0.f, s.getBottom(), false));
        g.fillRoundedRectangle (s.reduced (1.f, 0.8f), h * 0.4f);
    }
}

// touche : en relief au repos, enfoncee et eclairee (accent) quand elle est active
void drawKey (juce::Graphics& g, juce::Rectangle<float> r, bool pressed, bool hover)
{
    const auto& th = look::T();
    const juce::Colour hi (th.keyHi), lo (th.keyLo), ac (th.accent);
    if (! pressed)
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0.f, 1.8f), 4.f);
        g.setGradientFill (juce::ColourGradient (hover ? hi.brighter (0.08f) : hi, r.getX(), r.getY(), lo, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (juce::Colours::white.withAlpha (0.13f));
        g.drawHorizontalLine ((int) (r.getY() + 1.f), r.getX() + 4.f, r.getRight() - 4.f);
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.drawRoundedRectangle (r, 4.f, 1.f);
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (ac.brighter (0.25f), r.getX(), r.getY(), ac.darker (0.45f), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r.translated (0.f, 0.8f), 4.f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.35f), r.getX(), r.getY(), juce::Colours::black.withAlpha (0.f), r.getX(), r.getY() + 6.f, false));
        g.fillRoundedRectangle (r.translated (0.f, 0.8f), 4.f);
        g.setColour (ac.darker (0.7f));
        g.drawRoundedRectangle (r.translated (0.f, 0.8f), 4.f, 1.f);
    }
}

// voyant LED en relief
void drawJewel (juce::Graphics& g, juce::Rectangle<float> r, bool on, juce::Colour ledCol)
{
    if (on)
    {
        g.setGradientFill (juce::ColourGradient (ledCol.withAlpha (0.55f), r.getCentreX(), r.getCentreY(), ledCol.withAlpha (0.f), r.getCentreX(), r.getY() - r.getHeight() * 0.9f, true));
        g.fillEllipse (r.expanded (r.getWidth() * 1.1f));
    }
    g.setGradientFill (juce::ColourGradient (on ? ledCol.brighter (0.8f) : ledCol.withSaturation (0.3f).withMultipliedBrightness (0.35f), r.getX() + r.getWidth() * 0.3f, r.getY() + r.getHeight() * 0.25f,
                                             on ? ledCol.darker (0.4f) : juce::Colour (0xff141416), r.getRight(), r.getBottom(), true));
    g.fillEllipse (r);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (r, 0.9f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.8f : 0.25f));
    g.fillEllipse (r.getX() + r.getWidth() * 0.22f, r.getY() + r.getHeight() * 0.14f, r.getWidth() * 0.34f, r.getHeight() * 0.26f);
}

// potard 3D : ombre portee, jupe moletee, chapeau metal tourne avec reflets, repere
void drawKnobBody (juce::Graphics& g, juce::Point<float> c, float kr, float angle, float alpha)
{
    const auto& th = look::T();
    for (int i = 0; i < 5; ++i)                                                       // ombre portee douce
    {
        const float e = (float) i * 1.4f;
        g.setColour (juce::Colours::black.withAlpha (0.13f * alpha));
        g.fillEllipse (c.x - kr - e + 1.5f, c.y - kr - e + 4.f + (float) i * 0.7f, (kr + e) * 2.f, (kr + e) * 2.f);
    }
    juce::ColourGradient sk (C (th.skirtHi), c.x - kr * 0.7f, c.y - kr * 0.9f, C (th.skirtLo), c.x + kr * 0.6f, c.y + kr, false);
    g.setGradientFill (sk);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
    const int teeth = juce::jlimit (24, 64, (int) (kr * 2.2f));
    for (int i = 0; i < teeth; ++i)                                                   // moletage
    {
        const float a = (float) i / (float) teeth * juce::MathConstants<float>::twoPi, s = std::sin (a), co = std::cos (a);
        const float lit = std::max (0.f, -co * 0.6f - s * 0.4f);
        g.setColour (juce::Colours::white.withAlpha ((0.04f + 0.11f * lit) * alpha));
        g.drawLine (c.x + kr * 0.80f * s, c.y - kr * 0.80f * co, c.x + kr * 0.98f * s, c.y - kr * 0.98f * co, 1.f);
        g.setColour (juce::Colours::black.withAlpha (0.25f * alpha));
        g.drawLine (c.x + kr * 0.80f * std::sin (a + 0.03f), c.y - kr * 0.80f * std::cos (a + 0.03f), c.x + kr * 0.98f * std::sin (a + 0.03f), c.y - kr * 0.98f * std::cos (a + 0.03f), 0.6f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.2f * alpha));
    juce::Path rim; rim.addCentredArc (c.x, c.y, kr - 0.6f, kr - 0.6f, 0.f, -2.4f, 0.5f, true);
    g.strokePath (rim, juce::PathStrokeType (1.f));
    const float cr = kr * 0.68f;
    g.setColour (juce::Colours::black.withAlpha (0.6f * alpha));                      // epaulement sous le chapeau
    g.fillEllipse (c.x - cr - 1.2f, c.y - cr, cr * 2.f + 2.4f, cr * 2.f + 2.6f);
    juce::ColourGradient cap (C (th.capHi), c.x - cr * 0.7f, c.y - cr * 0.85f, C (th.capLo), c.x + cr * 0.8f, c.y + cr * 0.95f, false);
    cap.addColour (0.45, C (th.capMid));
    g.setGradientFill (cap);
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);
        g.reduceClipRegion (clip);
        for (int i = 1; i < 10; ++i)                                                  // tournage concentrique
        {
            const float r2 = cr * (float) i / 10.f;
            g.setColour ((i % 2 ? juce::Colours::white : juce::Colours::black).withAlpha (0.045f));
            g.drawEllipse (c.x - r2, c.y - r2, r2 * 2.f, r2 * 2.f, 0.8f);
        }
        for (float a0 : { -1.25f, 1.9f })                                             // reflets anisotropes du metal tourne
        {
            juce::Path w; w.addPieSegment (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f, a0, a0 + 0.62f, 0.f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.02f), c.x, c.y, juce::Colours::white.withAlpha (0.45f * alpha), c.x + cr, c.y, true));
            g.fillPath (w);
        }
    }
    g.setColour (juce::Colours::black.withAlpha (0.35f * alpha));
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f, 0.8f);
    const auto xf = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
    juce::Path p; p.addRoundedRectangle (-1.5f, -kr * 0.98f, 3.f, kr * 0.36f, 1.2f);  // repere sur la jupe
    g.setColour (C (th.pointer).withAlpha (alpha));
    g.fillPath (p, xf);
    juce::Path d; d.addRoundedRectangle (-1.3f, -cr * 0.9f, 2.6f, cr * 0.42f, 1.f);   // trait grave sur le chapeau
    g.setColour (juce::Colours::black.withAlpha (0.55f * alpha));
    g.fillPath (d, xf);
}

void drawLcdBox (juce::Graphics& g, juce::Rectangle<float> b)
{
    const auto& th = look::T();
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (b.translated (0.f, 1.f), 4.f, 1.f);
    g.setColour (C (th.lcdBg));
    g.fillRoundedRectangle (b, 4.f);
    g.setGradientFill (juce::ColourGradient (C (th.lcdText).withAlpha (0.08f), b.getCentreX(), b.getCentreY(), juce::Colours::transparentBlack, b.getX(), b.getY(), true));
    g.fillRoundedRectangle (b, 4.f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.7f), 0.f, b.getY(), juce::Colours::transparentBlack, 0.f, b.getY() + 6.f, false));
    g.fillRoundedRectangle (b, 4.f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (b, 4.f, 1.2f);
}

// lampe : verre, getter argente, plaques, mica, filament qui rougit, socle bakelite
void drawTube (juce::Graphics& g, juce::Rectangle<float> r, float heat, bool big, int seed)
{
    heat = juce::jlimit (0.f, 1.f, heat);
    auto base = r.removeFromBottom (r.getHeight() * 0.14f);
    const auto glass = r.withTrimmedBottom (-4.f);
    const float w = glass.getWidth(), cx = glass.getCentreX();
    const auto glowCol = heatColour (0.45f + 0.55f * heat);
    // halo chaud autour de la lampe
    g.setGradientFill (juce::ColourGradient (glowCol.withAlpha (0.10f + 0.40f * heat), cx, glass.getBottom() - glass.getHeight() * 0.35f,
                                             glowCol.withAlpha (0.f), cx, glass.getY() - glass.getHeight() * 0.25f, true));
    g.fillEllipse (glass.expanded (w * 0.85f, glass.getHeight() * 0.2f));
    juce::Path env;
    env.addRoundedRectangle (glass.getX(), glass.getY(), w, glass.getHeight(), w * 0.5f, w * 0.5f, true, true, false, false);
    g.setColour (juce::Colour (0xff0d0f12).withAlpha (0.9f));
    g.fillPath (env);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (env);
        // getter : depot argente en haut du verre
        const auto top = glass.withHeight (glass.getHeight() * (big ? 0.2f : 0.24f));
        juce::ColourGradient gt (juce::Colour (0xffd9dde3), top.getX() + w * 0.25f, top.getY() + 3.f, juce::Colour (0xff2b2e33), top.getRight(), top.getBottom(), false);
        gt.addColour (0.5, juce::Colour (0xff8a9099));
        g.setGradientFill (gt);
        g.fillRect (top);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.f), 0.f, top.getBottom() - 6.f, juce::Colour (0xff0d0f12), 0.f, top.getBottom() + 2.f, false));
        g.fillRect (top.withTrimmedTop (top.getHeight() - 8.f).expanded (0.f, 2.f));
        // micas
        const float m1 = glass.getY() + glass.getHeight() * (big ? 0.24f : 0.28f), m2 = glass.getBottom() - glass.getHeight() * 0.16f;
        g.setColour (juce::Colour (0xffb9b4a6).withAlpha (0.55f));
        g.fillRect (glass.getX() + w * 0.12f, m1, w * 0.76f, 2.2f);
        g.fillRect (glass.getX() + w * 0.12f, m2, w * 0.76f, 2.2f);
        // plaques (double triode : deux boites cote a cote)
        const float ph = m2 - m1 - 6.f, pw = w * 0.3f;
        for (int k = 0; k < 2; ++k)
        {
            const auto pl = juce::Rectangle<float> (cx + (k == 0 ? -pw - w * 0.04f : w * 0.04f), m1 + 3.f, pw, ph);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6b6e74), pl.getX(), pl.getY(), juce::Colour (0xff1e2024), pl.getRight(), pl.getY(), false));
            g.fillRect (pl);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            for (float x = pl.getX() + 2.5f; x < pl.getRight() - 1.f; x += 3.5f) g.drawVerticalLine ((int) x, pl.getY() + 3.f, pl.getBottom() - 3.f);
            // lueur rouge-orange qui filtre entre les plaques quand ca chauffe
            const auto slit = pl.withX (k == 0 ? pl.getRight() - 1.5f : pl.getX() - 1.5f).withWidth (3.f).reduced (0.f, ph * 0.12f);
            g.setColour (heatColour (0.35f + 0.65f * heat).withAlpha (0.35f + 0.6f * heat));
            g.fillRect (slit);
        }
        // filament : point incandescent en bas des plaques
        const float fy = m2 - 5.f;
        g.setGradientFill (juce::ColourGradient (heatColour (0.55f + 0.45f * heat).withAlpha (0.95f), cx, fy, heatColour (0.5f).withAlpha (0.f), cx + w * 0.3f, fy, true));
        g.fillEllipse (cx - w * 0.3f, fy - 6.f, w * 0.6f, 12.f);
        g.setColour (heatColour (0.6f + 0.4f * heat));
        g.fillEllipse (cx - 2.5f, fy - 1.5f, 5.f, 3.f);
        // tiges de support
        g.setColour (juce::Colour (0xff9aa0a8).withAlpha (0.55f));
        g.drawVerticalLine ((int) (glass.getX() + w * 0.16f), m1, m2 + 8.f);
        g.drawVerticalLine ((int) (glass.getRight() - w * 0.16f), m1, m2 + 8.f);
        // reflets du verre
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.42f), glass.getX() + w * 0.12f, 0.f,
                                                 juce::Colours::white.withAlpha (0.f), glass.getX() + w * 0.32f, 0.f, false));
        g.fillRect (glass.getX() + w * 0.1f, glass.getY() + w * 0.25f, w * 0.22f, glass.getHeight() * 0.8f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRect (glass.getRight() - w * 0.16f, glass.getY() + w * 0.4f, w * 0.05f, glass.getHeight() * 0.6f);
        juce::ignoreUnused (seed);
    }
    g.setColour (juce::Colour (0xffc9ced6).withAlpha (0.35f));
    g.strokePath (env, juce::PathStrokeType (1.1f));
    // socle bakelite + bague metal
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb8bcc2), base.getX(), base.getY(), juce::Colour (0xff3a3d42), base.getX(), base.getY() + 4.f, false));
    g.fillRect (base.getX() - 1.f, base.getY() - 1.f, base.getWidth() + 2.f, 4.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2725), base.getX(), base.getY(), juce::Colour (0xff060606), base.getX(), base.getBottom(), false));
    g.fillRoundedRectangle (base.expanded (4.f, 0.f).withTrimmedTop (3.f), 3.f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawHorizontalLine ((int) base.getY() + 4, base.getX() - 3.f, base.getRight() + 3.f);
}

// flanc en bois verni (veinage procedural)
void drawWood (juce::Graphics& g, juce::Rectangle<int> rect, bool left)
{
    const auto& th = look::T();
    const int W = rect.getWidth(), H = rect.getHeight();
    const juce::Colour a (th.woodA), b (th.woodB);
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (rect);
    g.setOrigin (rect.getPosition());
    g.setGradientFill (juce::ColourGradient (a, 0.f, 0.f, b, (float) W, 0.f, false));
    g.fillRect (0, 0, W, H);
    juce::Random rr (left ? 1234 : 5678);
    for (int k = 0; k < 30; ++k)
    {
        const float x0 = rr.nextFloat() * (float) W, amp = 0.6f + rr.nextFloat() * 2.2f, freq = 0.004f + rr.nextFloat() * 0.01f, ph = rr.nextFloat() * 6.28f;
        juce::Path p;
        for (int y = 0; y <= H; y += 6)
        {
            const float x = x0 + amp * std::sin ((float) y * freq + ph) + 1.2f * std::sin ((float) y * freq * 3.1f + ph * 2.f);
            if (y == 0) p.startNewSubPath (x, (float) y); else p.lineTo (x, (float) y);
        }
        const bool dark = rr.nextFloat() < 0.6f;
        g.setColour ((dark ? b.darker (0.5f) : a.brighter (0.25f)).withAlpha (0.18f + 0.25f * rr.nextFloat()));
        g.strokePath (p, juce::PathStrokeType (0.6f + rr.nextFloat() * 1.6f));
    }
    for (int k = 0; k < 3; ++k)
    {
        const float cx = (float) W * (0.25f + 0.5f * rr.nextFloat()), cy = (float) H * rr.nextFloat();
        for (int r = 0; r < 5; ++r)
        {
            g.setColour (b.darker (0.4f).withAlpha (0.18f));
            g.drawEllipse (cx - 2.f - (float) r * 1.6f, cy - 9.f - (float) r * 7.f, 4.f + (float) r * 3.2f, 18.f + (float) r * 14.f, 0.8f);
        }
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.18f), left ? (float) W * 0.3f : (float) W * 0.7f, 0.f,
                                             juce::Colours::white.withAlpha (0.f), left ? (float) W : 0.f, 0.f, false));
    g.fillRect (0, 0, W, H);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawVerticalLine (left ? W - 1 : 0, 0.f, (float) H);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawVerticalLine (left ? 0 : W - 1, 0.f, (float) H);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.35f), 0.f, 0.f, juce::Colours::transparentBlack, 0.f, 26.f, false));
    g.fillRect (0, 0, W, 26);
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, 0.f, (float) H - 26.f, juce::Colours::black.withAlpha (0.35f), 0.f, (float) H, false));
    g.fillRect (0, H - 26, W, 26);
}
} // namespace ui
using namespace ui;

// ================================================================ look & feel
OrageLookAndFeel::OrageLookAndFeel() { refresh(); }

void OrageLookAndFeel::refresh()
{
    const auto& th = look::T();
    const juce::Colour bg (th.plateBot), ac (th.accent);
    setColour (juce::ComboBox::backgroundColourId, C (th.lcdBg));
    setColour (juce::ComboBox::textColourId, look::print);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
    setColour (juce::ComboBox::arrowColourId, look::print);
    setColour (juce::PopupMenu::backgroundColourId, bg.darker (0.3f));
    setColour (juce::PopupMenu::textColourId, look::print);
    setColour (juce::PopupMenu::headerTextColourId, ac);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ac.darker (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, C (th.keyHi));
    setColour (juce::TextButton::buttonOnColourId, ac);
    setColour (juce::TextButton::textColourOffId, look::print);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ScrollBar::thumbColourId, look::print.withAlpha (0.4f));
    setColour (juce::TextEditor::backgroundColourId, C (th.lcdBg));
    setColour (juce::TextEditor::textColourId, C (th.lcdText));
    setColour (juce::TextEditor::outlineColourId, juce::Colours::black);
    setColour (juce::TextEditor::focusedOutlineColourId, ac);
    setColour (juce::TooltipWindow::backgroundColourId, C (th.lcdBg));
    setColour (juce::TooltipWindow::textColourId, C (th.lcdText));
    setColour (juce::TooltipWindow::outlineColourId, ac.withAlpha (0.6f));
    setColour (juce::BubbleComponent::backgroundColourId, C (th.lcdBg));
    setColour (juce::BubbleComponent::outlineColourId, ac.withAlpha (0.6f));
    setColour (juce::Slider::textBoxTextColourId, C (th.lcdText));
}

void OrageLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& s)
{
    const auto c = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
    const bool small = (bool) s.getProperties()["choices"];
    const float r = (float) juce::jmin (w, h) * 0.5f, kr = r * (small ? 0.56f : 0.66f);
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float alpha = s.isEnabled() ? 1.f : 0.5f;
    if (s.isEnabled() && ! small)                                                       // arc lumineux de la valeur (depuis le cran)
    {
        const DetentSlider* ds = dynamic_cast<const DetentSlider*> (&s);
        const bool bip = ds != nullptr && ds->detent >= s.getMinimum() && ds->detent <= s.getMaximum();
        const float a0 = bip ? startAngle + (float) s.valueToProportionOfLength (ds->detent) * (endAngle - startAngle) : startAngle;
        juce::Path track; track.addCentredArc (c.x, c.y, kr + 5.f, kr + 5.f, 0.f, startAngle, endAngle, true);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.strokePath (track, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (std::abs (angle - a0) > 0.01f)
        {
            juce::Path arc; arc.addCentredArc (c.x, c.y, kr + 5.f, kr + 5.f, 0.f, juce::jmin (a0, angle), juce::jmax (a0, angle), true);
            g.setColour (look::cvRing.withAlpha (0.25f));
            g.strokePath (arc, juce::PathStrokeType (5.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (look::cvRing.withAlpha (0.95f));
            g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
    drawKnobBody (g, c, kr, angle, alpha);
}

void OrageLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    drawKey (g, b.getLocalBounds().toFloat().reduced (1.5f), down || b.getToggleState(), over);
}

void OrageLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setColour ((down || b.getToggleState()) ? juce::Colours::white : look::print.withAlpha (0.95f));
    g.setFont (font (juce::jmin (13.f, (float) b.getHeight() * 0.48f)));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 2), juce::Justification::centred, 1);
}

// ================================================================ potard
KnobUnit::KnobUnit (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& t,
                    const juce::String& lo, const juce::String& hi, int cvIdx)
    : text (t), loText (lo), hiText (hi), cvIndex (cvIdx), attachmentParam (apvts.getParameter (id)), attachment (apvts, id, slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setScrollWheelEnabled (false);
    slider.getProperties().set ("seed", id.hashCode());
    if (auto* prm = apvts.getParameter (id))
        slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0) slider.detent = 0.0;           // cran au zero des reglages +/-
    addAndMakeVisible (slider);
}

void KnobUnit::setChoices (const juce::StringArray& names)
{
    choices = names;
    slider.getProperties().set ("choices", true);
    slider.setMouseDragSensitivity (120);
    slider.setPopupDisplayEnabled (false, false, nullptr);
    repaint();
}

void KnobUnit::resized() { slider.setBounds (getLocalBounds().withTrimmedBottom (choices.size() > 1 ? 18 : 24)); }

void KnobUnit::setCvActive (bool a) { if (a != cvActive) { cvActive = a; repaint(); } }

void KnobUnit::setMacroValue (bool active, double effective)
{
    const double range = slider.getMaximum() - slider.getMinimum();
    if (active == macroOn && (! active || std::abs (effective - macroVal) < range * 0.0015)) return;
    macroOn = active; macroVal = effective;
    if (active)
    {
        if (auto* prm = dynamic_cast<juce::RangedAudioParameter*> (attachmentParam))
            slider.setTooltip ("Macros : " + prm->getText (prm->convertTo0to1 ((float) slider.getValue()), 0) + "  ->  "
                        + prm->getText (prm->convertTo0to1 ((float) juce::jlimit (slider.getMinimum(), slider.getMaximum(), effective)), 0));
    }
    else slider.setTooltip ({});
    repaint();
}

void KnobUnit::paint (juce::Graphics& g)
{
    const auto sb = slider.getBounds().toFloat();
    const auto c = sb.getCentre();
    const float r = juce::jmin (sb.getWidth(), sb.getHeight()) * 0.5f;
    const float sa = juce::MathConstants<float>::pi * 1.2f, ea = juce::MathConstants<float>::pi * 2.8f;
    const float alpha = slider.isEnabled() ? 1.f : 0.45f;
    const bool big = getWidth() >= 120;
    if (choices.size() > 1)                                                         // commutateur : libelles autour
    {
        const int n = choices.size(), sel = juce::roundToInt (slider.getValue());
        const float fs = juce::jlimit (9.f, 12.f, r * 0.2f);
        for (int i = 0; i < n; ++i)
        {
            const float a = sa + (float) i / (float) (n - 1) * (ea - sa);
            const float r1 = r * 0.6f, r2 = r * 0.68f;
            g.setColour (look::print.withAlpha (0.7f * alpha));
            g.drawLine (c.x + r1 * std::sin (a), c.y - r1 * std::cos (a), c.x + r2 * std::sin (a), c.y - r2 * std::cos (a), 1.2f);
            const float rt = r * 0.86f;
            g.setColour (i == sel ? look::cvRing : look::print.withAlpha (0.85f * alpha));
            g.setFont (font (fs));
            g.drawText (choices[i], (int) (c.x + rt * std::sin (a)) - 20, (int) (c.y - rt * std::cos (a)) - 7, 40, 14, juce::Justification::centred, false);
        }
    }
    else
    {
        for (int i = 0; i <= 10; ++i)                                                   // graduation
        {
            const float a = sa + (float) i / 10.f * (ea - sa), rad = r * 0.94f, dot = (i % 5 == 0) ? 3.2f : 2.f;
            g.setColour (look::print.withAlpha (0.75f * alpha));
            g.fillEllipse (c.x + rad * std::sin (a) - dot * 0.5f, c.y - rad * std::cos (a) - dot * 0.5f, dot, dot);
        }
        g.setFont (font (big ? 11.f : 9.5f));
        g.setColour (look::print.withAlpha (0.72f * alpha));
        const int lw = juce::jmin (60, getWidth() / 2 - 1), ly = (int) (c.y + r * 0.8f);
        g.drawText (loText, juce::jmax (0, (int) (c.x - r * 0.78f) - lw / 2), ly, lw, 12, juce::Justification::centred, false);
        g.drawText (hiText, juce::jmin (getWidth() - lw, (int) (c.x + r * 0.78f) - lw / 2), ly, lw, 12, juce::Justification::centred, false);
        if (slider.detent >= slider.getMinimum() && slider.detent <= slider.getMaximum())               // repere du cran
        {
            const float pos = (float) slider.valueToProportionOfLength (slider.detent);
            const float a = sa + pos * (ea - sa), rad = r * 0.94f;
            g.setColour (look::cvRing.withAlpha (0.95f * alpha));
            g.fillEllipse (c.x + rad * std::sin (a) - 2.5f, c.y - rad * std::cos (a) - 2.5f, 5.f, 5.f);
        }
        if (macroOn)                                                                    // ce que les macros font reellement a ce reglage
        {
            const double mn = slider.getMinimum(), mx = slider.getMaximum();
            const float p0 = (float) slider.valueToProportionOfLength (slider.getValue());
            const float p1 = (float) slider.valueToProportionOfLength (juce::jlimit (mn, mx, macroVal));
            const bool beyond = macroVal < mn || macroVal > mx;
            const float a0 = sa + p0 * (ea - sa), a1 = sa + p1 * (ea - sa), rad = r * 0.8f;
            juce::Path arc;
            arc.addCentredArc (c.x, c.y, rad, rad, 0.f, juce::jmin (a0, a1), juce::jmax (a0, a1), true);
            g.setColour (juce::Colours::white.withAlpha (0.25f * alpha));
            g.strokePath (arc, juce::PathStrokeType (5.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (juce::Colours::white.withAlpha (0.9f * alpha));
            g.strokePath (arc, juce::PathStrokeType (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            const juce::Point<float> tip (c.x + rad * std::sin (a1), c.y - rad * std::cos (a1));
            g.setColour (juce::Colours::white.withAlpha (alpha));
            if (beyond) g.drawEllipse (tip.x - 3.5f, tip.y - 3.5f, 7.f, 7.f, 1.6f);
            else        g.fillEllipse (tip.x - 3.f, tip.y - 3.f, 6.f, 6.f);
        }
    }
    g.setFont (font (big ? 14.f : 11.5f));
    g.setColour (look::print.withAlpha (0.97f * alpha));
    g.drawText (text, -6, getHeight() - 17, getWidth() + 12, 16, juce::Justification::centred, false);
}

// ================================================================ touches / interrupteurs
SegmentSelector::SegmentSelector (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, juce::StringArray names)
    : labels (std::move (names)),
      attachment (*apvts.getParameter (id), [this] (float v) { selected = juce::roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

void SegmentSelector::paint (juce::Graphics& g)
{
    const int n = labels.size(), cols = columns > 0 ? columns : n, rows = (n + cols - 1) / cols;
    const float w = (float) getWidth() / (float) cols, h = (float) getHeight() / (float) rows;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (w * (float) (i % cols), h * (float) (i / cols), w, h).reduced (2.5f);
        const bool on = (i == selected);
        drawKey (g, r, on);
        g.setColour (on ? juce::Colours::white : look::print.withAlpha (0.85f));
        g.setFont (font (juce::jmin (13.f, r.getHeight() * 0.42f)));
        g.drawFittedText (labels[i], r.toNearestInt().reduced (3, 1).translated (0, on ? 1 : 0), juce::Justification::centred, 1);
    }
}

void SegmentSelector::mouseDown (const juce::MouseEvent& e)
{
    const int n = labels.size(), cols = columns > 0 ? columns : n, rows = (n + cols - 1) / cols;
    const int cx = juce::jlimit (0, cols - 1, (int) (e.position.x / ((float) getWidth() / (float) cols)));
    const int cy = juce::jlimit (0, rows - 1, (int) (e.position.y / ((float) getHeight() / (float) rows)));
    attachment.setValueAsCompleteGesture ((float) juce::jlimit (0, n - 1, cy * cols + cx));
}

void LedToggle::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (2.f);
    const bool on = getToggleState();
    drawKey (g, down ? r.translated (0.f, 0.6f) : r, false, over);
    drawJewel (g, juce::Rectangle<float> (9.f, 9.f).withCentre ({ r.getX() + 12.f, r.getCentreY() }), on, juce::Colour (look::T().led));
    g.setColour (on ? look::print : look::print.withAlpha (0.65f));
    g.setFont (font (juce::jmin (12.5f, r.getHeight() * 0.48f)));
    g.drawFittedText (getButtonText(), r.withTrimmedLeft (22.f).toNearestInt(), juce::Justification::centredLeft, 1);
}

// ================================================================ vumetre : boitier, cadran retroeclaire, aiguille a contrepoids
void VuMeter::tick()
{
    shown += (target > shown ? 0.45f : 0.12f) * (target - shown);
    if (std::fabs (shown - lastDrawn) > 0.03f) { lastDrawn = shown; repaint(); }
}

void VuMeter::paint (juce::Graphics& g)
{
    const auto& th = look::T();
    const auto outer = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (outer.translated (0.f, 3.f), 8.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9ccd2), outer.getX(), outer.getY(), juce::Colour (0xff3e4146), outer.getX(), outer.getBottom(), false));
    g.fillRoundedRectangle (outer, 8.f);                                               // lunette chromee
    const auto b = outer.reduced (5.f);
    g.setColour (juce::Colour (0xff0b0b0c));
    g.fillRoundedRectangle (b.expanded (1.f), 6.f);
    juce::Graphics::ScopedSaveState ss (g);
    juce::Path clip; clip.addRoundedRectangle (b, 5.f);
    g.reduceClipRegion (clip);
    const juce::Colour face (th.vuFace), ink (th.vuInk);
    g.setGradientFill (juce::ColourGradient (face.brighter (0.08f), b.getCentreX(), b.getBottom() - b.getHeight() * 0.1f,       // retroeclairage chaud
                                             face.darker (0.35f), b.getX(), b.getY(), true));
    g.fillRect (b);

    const juce::Point<float> pivot (b.getCentreX(), b.getBottom() + b.getHeight() * 0.18f);
    const float R = b.getHeight() * 0.95f, a0 = -0.72f, a1 = 0.72f;
    auto dir = [] (float a) { return juce::Point<float> (std::sin (a), -std::cos (a)); };
    {
        juce::Path arc; arc.addCentredArc (pivot.x, pivot.y, R, R, 0.f, a0, a1, true);
        g.setColour (ink); g.strokePath (arc, juce::PathStrokeType (1.4f));
        juce::Path red; red.addCentredArc (pivot.x, pivot.y, R + 3.f, R + 3.f, 0.f, a0 + (a1 - a0) * 0.75f, a1, true);
        g.setColour (juce::Colour (0xffb3261a)); g.strokePath (red, juce::PathStrokeType (4.f));
    }
    for (int i = 0; i <= 24; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 24.f, len = (i % 6 == 0) ? 10.f : 5.f;
        const auto p1 = pivot + dir (a) * (R - len), p2 = pivot + dir (a) * R;
        g.setColour (i >= 18 ? juce::Colour (0xffb3261a) : ink);
        g.drawLine (p1.x, p1.y, p2.x, p2.y, (i % 6 == 0) ? 1.7f : 1.f);
        if (i % 6 == 0)
        {
            const auto t = pivot + dir (a) * (R - 22.f);
            g.setFont (font (11.f));
            g.drawText (juce::String (i), (int) t.x - 12, (int) t.y - 7, 24, 14, juce::Justification::centred, false);
        }
    }
    g.setColour (ink);
    g.setFont (font (11.f));
    g.drawText (caption, b.withTrimmedTop (b.getHeight() * 0.52f).withHeight (14.f), juce::Justification::centred, false);
    g.setFont (font (9.5f, false));
    g.drawText ("GR dB", b.reduced (8.f, 6.f), juce::Justification::bottomLeft, false);
    g.drawText ("FOUDRE AUDIO", b.reduced (8.f, 6.f), juce::Justification::bottomRight, false);

    const float v = juce::jlimit (0.f, 25.f, shown), a = a0 + (a1 - a0) * v / 24.f;
    const auto tip = pivot + dir (a) * (R + 2.f), tail = pivot + dir (a) * (R * 0.42f);
    g.setColour (juce::Colours::black.withAlpha (0.22f));                                // ombre portee de l'aiguille
    g.drawLine (tail.x + 3.f, tail.y + 3.f, tip.x + 3.f, tip.y + 3.f, 1.8f);
    g.setColour (juce::Colour (0xff111111));
    g.drawLine (tail.x, tail.y, tip.x, tip.y, 1.7f);
    const auto mid = pivot + dir (a) * (R * 0.72f);
    g.setColour (juce::Colour (0xffb3261a));
    g.drawLine (mid.x, mid.y, tip.x, tip.y, 1.7f);
    // cache-pivot en demi-dome
    const float pr = b.getHeight() * 0.16f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3d), pivot.x, pivot.y - pr, juce::Colour (0xff0c0c0d), pivot.x, pivot.y, false));
    g.fillEllipse (pivot.x - pr * 1.25f, pivot.y - pr, pr * 2.5f, pr * 2.f);
    // verre : reflet oblique
    juce::Path refl;
    refl.addQuadrilateral (b.getX(), b.getY(), b.getX() + b.getWidth() * 0.55f, b.getY(), b.getX() + b.getWidth() * 0.35f, b.getBottom(), b.getX(), b.getBottom());
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.18f), b.getX(), b.getY(), juce::Colours::white.withAlpha (0.f), b.getX() + b.getWidth() * 0.4f, b.getY() + b.getHeight() * 0.6f, false));
    g.fillPath (refl);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.35f), 0.f, b.getY(), juce::Colours::transparentBlack, 0.f, b.getY() + 10.f, false));
    g.fillRect (b);
}

// ================================================================ boite de dialogue integree
OrageDialog::OrageDialog (juce::String t, juce::String m, bool askText, juce::String initialText, juce::String okText,
                          std::function<void (bool, const juce::String&)> callback)
    : titleText (std::move (t)), messageText (std::move (m)), hasText (askText), cb (std::move (callback))
{
    setWantsKeyboardFocus (true);
    okBtn.setButtonText (okText);
    cancelBtn.setButtonText ("ANNULER");
    okBtn.onClick = [this] { finish (true); };
    cancelBtn.onClick = [this] { finish (false); };
    addAndMakeVisible (okBtn);
    addAndMakeVisible (cancelBtn);
    if (hasText)
    {
        editor.setText (initialText, false);
        editor.setFont (font (16.f));
        editor.setSelectAllWhenFocused (true);
        editor.setIndents (8, 6);
        editor.onReturnKey = [this] { finish (true); };
        editor.onEscapeKey = [this] { finish (false); };
        addAndMakeVisible (editor);
    }
}

void OrageDialog::focus() { if (hasText) editor.grabKeyboardFocus(); else grabKeyboardFocus(); }

bool OrageDialog::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { finish (false); return true; }
    if (k == juce::KeyPress::returnKey) { finish (true); return true; }
    return true;
}

void OrageDialog::finish (bool ok)
{
    if (done) return;
    done = true;
    const auto text = editor.getText();
    setVisible (false);
    if (cb) cb (ok, text);
    if (onFinished) onFinished();
}

void OrageDialog::resized()
{
    const int w = 520, h = hasText ? 230 : 200;
    box = juce::Rectangle<int> (w, h).withCentre (getLocalBounds().getCentre());
    if (hasText) editor.setBounds (box.getX() + 28, box.getY() + 96, w - 56, 34);
    cancelBtn.setBounds (box.getRight() - 28 - 140 - 10 - 140, box.getBottom() - 30 - 34, 140, 34);
    okBtn.setBounds (box.getRight() - 28 - 140, box.getBottom() - 30 - 34, 140, 34);
}

void OrageDialog::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.62f));
    drawPlate (g, box.toFloat());
    g.setColour (look::print);
    g.setFont (font (16.f));
    g.drawText (titleText, box.getX() + 32, box.getY() + 24, box.getWidth() - 64, 22, juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (look::T().logo));
    g.fillRect (box.getX() + 32, box.getY() + 50, 60, 3);
    g.setColour (look::print.withAlpha (0.9f));
    g.setFont (font (13.f, false));
    g.drawFittedText (messageText, box.getX() + 32, box.getY() + 62, box.getWidth() - 64, hasText ? 30 : 70, juce::Justification::topLeft, 3);
}
