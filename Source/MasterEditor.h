#pragma once
#include <map>
#include "MasterProcessor.h"
#include "UiKit.h"

// plaque vissee (titre grave + zones creusees + serigraphies libres)
class Plate : public juce::Component
{
public:
    explicit Plate (juce::String t) : title (std::move (t)) {}
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics& g) override;
    juce::String title;
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> insets;      // zones creusees (titre, rectangle)
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> labels;      // serigraphies libres
};

// barre de LED : niveau (vert->rouge), reduction (accent) ou gain ajoute ; verticale ou horizontale
class LedBar : public juce::Component
{
public:
    enum Mode { Level, Reduction, Boost };
    LedBar (Mode m, float lo, float hi, juce::String cap = {}, bool horiz = false) : mode (m), lo_ (lo), hi_ (hi), caption (std::move (cap)), horizontal (horiz) {}
    void set (float v) { if (std::abs (v - value) > 0.05f) { value = v; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    Mode mode; float lo_, hi_, value = 0.f; juce::String caption; bool horizontal;
};

// afficheur (theme) : titre + valeur, ou texte libre sur plusieurs lignes
class Lcd : public juce::Component
{
public:
    Lcd (juce::String l, bool big = false, bool multi = false) : label (std::move (l)), large (big), multiline (multi) {}
    void set (const juce::String& t) { if (t != text) { text = t; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    juce::String label, text { "--" }; bool large, multiline;
};

// tuile de la chaine : un module, cliquable, ouvre son tiroir
class ModuleTile : public juce::Component
{
public:
    ModuleTile (int n, juce::String nm) : number (n), name (std::move (nm)) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void setState (bool on, const juce::String& sub, const juce::String& value, bool sel);
    std::function<void()> onClick;
    juce::String name;
private:
    int number;
    juce::String subText, valueText;
    bool active = true, selected = false, hover = false;
};

// courbe de reponse de l'EQ (valeurs reellement appliquees, macros comprises)
class EqCurve : public juce::Component
{
public:
    void setValues (const float* e, bool on);
    void paint (juce::Graphics&) override;
private:
    float hpf = 20.f, lsF = 100.f, lsG = 0.f, mdF = 1500.f, mdG = 0.f, mdQ = 0.9f, hsF = 8000.f, hsG = 0.f;
    bool enabled = true;
};

// facade du compresseur : change avec le modele (inset sobre, martele gris, noir anodise, bleu acier)
class ModelFace : public juce::Component
{
public:
    void setModel (int m) { if (m != model) { model = m; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    int model = 0;
};

class ZenithContent : public juce::Component, private juce::Timer
{
public:
    explicit ZenithContent (ZenithProcessor&);
    ~ZenithContent() override;
    void paint (juce::Graphics&) override;
    static constexpr int designW = 1400, designH = 860;
private:
    void timerCallback() override;
    void buildPanelTex();
    KnobUnit* knob (juce::Component& parent, tn::P id, const juce::String& text, const char* lo = "", const char* hi = "", double detent = -1e30);
    KnobUnit* rotary (juce::Component& parent, tn::P id, const juce::String& text);
    LedToggle* toggle (juce::Component& parent, tn::P id, const juce::String& text);
    SegmentSelector* selector (juce::Component& parent, tn::P id);
    juce::Label* caption (juce::Component& parent, const juce::String& text, juce::Rectangle<int> r, float size = 11.f);
    void showModel (int m);
    void showTab (int t);
    void cycleTheme();
    void stepPreset (int dir);
    void presetMenu();
    void afterPreset();
    void saveDialog();
    void showDialog (const juce::String&, const juce::String&, bool, const juce::String&, const juce::String&, std::function<void (bool, const juce::String&)>);
    void confirm (const juce::String&, const juce::String&, std::function<void()>);

    ZenithProcessor& proc;
    OrageLookAndFeel laf;
    juce::TooltipWindow tips { this, 350 };          // bulle "valeur reglee -> valeur reelle" des potards deplaces par les macros
    juce::Image panelTex;
    juce::Random rnd;
    float heatShown = 0.f, flicker = 0.f;
    int tab = 3;

    Plate actPlate { "COMPRESSION" }, macroPlate { "INTENSITY  &  MACROS" }, outPlate { "OUTPUT  &  LOUDNESS" };
    Plate drawer[5] { Plate ("1  -  INPUT"), Plate ("2  -  OTT  :  3-BAND UP / DOWN COMPRESSION"), Plate ("3  -  EQ"),
                      Plate ("4  -  COMPRESSOR"), Plate ("5  -  LIMITER") };
    ModuleTile tiles[5] { ModuleTile (1, "INPUT"), ModuleTile (2, "OTT"), ModuleTile (3, "EQ"), ModuleTile (4, "COMP"), ModuleTile (5, "LIMITER") };

    std::vector<std::unique_ptr<KnobUnit>> knobs;
    KnobUnit* byId[tn::kNumP] {};
    std::vector<std::pair<KnobUnit*, int>> extra;                                    // potards en double (meme parametre dans plusieurs vues)
    std::vector<std::unique_ptr<LedToggle>> toggles;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> att;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<juce::Component*> modelGroup[4];
    int shownModel = -1;

    LedBar *inBar = nullptr, *outBar = nullptr, *limBar = nullptr, *cmpBar = nullptr, *dlBar = nullptr, *bandDown[3] {}, *bandUp[3] {}, *actBar[4] {};
    VuMeter vu;
    EqCurve eqCurve;
    ModelFace modelFace;
    Lcd *lcdInS = nullptr, *lcdOutM = nullptr, *lcdOutS = nullptr, *lcdOutI = nullptr, *lcdMatch = nullptr, *lcdBand[3] {}, *lcdAct[4] {},
        *lcdDelta = nullptr, *lcdModel = nullptr;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, nameBtn, saveBtn { "SAVE" }, resetLufs { "RESET LUFS" }, themeBtn;
    std::unique_ptr<OrageDialog> dialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZenithContent)
};

class ZenithEditor : public juce::AudioProcessorEditor
{
public:
    explicit ZenithEditor (ZenithProcessor&);
    ~ZenithEditor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff0b0b0c)); }
    void resized() override;
    ZenithContent content;
private:
    juce::ComponentBoundsConstrainer constrainer;
};
