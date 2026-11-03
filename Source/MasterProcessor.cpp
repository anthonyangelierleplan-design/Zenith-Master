#include "MasterProcessor.h"
#include "MasterEditor.h"

using namespace tn;

namespace
{
// texte affiche dans les bulles des potards : vraies unites
juce::String valueText (int i, float v)
{
    switch (i)
    {
        case p_in: case p_out: case p_gL: case p_gM: case p_gH: case p_lsG: case p_mdG: case p_hsG: case p_cmpMk:
            return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB";
        case p_tL: case p_fL: case p_tM: case p_fM: case p_tH: case p_fH: case p_cmpThr: case p_limCeil:
            return juce::String (v, 1) + " dB";
        case p_limPush: case p_cmpKnee: return juce::String (v, 1) + " dB";
        case p_xo1: case p_xo2: case p_eqHpf: case p_lsF: case p_mdF: case p_hsF: case p_cmpSc:
            return v >= 1000.f ? juce::String (v / 1000.f, 2) + " kHz" : juce::String ((int) std::round (v)) + " Hz";
        case p_cmpRat:   return juce::String (v, 1) + " : 1";
        case p_mdQ:      return "Q " + juce::String (v, 2);
        case p_cmpAtk: case p_cmpRel: case p_limRel: return (v < 10.f ? juce::String (v, 1) : juce::String ((int) std::round (v))) + " ms";
        case p_ottAtk:   { const float ms = 0.2f * std::pow (150.f, v); return (ms < 10.f ? juce::String (ms, 1) : juce::String ((int) std::round (ms))) + " ms"; }
        case p_ottRel:   return juce::String ((int) std::round (15.f * std::pow (60.f, v))) + " ms";
        case p_mDyn: case p_mBri: case p_mSq:
        {
            const int pc = (int) std::round ((v - 0.5f) * 200.f);
            return (pc > 0 ? "+" : "") + juce::String (pc) + " %";
        }
        default:         return juce::String ((int) std::round (v * 100.f)) + " %";
    }
}
}

juce::AudioProcessorValueTreeState::ParameterLayout ZenithProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < kNumP; ++i)
    {
        const auto& d = paramDefs()[i];
        const juce::ParameterID pid { d.id, 1 };
        if (d.kind == 1)
            layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
        else
        {
            juce::NormalisableRange<float> r (d.mn, d.mx);
            if (d.centre > d.mn && d.centre < d.mx) r.setSkewForCentre (d.centre);
            juce::AudioParameterFloatAttributes at;
            at = at.withStringFromValueFunction ([i] (float v, int) { return valueText (i, v); })
                   .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
            layout.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, r, d.def, at));
        }
    }
    return layout;
}

ZenithProcessor::ZenithProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ZENITH", makeLayout())
{
    for (int i = 0; i < kNumP; ++i) raw[i] = apvts.getRawParameterValue (paramDefs()[i].id);
}

bool ZenithProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ZenithProcessor::prepareToPlay (double sr, int)
{
    master.prepare ((float) sr);
    setLatencySamples (master.latency());
}

void ZenithProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    if (buf.getNumChannels() < 2) return;
    for (int i = 0; i < kNumP; ++i) values[i] = raw[i]->load();
    master.setParams (values);
    master.process (buf.getWritePointer (0), buf.getWritePointer (1), buf.getNumSamples());
}

juce::AudioProcessorEditor* ZenithProcessor::createEditor() { return new ZenithEditor (*this); }

// ---------------------------------------------------------------- etat et presets
juce::ValueTree ZenithProcessor::captureState()
{
    auto s = apvts.copyState();
    s.setProperty ("presetName", presetName, nullptr);
    return s;
}

void ZenithProcessor::applyState (const juce::ValueTree& s)
{
    presetName = s.getProperty ("presetName", "Init").toString();
    apvts.replaceState (s);
}

void ZenithProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = captureState().createXml()) copyXmlToBinary (*xml, dest);
}

void ZenithProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) applyState (juce::ValueTree::fromXml (*xml));
}

namespace
{
struct Setting { int id; float v; };
struct Factory { const char* name; std::vector<Setting> s; };

const std::vector<Factory>& factory()
{
    static const std::vector<Factory> f {
        { "Init (neutre)", {} },
        { "Gentle Glue", {
            { p_intensity, .35f }, { p_ottDepth, .5f }, { p_ottAtk, .45f }, { p_ottRel, .6f }, { p_cmpThr, -18.f }, { p_cmpRat, 2.f },
            { p_cmpAtk, 30.f }, { p_cmpRel, 250.f }, { p_cmpKnee, 8.f }, { p_limPush, 2.f } } },
        { "Dirty Punch", {
            { p_intensity, .75f }, { p_ottDepth, .85f }, { p_ottAtk, .35f }, { p_ottRel, .4f }, { p_mDyn, .68f }, { p_cmpThr, -16.f },
            { p_cmpRat, 4.f }, { p_cmpAtk, 10.f }, { p_cmpRel, 120.f }, { p_limPush, 5.f }, { p_limClip, .2f }, { p_hsG, 1.f } } },
        { "Justice Squash", {
            { p_intensity, .9f }, { p_ottDepth, 1.f }, { p_ottAtk, .3f }, { p_ottRel, .35f }, { p_mSq, .78f }, { p_mBri, .6f },
            { p_cmpThr, -26.f }, { p_cmpRat, 6.f }, { p_cmpAtk, 8.f }, { p_cmpRel, 90.f }, { p_cmpKnee, 3.f },
            { p_limPush, 8.f }, { p_limClip, .35f }, { p_lsG, 1.5f } } },
        { "Cold Crush", {
            { p_intensity, .8f }, { p_ottDepth, .8f }, { p_ottAtk, .4f }, { p_mBri, .7f }, { p_mDyn, .4f },
            { p_eqHpf, 30.f }, { p_lsG, 1.5f }, { p_mdG, -1.5f }, { p_mdF, 400.f }, { p_hsG, 1.f },
            { p_cmpThr, -20.f }, { p_cmpRat, 4.f }, { p_cmpAtk, 25.f }, { p_cmpRel, 150.f }, { p_cmpMS, 1.f }, { p_limPush, 6.f } } },
        { "Destroyer", {
            { p_intensity, 1.f }, { p_ottDepth, 1.f }, { p_dL, .85f }, { p_dM, .9f }, { p_dH, .85f }, { p_uL, .7f }, { p_uM, .85f }, { p_uH, .9f },
            { p_ottAtk, .2f }, { p_ottRel, .25f }, { p_mSq, .88f }, { p_cmpThr, -32.f }, { p_cmpRat, 12.f }, { p_cmpAtk, 3.f }, { p_cmpRel, 60.f },
            { p_limPush, 14.f }, { p_limClip, .7f } } },
    };
    return f;
}
}

int ZenithProcessor::getNumFactoryPresets() { return (int) factory().size(); }
juce::String ZenithProcessor::getFactoryPresetName (int i) { return factory()[(size_t) juce::jlimit (0, getNumFactoryPresets() - 1, i)].name; }

void ZenithProcessor::loadFactoryPreset (int i)
{
    const auto& f = factory()[(size_t) juce::jlimit (0, getNumFactoryPresets() - 1, i)];
    for (int k = 0; k < kNumP; ++k)                                           // tout revient d'abord au neutre
        if (auto* prm = apvts.getParameter (paramDefs()[k].id)) prm->setValueNotifyingHost (prm->getDefaultValue());
    for (const auto& s : f.s)
        if (auto* prm = apvts.getParameter (paramDefs()[s.id].id)) prm->setValueNotifyingHost (prm->convertTo0to1 (s.v));
    presetName = f.name;
}

juce::File ZenithProcessor::getPresetDir() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Foudre Audio").getChildFile ("Zenith").getChildFile ("Presets");
}

juce::StringArray ZenithProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getPresetDir().findChildFiles (juce::File::findFiles, false, "*.zenith"))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool ZenithProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = name.trim();
    if (clean.isEmpty()) return false;
    const auto dir = getPresetDir();
    if (! dir.createDirectory().wasOk()) return false;
    presetName = clean;
    if (auto xml = captureState().createXml())
        return xml->writeTo (dir.getChildFile (juce::File::createLegalFileName (clean) + ".zenith"));
    return false;
}

bool ZenithProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = getPresetDir().getChildFile (juce::File::createLegalFileName (name) + ".zenith");
    if (auto xml = juce::parseXML (file))
        if (xml->hasTagName (apvts.state.getType()))
        {
            applyState (juce::ValueTree::fromXml (*xml));
            presetName = name;
            return true;
        }
    return false;
}

bool ZenithProcessor::deleteUserPreset (const juce::String& name)
{
    return getPresetDir().getChildFile (juce::File::createLegalFileName (name) + ".zenith").deleteFile();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ZenithProcessor(); }
