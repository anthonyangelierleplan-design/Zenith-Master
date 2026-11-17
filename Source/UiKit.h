#pragma once
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// ================================================================ themes Foudre Audio deluxe
namespace look
{
struct Theme
{
    const char* name;
    juce::uint32 panelTop, panelBot, plateTop, plateBot;       // facade et plaques vissees
    juce::uint32 print, accent, pointer, led;                  // serigraphie, accent (arcs, selection), repere des potards, LED
    juce::uint32 skirtHi, skirtLo, capHi, capMid, capLo;       // potards : jupe moletee + chapeau
    juce::uint32 keyHi, keyLo, lcdBg, lcdText;                 // touches, afficheurs
    juce::uint32 woodA, woodB;                                 // flancs en bois
    juce::uint32 vuFace, vuInk, logo;                          // cadran du VU, encre du cadran, filet du logo
};
inline const Theme kThemes[3] {
    // ORIGINAL : graphite d'Orage, LED orange, potards metal sombre, noyer
    { "ORIGINAL", 0xff1a1a1c, 0xff0d0d0e, 0xff1d1d20, 0xff101012,
      0xffe9e5d8, 0xffff7a1a, 0xfff0ece0, 0xffff9a3a,
      0xff3c3c41, 0xff060607, 0xffa4a4ab, 0xff4a4a50, 0xff141416,
      0xff2c2c30, 0xff121214, 0xff070605, 0xffffa63a,
      0xff6b3f22, 0xff351d0e,
      0xffeee3bd, 0xff2a2418, 0xffff7a1a },
    // MINUIT : bleu nuit de Zephyr, aluminium et bleu glacier, palissandre
    { "MINUIT",   0xff1b2536, 0xff0b111c, 0xff26324a, 0xff141c2a,
      0xffdde7f5, 0xff4f9bff, 0xff9fd2ff, 0xff5ab4ff,
      0xff3b4659, 0xff0f141d, 0xfff0f3f8, 0xffaab3c0, 0xff4a5260,
      0xff334059, 0xff1a2232, 0xff070c16, 0xffcfe6ff,
      0xff5c3621, 0xff2e1a0f,
      0xffe3edf8, 0xff13233a, 0xff4f9bff },
    // ZENITH : noir et creme d'origine, filet rouge, bakelite a chapeau alu, teck
    { "ZENITH",   0xff161413, 0xff0a0908, 0xff1f1c1a, 0xff110f0e,
      0xffefe8d6, 0xffe0662a, 0xffefe8d6, 0xffff9a3a,
      0xff2e2a27, 0xff050404, 0xfff7f6f1, 0xffc6c3bb, 0xff5c5a55,
      0xff2e2a27, 0xff12100f, 0xff080605, 0xffffa63a,
      0xffa8693a, 0xff5a3215,
      0xffefe4c2, 0xff2a2418, 0xffc8281c },
};
constexpr int kNumThemes = 3;
inline int themeIdx = 0;
inline const Theme& T() { return kThemes[themeIdx < 0 || themeIdx >= kNumThemes ? 0 : themeIdx]; }
inline juce::Colour paint  { 0xff101012 };       // fond des plaques (suit le theme)
inline juce::Colour print  { 0xffe9e5d8 };       // serigraphie (suit le theme)
inline juce::Colour cvRing { 0xffff7a1a };       // accent : arcs des macros, crans, selection (suit le theme)
inline void applyTheme (int i)
{
    themeIdx = juce::jlimit (0, kNumThemes - 1, i);
    paint = juce::Colour (T().plateBot); print = juce::Colour (T().print); cvRing = juce::Colour (T().accent);
}
}

namespace ui
{
juce::String fr (const char* utf8);
juce::Font font (float h, bool bold = true);
juce::Colour heatColour (float t);
void drawScrew (juce::Graphics& g, juce::Point<float> p, float r, int seed);
void drawPlate (juce::Graphics& g, juce::Rectangle<float> b, bool screws = true);           // plaque vissee, metal brosse
void drawInset (juce::Graphics& g, juce::Rectangle<float> b);                               // zone creusee dans une plaque
void drawVent (juce::Graphics& g, juce::Rectangle<float> b);                                // grille d'aeration (fentes)
void drawKey (juce::Graphics& g, juce::Rectangle<float> r, bool pressed, bool hover = false);
void drawJewel (juce::Graphics& g, juce::Rectangle<float> r, bool on, juce::Colour ledCol);
void drawKnobBody (juce::Graphics& g, juce::Point<float> c, float kr, float angle, float alpha);
void drawLcdBox (juce::Graphics& g, juce::Rectangle<float> b);
void drawTube (juce::Graphics& g, juce::Rectangle<float> r, float heat, bool big, int seed);  // lampe realiste qui chauffe
void drawWood (juce::Graphics& g, juce::Rectangle<int> r, bool left);
}

class OrageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OrageLookAndFeel();
    void refresh();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getPopupMenuFont() override { return ui::font (14.f, false); }
};

// ---------------------------------------------------------------- potard (echelle et nom serigraphies dans paint())
// potard avec "cran" : un aimant leger retient la valeur au zero / au centre (double-clic = valeur par defaut)
class DetentSlider : public juce::Slider
{
public:
    double detent = -1e30;                                   // valeur du cran (hors plage = aucun)
    double snapValue (double v, DragMode mode) override
    {
        if (mode == notDragging || detent < getMinimum() || detent > getMaximum()) return v;
        return std::abs (v - detent) < (getMaximum() - getMinimum()) * 0.022 ? detent : v;
    }
};

class KnobUnit : public juce::Component
{
public:
    KnobUnit (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& text,
              const juce::String& lo, const juce::String& hi, int cvIdx = -1);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setCvActive (bool);
    // valeur reellement appliquee (apres macros) : arc d'accent + aiguille fantome entre le reglage et cette valeur
    void setMacroValue (bool active, double effective);
    // commutateur rotatif : un libelle par position autour du bouton (parametre "choix")
    void setChoices (const juce::StringArray& names);

    DetentSlider slider;
    juce::String text, loText, hiText;
    juce::StringArray choices;
    int cvIndex;
    bool cvActive = false;
    bool macroOn = false;
    double macroVal = 0.0;
private:
    juce::AudioProcessorParameter* attachmentParam = nullptr;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

// ---------------------------------------------------------------- touches lumineuses (parametre "choix")
class SegmentSelector : public juce::Component
{
public:
    SegmentSelector (juce::AudioProcessorValueTreeState&, const juce::String& id, juce::StringArray names);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    int columns = 0;                                         // 0 = une seule rangee
private:
    juce::StringArray labels;
    int selected = 0;
    juce::ParameterAttachment attachment;
};

class LedToggle : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// ---------------------------------------------------------------- vumetre a aiguille, cadran retroeclaire
class VuMeter : public juce::Component
{
public:
    explicit VuMeter (juce::String cap = "COMPRESSION") : caption (std::move (cap)) {}
    void setValue (float grDb) { target = grDb; }
    void tick();
    void paint (juce::Graphics&) override;
private:
    juce::String caption;
    float target = 0.f, shown = 0.f, lastDrawn = -1.f;
};

// ---------------------------------------------------------------- boite de dialogue integree
class OrageDialog : public juce::Component
{
public:
    OrageDialog (juce::String title, juce::String message, bool askText, juce::String initialText, juce::String okText,
                 std::function<void (bool, const juce::String&)> callback);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void focus();

    std::function<void()> onFinished;
private:
    void finish (bool ok);
    juce::String titleText, messageText;
    bool hasText;
    juce::TextEditor editor;
    juce::TextButton okBtn, cancelBtn;
    std::function<void (bool, const juce::String&)> cb;
    juce::Rectangle<int> box;
    bool done = false;
};
