#pragma once
#include <map>
#include "MasterProcessor.h"
#include "UiKit.h"

// plaque serigraphiee (titre + filet)
class Plate : public juce::Component
{
public:
    explicit Plate (juce::String t) : title (std::move (t)) {}
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics& g) override;
    juce::String title;
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> labels;      // serigraphies libres
};

// barre de LED verticale : niveau (vert->rouge), reduction (orange, depuis le haut) ou gain ajoute (vert, depuis le bas)
class LedBar : public juce::Component
{
public:
    enum Mode { Level, Reduction, Boost };
    LedBar (Mode m, float lo, float hi, juce::String cap = {}) : mode (m), lo_ (lo), hi_ (hi), caption (std::move (cap)) {}
    void set (float v) { if (std::abs (v - value) > 0.05f) { value = v; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    Mode mode; float lo_, hi_, value = 0.f; juce::String caption;
};

// afficheur LCD ambre
class Lcd : public juce::Component
{
public:
    Lcd (juce::String l, bool big = false) : label (std::move (l)), large (big) {}
    void set (const juce::String& t) { if (t != text) { text = t; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    juce::String label, text { "--" }; bool large;
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
    LedToggle* toggle (juce::Component& parent, tn::P id, const juce::String& text);
    SegmentSelector* selector (juce::Component& parent, tn::P id);
    juce::Label* caption (juce::Component& parent, const juce::String& text, juce::Rectangle<int> r);
    void showModel (int m);
    void drawTube (juce::Graphics&, juce::Rectangle<float>, float glow, float arc);
    void drawBolt (juce::Graphics&, juce::Point<float>, juce::Point<float>);
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
    float heatShown = 0.f, arcShown = 0.f;

    Plate inPlate { "INPUT" }, ottPlate { "OTT  -  3-BAND UP/DOWN COMPRESSION" }, macroPlate { "INTENSITY  &  MACROS" },
          eqPlate { "EQ" }, cmpPlate { "COMPRESSOR" }, limPlate { "LIMITER" }, outPlate { "OUTPUT  &  LOUDNESS" };

    std::vector<std::unique_ptr<KnobUnit>> knobs;
    KnobUnit* byId[tn::kNumP] {};
    std::vector<std::unique_ptr<LedToggle>> toggles;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> att;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<juce::Component*> modelGroup[4];                     // reglages propres a chaque modele de compresseur
    int shownModel = -1;

    LedBar *inBar = nullptr, *outBar = nullptr, *limBar = nullptr, *bandDown[3] {}, *bandUp[3] {};
    VuMeter vu;
    Lcd *lcdInS = nullptr, *lcdOutM = nullptr, *lcdOutS = nullptr, *lcdOutI = nullptr, *lcdMatch = nullptr, *lcdBand[3] {};
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, nameBtn, saveBtn { "SAVE" }, resetLufs { "RESET" };
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
