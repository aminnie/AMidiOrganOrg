/*
  ==============================================================================

    AMidiUtils.h
    Created: 7 Jan 2024 9:00:37pm
    Author:  a_min

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <functional>
#include <map>
#include <optional>

//==============================================================================
static void BubbleMessage(Component& targetComponent, const String& textToShow,
    std::unique_ptr<BubbleMessageComponent>& bmc);

static int check0to127(int val);
static int check1to16(int val);
static int lookupPanelGroup(int buttonidx);

static FileLogger* flogger;

// Fixed number of Buttons, Button Groups and Presets
static const int numbervoicebuttons = 96;
static const int numberbuttongroups = 12;
static const int numberpresets = 13;       // Manual (0) + Preset 1..12
static const int numberdisplaypresets = 6; // UI shows six numbered presets per bank

static const int defvelocityout = 64;

static const int numbermodules = 6;
static constexpr int kProjectMiddleCOctave = 4; // Project-wide convention: MIDI 60 == C4

inline juce::String getProjectMidiNoteName(int noteNumber)
{
    return juce::MidiMessage::getMidiNoteName(noteNumber, true, true, kProjectMiddleCOctave);
}

inline juce::String getProjectMidiMessageDescription(const juce::MidiMessage& message)
{
    if (message.isNoteOn())           return "Note on " + getProjectMidiNoteName(message.getNoteNumber());
    if (message.isNoteOff())          return "Note off " + getProjectMidiNoteName(message.getNoteNumber());
    if (message.isProgramChange())    return "Program change " + juce::String(message.getProgramChangeNumber());
    if (message.isPitchWheel())       return "Pitch wheel " + juce::String(message.getPitchWheelValue());
    if (message.isAftertouch())       return "After touch " + getProjectMidiNoteName(message.getNoteNumber()) + ": " + juce::String(message.getAfterTouchValue());
    if (message.isChannelPressure())  return "Channel pressure " + juce::String(message.getChannelPressureValue());
    if (message.isAllNotesOff())      return "All notes off";
    if (message.isAllSoundOff())      return "All sound off";
    if (message.isMetaEvent())        return "Meta event";
    if (message.isSysEx())
        return "SysEx " + juce::String::toHexString(message.getRawData(), message.getRawDataSize());

    if (message.isController())
    {
        juce::String name(juce::MidiMessage::getControllerName(message.getControllerNumber()));

        if (name.isEmpty())
            name = "[" + juce::String(message.getControllerNumber()) + "]";

        return "Controller " + name + ": " + juce::String(message.getControllerValue());
    }

    return juce::String::toHexString(message.getRawData(), message.getRawDataSize());
}

/** Shared keyboard/preset label colour: configured entries use black text; unconfigured use readable mid-grey. */
inline juce::Colour getConfiguredLabelTextColour(bool configured)
{
    return configured ? juce::Colours::black : juce::Colour(0xff888888);
}

/** Display bank for real preset id (1..6 => bank 0, 7..12 => bank 1). */
inline int getPresetDisplayBankForRealPresetIdx(int presetIdx)
{
    return presetIdx >= 7 ? 1 : 0;
}

/** First real preset index rendered in each display bank. */
inline int getBankStartPresetIdxForDisplayBank(int bank)
{
    return bank == 1 ? 7 : 1;
}

/** Slot index (0..5) for a real preset id in the selected display bank, or -1 if not visible. */
inline int getDisplaySlotForRealPresetIdx(int presetIdx, int displayBank)
{
    if (presetIdx <= 0 || presetIdx >= numberpresets)
        return -1;

    const int bankStart = getBankStartPresetIdxForDisplayBank(displayBank);
    const int slot = presetIdx - bankStart;
    return (slot >= 0 && slot < numberdisplaypresets) ? slot : -1;
}

// Tracking switching between tabs to trigger updates, e.g. from Sounds tab back to Upper
static int lasttabidx = 1;
static bool tabchanged = false;
static bool bvoiceupdated = false;
static bool beffectsupdated = false;
static bool bpendingSoundEdit = false;
static bool bpendingEffectsEdit = false;
static bool bpendingPresetSet = false;

static bool hasPendingExitSavePrompt()
{
    return bpendingSoundEdit || bpendingEffectsEdit || bpendingPresetSet;
}

static void clearPendingSoundEditPrompt()
{
    bpendingSoundEdit = false;
}

static void clearPendingEffectsEditPrompt()
{
    bpendingEffectsEdit = false;
}

static void clearPendingExitSavePrompt()
{
    bpendingSoundEdit = false;
    bpendingEffectsEdit = false;
    bpendingPresetSet = false;
}

// Single config file supported with master panel file for each Midi mpdule supported
static const String organdir = "AMidiOrgan";

static const String defpanelfname = "masterinstrument.pnl";
static const String defmodulefname = "mastermodules.mod";

// Centralized runtime state for filenames/module selection.
struct AppState final
{
    // Use literals here to avoid startup-order coupling with other global Strings.
    String panelfname = "masterinstrument.pnl";
    String panelfullpathname = "";

    String configfname = "amidiconfigs.cfg";
    String pnlconfigfname = "amidiconfigs.cfg";
    String configdir = "configs";
    int defaultEffectsVol = 100;
    /** Default Brilliance (CC) for newly assigned voices (Sounds tab route + fresh panel init). */
    int defaultEffectsBri = 30;
    /** Remaining default MIDI effect CC values for new voice assignments (0–127; match `Instrument` defaults). */
    int defaultEffectsExp = 127;
    int defaultEffectsRev = 20;
    int defaultEffectsCho = 10;
    int defaultEffectsMod = 0;
    int defaultEffectsTim = 0;
    int defaultEffectsAtk = 0;
    int defaultEffectsRel = 0;
    int defaultEffectsPan = 64;
    /** Config-global MIDI Program Change trigger for Preset Next (input channel 1..16). */
    int presetMidiPcInputChannel = 16;
    /** Config-global MIDI Program Change number (0..127) that triggers Preset Next. */
    int presetMidiPcValue = 0;
    /** Active UI profile id loaded from configs/ui_profiles.json (e.g. 1480x320, 2560x720). */
    String uiProfileId = "1480x320";
    bool startupMonitorEnabled = false;
    bool configchanged = false;
    bool configreload = false;
    /** User chose "Load anyway" when cfg vs panel embedded name differed; gates panel Save until confirmed or realigned. */
    bool configPanelPairingMismatchAcknowledged = false;

    String modulefname = "amidimodules.mod";
    String defmodulefname = "mastermodules.mod";

    String userdata = "";

    // Instrument module defaults. Changed when new module selected.
    int moduleidx = 1;
    /** Instrument JSON catalogs live under Documents/AMidiOrgan/instruments/ */
    String instrumentdir = "instruments";
    /** Panel .pnl files live under Documents/AMidiOrgan/panels/ */
    String paneldir = "panels";
    String instrumentfname = "maxplus.json";
    String vendorname = "Deebach Blackbox";
    String instrumentdname = "BlackBox";

    bool isrotary = true;
    bool iszerobased = true;

    /** Manual Leslie Fast/Slow + Brake UI on Upper keyboard tab (saved on panel root). */
    bool upperManualRotaryFast = true;
    bool upperManualRotaryBrake = false;
    /** Upper manual rotary target group selector (1=Group 1, 2=Group 2), saved on panel root. */
    int upperManualRotaryTargetGroup = 1;
    /** Manual Leslie Fast/Slow + Brake UI on Lower keyboard tab (saved on panel root). */
    bool lowerManualRotaryFast = true;
    bool lowerManualRotaryBrake = false;
    /** Lower manual rotary target group selector (1=Group 1, 2=Group 2), saved on panel root. */
    int lowerManualRotaryTargetGroup = 1;
    /** Runtime-only last voice button selected on keyboard tabs; used to hydrate Sounds/Effects context. */
    int lastSelectedPanelButtonIdx = -1;
    /** True only after an explicit user click on a keyboard voice button. */
    bool hasExplicitVoiceSelection = false;

    // Static voice from the 1st voice in selected Midi module. Used to create new panel file.
    String sdefVoice;
    int sdefMSB = 0;
    int sdefLSB = 0;
    int sdefFont = 0;
};

inline AppState& getAppState()
{
    static AppState state;
    return state;
}

// Backward-compatible aliases while code is gradually migrated to getAppState().
inline String& panelfname = getAppState().panelfname;
inline String& panelfullpathname = getAppState().panelfullpathname;
inline String& configfname = getAppState().configfname;
inline String& pnlconfigfname = getAppState().pnlconfigfname;
inline String& configdir = getAppState().configdir;
inline int& defaultEffectsVol = getAppState().defaultEffectsVol;
inline int& defaultEffectsBri = getAppState().defaultEffectsBri;
inline int& presetMidiPcInputChannel = getAppState().presetMidiPcInputChannel;
inline int& presetMidiPcValue = getAppState().presetMidiPcValue;
inline String& uiProfileId = getAppState().uiProfileId;
inline bool& startupMonitorEnabled = getAppState().startupMonitorEnabled;
inline bool& configchanged = getAppState().configchanged;
inline bool& configreload = getAppState().configreload;
inline bool& configPanelPairingMismatchAcknowledged = getAppState().configPanelPairingMismatchAcknowledged;
inline String& modulefname = getAppState().modulefname;
inline String& userdata = getAppState().userdata;
inline int& moduleidx = getAppState().moduleidx;
inline String& instrumentdir = getAppState().instrumentdir;
inline String& paneldir = getAppState().paneldir;
inline String& instrumentfname = getAppState().instrumentfname;
inline String& vendorname = getAppState().vendorname;
inline String& instrumentdname = getAppState().instrumentdname;
inline bool& isrotary = getAppState().isrotary;
inline bool& iszerobased = getAppState().iszerobased;
inline String& sdefVoice = getAppState().sdefVoice;
inline int& sdefMSB = getAppState().sdefMSB;
inline int& sdefLSB = getAppState().sdefLSB;
inline int& sdefFont = getAppState().sdefFont;

/** `Documents/AMidiOrgan` (folder may not exist until first run). */
inline juce::File getOrganUserDocumentsRoot()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(organdir);
}

/**
 * Canonical folder for `.pnl` files (`Documents/AMidiOrgan/panels` by default).
 * Ensures the directory exists so native FileChooser dialogs reliably open there.
 */
inline juce::File getDefaultPanelsDirectory()
{
    juce::File dir = getOrganUserDocumentsRoot().getChildFile(getAppState().paneldir);
    if (!dir.isDirectory())
        dir.createDirectory();
    return dir;
}

/** Last Start-tab MIDI In/Out selections (`configs/midi_sticky_devices.json`, JUCE device identifiers). */
inline juce::File getMidiStickyDevicesFile()
{
    juce::File dir = getOrganUserDocumentsRoot().getChildFile(getAppState().configdir);
    if (!dir.isDirectory())
        dir.createDirectory();
    return dir.getChildFile("midi_sticky_devices.json");
}

/** Reads sticky port ids; missing file is OK (leaves arrays empty). */
inline void loadMidiStickyDeviceIdentifiersFromFile(juce::StringArray& inputIds, juce::StringArray& outputIds)
{
    inputIds.clear();
    outputIds.clear();

    const juce::File f = getMidiStickyDevicesFile();
    if (!f.existsAsFile())
        return;

    juce::FileInputStream in(f);
    if (!in.openedOk())
        return;

    const juce::var parsed = juce::JSON::parse(in.readEntireStreamAsString());
    if (parsed.isVoid())
        return;

    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr)
        return;

    auto appendStringArray = [](const juce::var& v, juce::StringArray& out)
        {
            if (!v.isArray())
                return;
            const auto* arr = v.getArray();
            if (arr == nullptr)
                return;
            for (const auto& item : *arr)
            {
                const juce::String s = item.toString().trim();
                if (s.isNotEmpty())
                    out.addIfNotAlreadyThere(s);
            }
        };

    appendStringArray(obj->getProperty("midiInputIdentifiers"), inputIds);
    appendStringArray(obj->getProperty("midiOutputIdentifiers"), outputIds);
}

/** Overwrites sticky file; returns false if the directory could not be created or write failed. */
inline bool saveMidiStickyDeviceIdentifiersToFile(const juce::StringArray& inputIds, const juce::StringArray& outputIds)
{
    const juce::File f = getMidiStickyDevicesFile();
    if (!f.getParentDirectory().createDirectory())
        return false;

    juce::Array<juce::var> inArr;
    juce::Array<juce::var> outArr;
    for (const auto& s : inputIds)
        if (s.isNotEmpty())
            inArr.add(juce::var(s));
    for (const auto& s : outputIds)
        if (s.isNotEmpty())
            outArr.add(juce::var(s));

    juce::DynamicObject::Ptr o = new juce::DynamicObject();
    o->setProperty("midiInputIdentifiers", juce::var(inArr));
    o->setProperty("midiOutputIdentifiers", juce::var(outArr));

    const juce::String txt = juce::JSON::toString(juce::var(o.get()), true);
    return f.replaceWithText(txt);
}

/** Last used startup session state (`configs/last_session.json`). */
inline juce::File getLastSessionStateFile()
{
    juce::File dir = getOrganUserDocumentsRoot().getChildFile(getAppState().configdir);
    if (!dir.isDirectory())
        dir.createDirectory();
    return dir.getChildFile("last_session.json");
}

struct UiProfileDefinition final
{
    juce::String id = "1480x320";
    juce::String displayName = "1480 x 320";
    int baseWidth = 1480;
    int baseHeight = 320;
    float xScale = 1.0f;
    float yScale = 1.0f;
    float buttonFontScale = 1.0f;
    float labelFontScale = 1.0f;
    float toggleFontScale = 1.0f;
    float comboFontScale = 1.0f;
    float groupTitleFontScale = 1.0f;
    /** Optional absolute rect overrides for keyboard tab controls (keyed by component id). */
    std::map<juce::String, juce::Rectangle<int>> keyboardRectOverrides;
    /** Optional absolute rect overrides for Start tab controls (keyed by component id). */
    std::map<juce::String, juce::Rectangle<int>> startRectOverrides;
    /** Optional absolute rect overrides for Config tab controls (keyed by component id). */
    std::map<juce::String, juce::Rectangle<int>> configRectOverrides;
    /** Optional per-control font scale overrides (keyed by component id). */
    std::map<juce::String, float> fontScaleOverrides;
};

inline juce::String getDefaultUiProfileId()
{
    return "1480x320";
}

inline juce::File getUiProfilesCatalogFile()
{
    juce::File dir = getOrganUserDocumentsRoot().getChildFile(getAppState().configdir);
    if (!dir.isDirectory())
        dir.createDirectory();
    return dir.getChildFile("ui_profiles.json");
}

inline juce::String buildDefaultUiProfilesCatalogJson()
{
    // Baseline profile and a larger widescreen profile for 2560x720.
    return R"json(
{
  "profiles": [
    {
      "id": "1480x320",
      "displayName": "1480 x 320 (Default)",
      "baseWidth": 1480,
      "baseHeight": 320,
      "xScale": 1.0,
      "yScale": 1.0,
      "buttonFontScale": 1.0,
      "labelFontScale": 1.0,
      "toggleFontScale": 1.0,
      "comboFontScale": 1.0,
      "groupTitleFontScale": 1.0,
      "keyboardRectOverrides": {},
      "startRectOverrides": {},
      "configRectOverrides": {},
      "fontScaleOverrides": {}
    },
    {
      "id": "2560x720",
      "displayName": "2560 x 720",
      "baseWidth": 2560,
      "baseHeight": 720,
      "xScale": 1.7297,
      "yScale": 2.25,
      "buttonFontScale": 1.35,
      "labelFontScale": 1.25,
      "toggleFontScale": 1.25,
      "comboFontScale": 1.2,
      "groupTitleFontScale": 1.2,
      "keyboardRectOverrides": {
        "kbd.upper.voiceEditsGroup": { "x": 330, "y": 430, "w": 295, "h": 180 },
        "kbd.lower.voiceEditsGroup": { "x": 330, "y": 430, "w": 295, "h": 180 },
        "kbd.bass.voiceEditsGroup": { "x": 330, "y": 430, "w": 295, "h": 180 },
        "kbd.upper.presetGroup": { "x": 640, "y": 430, "w": 1200, "h": 180 },
        "kbd.lower.presetGroup": { "x": 640, "y": 430, "w": 1200, "h": 180 },
        "kbd.bass.presetGroup": { "x": 640, "y": 430, "w": 1200, "h": 180 },
        "kbd.upper.rotaryGroup": { "x": 20, "y": 430, "w": 295, "h": 180 },
        "kbd.lower.rotaryGroup": { "x": 20, "y": 430, "w": 295, "h": 180 },
        "kbd.upper.navLower": { "x": 1860, "y": 473, "w": 138, "h": 113 },
        "kbd.upper.navBass": { "x": 2020, "y": 473, "w": 138, "h": 113 },
        "kbd.lower.navUpper": { "x": 1860, "y": 473, "w": 138, "h": 113 },
        "kbd.lower.navBass": { "x": 2020, "y": 473, "w": 138, "h": 113 },
        "kbd.bass.navUpper": { "x": 1860, "y": 473, "w": 138, "h": 113 },
        "kbd.bass.navLower": { "x": 2020, "y": 473, "w": 138, "h": 113 },
        "kbd.upper.status": { "x": 2175, "y": 420, "w": 346, "h": 46 },
        "kbd.lower.status": { "x": 2175, "y": 420, "w": 346, "h": 46 },
        "kbd.bass.status": { "x": 2175, "y": 420, "w": 346, "h": 46 },
        "kbd.upper.panelFile": { "x": 2175, "y": 473, "w": 346, "h": 68 },
        "kbd.lower.panelFile": { "x": 2175, "y": 473, "w": 346, "h": 68 },
        "kbd.bass.panelFile": { "x": 2175, "y": 473, "w": 346, "h": 68 },
        "kbd.upper.save": { "x": 2175, "y": 518, "w": 138, "h": 68 },
        "kbd.lower.save": { "x": 2175, "y": 518, "w": 138, "h": 68 },
        "kbd.bass.save": { "x": 2175, "y": 518, "w": 138, "h": 68 },
        "kbd.upper.saveAs": { "x": 2348, "y": 518, "w": 138, "h": 68 },
        "kbd.lower.saveAs": { "x": 2348, "y": 518, "w": 138, "h": 68 },
        "kbd.bass.saveAs": { "x": 2348, "y": 518, "w": 138, "h": 68 }
      },
      "startRectOverrides": {},
      "configRectOverrides": {},
      "fontScaleOverrides": {}
    }
  ]
}
)json";
}

inline std::map<juce::String, juce::Rectangle<int>> getBuiltIn2560KeyboardRectOverrides()
{
    return {
        { "kbd.upper.voiceEditsGroup", { 330, 430, 295, 180 } },
        { "kbd.lower.voiceEditsGroup", { 330, 430, 295, 180 } },
        { "kbd.bass.voiceEditsGroup", { 330, 430, 295, 180 } },
        { "kbd.upper.presetGroup", { 640, 430, 1200, 180 } },
        { "kbd.lower.presetGroup", { 640, 430, 1200, 180 } },
        { "kbd.bass.presetGroup", { 640, 430, 1200, 180 } },
        { "kbd.upper.rotaryGroup", { 20, 430, 295, 180 } },
        { "kbd.lower.rotaryGroup", { 20, 430, 295, 180 } },
        { "kbd.upper.navLower", { 1860, 473, 138, 113 } },
        { "kbd.upper.navBass", { 2020, 473, 138, 113 } },
        { "kbd.lower.navUpper", { 1860, 473, 138, 113 } },
        { "kbd.lower.navBass", { 2020, 473, 138, 113 } },
        { "kbd.bass.navUpper", { 1860, 473, 138, 113 } },
        { "kbd.bass.navLower", { 2020, 473, 138, 113 } },
        { "kbd.upper.status", { 2175, 420, 346, 46 } },
        { "kbd.lower.status", { 2175, 420, 346, 46 } },
        { "kbd.bass.status", { 2175, 420, 346, 46 } },
        { "kbd.upper.panelFile", { 2175, 473, 346, 68 } },
        { "kbd.lower.panelFile", { 2175, 473, 346, 68 } },
        { "kbd.bass.panelFile", { 2175, 473, 346, 68 } },
        { "kbd.upper.save", { 2175, 518, 138, 68 } },
        { "kbd.lower.save", { 2175, 518, 138, 68 } },
        { "kbd.bass.save", { 2175, 518, 138, 68 } },
        { "kbd.upper.saveAs", { 2348, 518, 138, 68 } },
        { "kbd.lower.saveAs", { 2348, 518, 138, 68 } },
        { "kbd.bass.saveAs", { 2348, 518, 138, 68 } }
    };
}

inline bool ensureUiProfilesCatalogFileExists()
{
    const juce::File f = getUiProfilesCatalogFile();
    if (f.existsAsFile())
        return true;
    return f.replaceWithText(buildDefaultUiProfilesCatalogJson());
}

inline juce::Array<UiProfileDefinition> loadUiProfilesCatalog()
{
    juce::Array<UiProfileDefinition> result;
    if (!ensureUiProfilesCatalogFileExists())
        return result;

    const juce::File f = getUiProfilesCatalogFile();
    juce::FileInputStream in(f);
    if (!in.openedOk())
        return result;

    const juce::var parsed = juce::JSON::parse(in.readEntireStreamAsString());
    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return result;

    const juce::var profilesVar = root->getProperty("profiles");
    if (!profilesVar.isArray())
        return result;
    const auto* profiles = profilesVar.getArray();
    if (profiles == nullptr)
        return result;

    auto readString = [](juce::DynamicObject* o, const char* key, const juce::String& fallback)
        {
            const juce::String s = o->getProperty(key).toString().trim();
            return s.isNotEmpty() ? s : fallback;
        };
    auto readInt = [](juce::DynamicObject* o, const char* key, int fallback)
        {
            const juce::var v = o->getProperty(key);
            if (v.isInt() || v.isInt64() || v.isDouble())
                return static_cast<int>(v);
            return fallback;
        };
    auto readFloat = [](juce::DynamicObject* o, const char* key, float fallback)
        {
            const juce::var v = o->getProperty(key);
            if (v.isInt() || v.isInt64() || v.isDouble())
                return static_cast<float>(static_cast<double>(v));
            return fallback;
        };
    auto readRect = [](const juce::var& v, juce::Rectangle<int>& outRect)
        {
            auto* rectObj = v.getDynamicObject();
            if (rectObj == nullptr)
                return false;
            const juce::var xv = rectObj->getProperty("x");
            const juce::var yv = rectObj->getProperty("y");
            const juce::var wv = rectObj->getProperty("w");
            const juce::var hv = rectObj->getProperty("h");
            if (!(xv.isInt() || xv.isInt64() || xv.isDouble())
                || !(yv.isInt() || yv.isInt64() || yv.isDouble())
                || !(wv.isInt() || wv.isInt64() || wv.isDouble())
                || !(hv.isInt() || hv.isInt64() || hv.isDouble()))
                return false;
            const int x = static_cast<int>(xv);
            const int y = static_cast<int>(yv);
            const int w = juce::jmax(1, static_cast<int>(wv));
            const int h = juce::jmax(1, static_cast<int>(hv));
            outRect = { x, y, w, h };
            return true;
        };
    auto readRectMap = [&readRect](const juce::var& v, std::map<juce::String, juce::Rectangle<int>>& out)
        {
            auto* obj = v.getDynamicObject();
            if (obj == nullptr)
                return;
            const auto& props = obj->getProperties();
            for (int i = 0; i < props.size(); ++i)
            {
                const juce::Identifier keyId = props.getName(i);
                juce::Rectangle<int> r;
                if (readRect(obj->getProperty(keyId), r))
                    out[keyId.toString()] = r;
            }
        };
    auto readFloatMap = [](const juce::var& v, std::map<juce::String, float>& out)
        {
            auto* obj = v.getDynamicObject();
            if (obj == nullptr)
                return;
            const auto& props = obj->getProperties();
            for (int i = 0; i < props.size(); ++i)
            {
                const juce::Identifier keyId = props.getName(i);
                const auto vv = obj->getProperty(keyId);
                if (vv.isInt() || vv.isInt64() || vv.isDouble())
                    out[keyId.toString()] = juce::jlimit(0.5f, 4.0f, static_cast<float>((double) vv));
            }
        };

    for (const auto& item : *profiles)
    {
        auto* obj = item.getDynamicObject();
        if (obj == nullptr)
            continue;

        UiProfileDefinition p;
        p.id = readString(obj, "id", getDefaultUiProfileId());
        p.displayName = readString(obj, "displayName", p.id);
        p.baseWidth = juce::jmax(1, readInt(obj, "baseWidth", 1480));
        p.baseHeight = juce::jmax(1, readInt(obj, "baseHeight", 320));
        p.xScale = juce::jlimit(0.25f, 8.0f, readFloat(obj, "xScale", 1.0f));
        p.yScale = juce::jlimit(0.25f, 8.0f, readFloat(obj, "yScale", 1.0f));
        p.buttonFontScale = juce::jlimit(0.50f, 4.0f, readFloat(obj, "buttonFontScale", 1.0f));
        p.labelFontScale = juce::jlimit(0.50f, 4.0f, readFloat(obj, "labelFontScale", 1.0f));
        p.toggleFontScale = juce::jlimit(0.50f, 4.0f, readFloat(obj, "toggleFontScale", 1.0f));
        p.comboFontScale = juce::jlimit(0.50f, 4.0f, readFloat(obj, "comboFontScale", 1.0f));
        p.groupTitleFontScale = juce::jlimit(0.50f, 4.0f, readFloat(obj, "groupTitleFontScale", 1.0f));

        readRectMap(obj->getProperty("keyboardRectOverrides"), p.keyboardRectOverrides);
        readRectMap(obj->getProperty("startRectOverrides"), p.startRectOverrides);
        readRectMap(obj->getProperty("configRectOverrides"), p.configRectOverrides);
        readFloatMap(obj->getProperty("fontScaleOverrides"), p.fontScaleOverrides);
        if (p.id == "2560x720" && p.keyboardRectOverrides.empty())
            p.keyboardRectOverrides = getBuiltIn2560KeyboardRectOverrides();
        result.add(std::move(p));
    }

    return result;
}

inline UiProfileDefinition resolveUiProfile(const juce::String& requestedId)
{
    const auto profiles = loadUiProfilesCatalog();
    if (profiles.isEmpty())
        return {};

    for (const auto& p : profiles)
        if (p.id == requestedId)
            return p;

    for (const auto& p : profiles)
        if (p.id == getDefaultUiProfileId())
            return p;

    return profiles.getReference(0);
}

inline juce::StringArray getUiProfileIds()
{
    juce::StringArray ids;
    for (const auto& p : loadUiProfilesCatalog())
        ids.addIfNotAlreadyThere(p.id);
    if (ids.isEmpty())
        ids.add(getDefaultUiProfileId());
    return ids;
}

inline std::optional<juce::Rectangle<int>> getKeyboardRectOverride(const UiProfileDefinition& profile, const juce::String& componentId)
{
    const auto it = profile.keyboardRectOverrides.find(componentId);
    if (it == profile.keyboardRectOverrides.end())
        return std::nullopt;
    return it->second;
}

inline std::optional<juce::Rectangle<int>> getStartRectOverride(const UiProfileDefinition& profile, const juce::String& componentId)
{
    const auto it = profile.startRectOverrides.find(componentId);
    if (it == profile.startRectOverrides.end())
        return std::nullopt;
    return it->second;
}

inline std::optional<juce::Rectangle<int>> getConfigRectOverride(const UiProfileDefinition& profile, const juce::String& componentId)
{
    const auto it = profile.configRectOverrides.find(componentId);
    if (it == profile.configRectOverrides.end())
        return std::nullopt;
    return it->second;
}

inline std::optional<float> getUiFontScaleOverride(const UiProfileDefinition& profile, const juce::String& componentId)
{
    const auto it = profile.fontScaleOverrides.find(componentId);
    if (it == profile.fontScaleOverrides.end())
        return std::nullopt;
    return it->second;
}

/** Restores last used panel/config basenames if those files still exist. */
inline void loadLastSessionStateFromFile(AppState& state)
{
    const juce::File f = getLastSessionStateFile();
    if (!f.existsAsFile())
        return;

    juce::FileInputStream in(f);
    if (!in.openedOk())
        return;

    const juce::var parsed = juce::JSON::parse(in.readEntireStreamAsString());
    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr)
        return;

    const juce::String panelFile = obj->getProperty("panelFile").toString().trim();
    if (panelFile.isNotEmpty())
    {
        const juce::File resolvedPanel = getDefaultPanelsDirectory().getChildFile(panelFile);
        if (resolvedPanel.existsAsFile())
        {
            state.panelfname = panelFile;
            state.panelfullpathname = {};
        }
    }

    const juce::String configFile = obj->getProperty("configFile").toString().trim();
    if (configFile.isNotEmpty())
    {
        const juce::File resolvedConfig = getOrganUserDocumentsRoot()
            .getChildFile(state.configdir)
            .getChildFile(configFile);
        if (resolvedConfig.existsAsFile())
            state.configfname = configFile;
    }
}

/** Saves last used panel/config basenames for startup restore. */
inline bool saveLastSessionStateToFile(const AppState& state)
{
    const juce::File f = getLastSessionStateFile();
    if (!f.getParentDirectory().createDirectory())
        return false;

    const juce::String panelJson = juce::JSON::toString(juce::var(state.panelfname), false);
    const juce::String configJson = juce::JSON::toString(juce::var(state.configfname), false);
    const juce::String txt = "{\n"
                             "  \"panelFile\": " + panelJson + ",\n"
                             "  \"configFile\": " + configJson + "\n"
                             "}\n";
    return f.replaceWithText(txt);
}

// Volume model helpers
static int sliderStepToMasterCc7(int sliderStep)
{
    const auto clampedStep = juce::jlimit(0, 10, sliderStep);
    return juce::jlimit(0, 127, juce::roundToInt((clampedStep / 10.0) * 127.0));
}

static int computeEffectiveVolumeCc7(int masterCc7, int effectVol, int defaultEffectsVolValue)
{
    const auto master = juce::jlimit(0, 127, masterCc7);
    const auto effect = juce::jlimit(0, 127, effectVol);
    const auto defaultVol = juce::jmax(1, defaultEffectsVolValue);
    const auto computed = juce::roundToInt(master * (effect / static_cast<double>(defaultVol)));
    return juce::jlimit(0, 127, computed);
}

/** Result of scanning .pnl files under the AMidiOrgan user folder for a config basename match. */
struct PanelScanResult
{
    int panelsScanned = 0;
    int referencingCount = 0;
};

/** True if any per-group MIDI module index differs between the two snapshots (config "hard" change). */
inline bool moduleIdxBaselineDiffersFromCurrent(const std::array<int, numberbuttongroups>& baseline,
                                                const std::array<int, numberbuttongroups>& current) noexcept
{
    for (size_t i = 0; i < baseline.size(); ++i)
        if (baseline[i] != current[i])
            return true;

    return false;
}

/**
 * Recursively finds *.pnl under organRoot (typically Documents/AMidiOrgan, including panels/,
 * configs-free subtrees, and legacy instruments/), reads each root ValueTree, and counts panels whose
 * embedded configfilename property equals configBaseName (e.g. "amidiconfigs.cfg").
 */
inline PanelScanResult countPanelsReferencingConfigFile(const juce::File& organRoot,
                                                        juce::StringRef configBaseName)
{
    PanelScanResult r;

    if (!organRoot.isDirectory())
        return r;

    static const juce::Identifier instrumentPanelType("InstrumentPanel");
    static const juce::Identifier configfilenameType("configfilename");

    for (const auto& entry : juce::RangedDirectoryIterator(organRoot, true, "*.pnl", juce::File::findFiles))
    {
        const juce::File panelFile = entry.getFile();
        if (!panelFile.existsAsFile())
            continue;

        ++r.panelsScanned;
        juce::FileInputStream in(panelFile);
        if (in.openedOk())
        {
            juce::ValueTree vt = juce::ValueTree::readFromStream(in);
            if (vt.isValid() && vt.hasType(instrumentPanelType))
            {
                const juce::String embedded = vt.getProperty(configfilenameType).toString();
                if (embedded == configBaseName)
                    ++r.referencingCount;
            }
        }
    }

    return r;
}

/** Read root `configfilename` from a .pnl ValueTree on disk; empty string if unreadable or missing. */
inline juce::String readEmbeddedConfigFilenameFromPanelFile(const juce::File& pnlFile)
{
    static const juce::Identifier instrumentPanelType("InstrumentPanel");
    static const juce::Identifier configfilenameType("configfilename");

    if (!pnlFile.existsAsFile())
        return {};

    juce::FileInputStream in(pnlFile);
    if (!in.openedOk())
        return {};

    juce::ValueTree vt = juce::ValueTree::readFromStream(in);
    if (!vt.isValid() || !vt.hasType(instrumentPanelType))
        return {};

    return vt.getProperty(configfilenameType).toString();
}

/** Clear acknowledged pairing drift once active cfg basename matches panel-embedded basename. */
inline void clearConfigPanelPairingMismatchIfAligned(AppState& state)
{
    if (state.configfname == state.pnlconfigfname)
        state.configPanelPairingMismatchAcknowledged = false;
}

/** Optional: set from UI so keyboard Save buttons refresh after panel save clears pairing state. */
inline std::function<void()> gNotifyPanelSaveAvailabilityChanged;

/** Optional: after panel load, sync Upper/Lower manual rotary buttons from AppState. */
inline std::function<void()> gNotifyManualRotarySyncFromAppState;

/** Optional: after preset recall, sync Upper/Lower rotary UI from ButtonGroup::rotary. */
inline std::function<void()> gNotifyPresetRotarySyncFromButtonGroups;

/** Optional: refresh Preset 1..12 text colours after Set or panel load (all KeyboardPanelPage tabs). */
inline std::function<void()> gNotifyPresetConfiguredStyle;

/** Optional: refresh keyboard voice button text colours after Sounds edits or panel load. */
inline std::function<void()> gNotifyVoiceConfiguredStyle;

/** Optional: refresh Sounds/Effects tab enabled state when voice-edit gating changes. */
inline std::function<void()> gNotifyVoiceEditTabAccessChanged;

/** Optional: refresh Start/Keyboard/Config panel+config status labels after filename/state changes. */
inline std::function<void()> gNotifyStatusLinesChanged;

/** Optional: apply selected UI profile to live tab layouts when changed in Config tab. */
inline std::function<void()> gNotifyUiProfileChanged;

/** Preset rotary encoding in .pnl per ButtonGroup: 0=slow, 1=fast, 2=brake (see tests). */
inline int encodePresetRotaryFromManual(bool isFast, bool brake) noexcept
{
    if (brake)
        return 2;
    return isFast ? 1 : 0;
}

inline void decodePresetRotary(int r, bool& isFast, bool& brake) noexcept
{
    const int rr = juce::jlimit(0, 2, r);
    brake = (rr == 2);
    isFast = (rr == 1);
}

/** User-facing text for Save / Save As / Save and Exit when pairing mismatch is acknowledged. */
inline juce::String getPanelSavePairingMismatchMessage()
{
    return "Saving will embed the current active config name into the panel file.\n\n"
           "Routing may not match this song/style until configs align.\n\n"
           "Save anyway?";
}

// To do: Consider moving quick access static Midi in keyboard handler vars below directly 
// from config classes. Updated during Config save or loads.
static int lowerchan = 3;
static int lowersolo = 15;

static int upperchan = 4;
static int uppersolo = 1;


//==============================================================================
// Sliders can have custom snapping applied to their values,
// This simple class snaps the value to 50 if it comes near.
struct SnappingSlider final : public Slider
{
    double snapValue(double attemptedValue, DragMode dragMode) override
    {
        if (dragMode == notDragging)
            return attemptedValue;  // if they're entering the value in the text-box, don't mess with it.

        if (attemptedValue > 40 && attemptedValue < 60)
            return 50.0;

        return attemptedValue;
    }
};


//==============================================================================
// Types: Various Enums
//==============================================================================
enum MidiCCType {
    CCVol = 7,
    CCExp = 11,
    CCRev = 91,
    CCCho = 93,
    CCMod = 1,
    CCTim = 71,
    CCAtk = 73,
    CCRel = 72,
    CCBri = 74,
    CCPan = 10,

    CCMSB = 0x00,
    CCLSB = 0x20
};

enum PanelTabsType {
    PTStart = 0,
    PTUpper = 1,
    PTLower = 2,
    PTBass = 3,
    PTVoices = 4,
    PTEffects = 5,
    PTConfig = 6,
    PTHotkeys = 7,
    PTMonitor = 8,
    PTHelp = 9,
    PTPlayer = 10,
    PTExit = 11
};

enum EffectsMap {
    MAPVOL = 2,
    MAPEXP = 4,
    MAPREV = 8,
    MAPCH0 = 16,
    MAPMOD = 32,
    MAPTIM = 64,
    MAPATK = 128,
    MAPREL = 256,
    MAPBRI = 512,
    MAPPAN = 1024
};

// Index Offset of a Button Group in Panel
enum IndexButtonGroupsType {
    UPPERG1 = 0,
    UPPERG2 = 12,
    UPPERG3 = 20,
    UPPERG4 = 26,
    LOWERG1 = 32,
    LOWERG2 = 44,
    LOWERG3 = 52,
    LOWERG4 = 58,
    BASSG1 = 64,
    BASSG2 = 76,
    BASSG3 = 84,
    BASSG4 = 90,
};

// Types: MIDI IO Channel Mappings
enum ButtonGroupType {
    BGORGAN = 1,
    BGSYMPHONIC = 2,
    BGORCHESTRAL = 3,
    BGSOLO = 4,
};

enum MidiKBDType {
    KBDSOLO = 1,
    KBDBASS = 2,
    KBDLOWER = 3,
    KBDUPPER = 4,
};

enum MidiIOType {
    IN1 = 0,
    OUT1 = 1,
    OUT2 = 2,
    OUT3 = 3,
    OUT4 = 4
};


// Reverse Lookup of Button to Button Group
static int lookupPanelGroup(int panelbuttonidx) {
    if (panelbuttonidx < UPPERG2) return 0;
    else if (panelbuttonidx < UPPERG3) return 1;
    else if (panelbuttonidx < UPPERG4) return 2;
    else if (panelbuttonidx < LOWERG1) return 3;
    else if (panelbuttonidx < LOWERG2) return 4;
    else if (panelbuttonidx < LOWERG3) return 5;
    else if (panelbuttonidx < LOWERG4) return 6;
    else if (panelbuttonidx < BASSG1) return 7;
    else if (panelbuttonidx < BASSG2) return 8;
    else if (panelbuttonidx < BASSG3) return 9;
    else if (panelbuttonidx < BASSG4) return 10;
    else return 11;
};


//-------------------------------------------------------------------------
// Keyboard Shortcuts Implementation
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
/*
Here's a brief overview of how you might use the ApplicationCommandManager:

Define Commands: First, you define the commands your application will support. Each command 
is given a unique ID and a description.

Register Commands: You then register these commands with the ApplicationCommandManager, 
typically during your application's initialization phase. This registration process 
involves creating an instance of ApplicationCommandManager and using it to register 
all your commands.

Map Commands to Actions: Once commands are registered, you can map them to specific
actions. This involves creating a class that inherits from ApplicationCommandTarget, 
which provides the logic for what should happen when a command is invoked.

Attach Triggers: The ApplicationCommandManager also allows you to attach various triggers
to commands, such as keyboard shortcuts or GUI elements like buttons or menu items. This
way, when the trigger is activated (e.g., a keyboard key is pressed), the corresponding 
command is executed.

Handling Commands: Finally, when a command is invoked (e.g., through a keyboard shortcut 
or a menu selection), the ApplicationCommandManager routes the command to the appropriate
ApplicationCommandTarget that can handle it, executing the associated action.

https://forum.juce.com/t/keylistener-documentation-request/27360
*/

// Keyboard shortcut command IDs (Phase 1 — see README “Keyboard shortcuts”)
enum KeyPressCommandIDs {
    btnTabUpper = 1,
    btnTabLower = 2,
    btnTabBass = 3,
    btnTabSounds = 4,
    btnTabEffects = 5,
    btnTabMonitor = 6,
    btnPreset0 = 10,
    btnPreset1 = 11,
    btnPreset2 = 12,
    btnPreset3 = 13,
    btnPreset4 = 14,
    btnPreset5 = 15,
    btnPreset6 = 16,
    btnPresetNext = 17,
    btnUpperRotaryFastSlow = 20,
    btnUpperRotaryBrake = 21,
    btnLowerRotaryFastSlow = 22,
    btnLowerRotaryBrake = 23
};

constexpr int kNumHotkeyCommands = 18;

inline constexpr std::array<KeyPressCommandIDs, kNumHotkeyCommands> kHotkeyCommandOrder = {
    KeyPressCommandIDs::btnTabUpper,
    KeyPressCommandIDs::btnTabLower,
    KeyPressCommandIDs::btnTabBass,
    KeyPressCommandIDs::btnTabSounds,
    KeyPressCommandIDs::btnTabEffects,
    KeyPressCommandIDs::btnTabMonitor,
    KeyPressCommandIDs::btnPreset0,
    KeyPressCommandIDs::btnPreset1,
    KeyPressCommandIDs::btnPreset2,
    KeyPressCommandIDs::btnPreset3,
    KeyPressCommandIDs::btnPreset4,
    KeyPressCommandIDs::btnPreset5,
    KeyPressCommandIDs::btnPreset6,
    KeyPressCommandIDs::btnPresetNext,
    KeyPressCommandIDs::btnUpperRotaryFastSlow,
    KeyPressCommandIDs::btnUpperRotaryBrake,
    KeyPressCommandIDs::btnLowerRotaryFastSlow,
    KeyPressCommandIDs::btnLowerRotaryBrake
};

struct HotkeyBindings
{
    std::array<std::optional<juce_wchar>, kNumHotkeyCommands> keys;

    static HotkeyBindings withDefaults();

    std::optional<juce_wchar> keyFor(CommandID commandID) const;
    void setKeyFor(CommandID commandID, std::optional<juce_wchar> k);
};

inline int indexOfHotkeyCommand(CommandID id) noexcept
{
    for (int i = 0; i < kNumHotkeyCommands; ++i)
        if (static_cast<CommandID>(kHotkeyCommandOrder[(size_t) i]) == id)
            return i;

    return -1;
}

inline HotkeyBindings HotkeyBindings::withDefaults()
{
    HotkeyBindings b;
    b.keys[0] = L'a';
    b.keys[1] = L's';
    b.keys[2] = L'd';
    b.keys[3] = L'z';
    b.keys[4] = L'x';
    b.keys[5] = L'm';
    b.keys[6] = L'0';
    b.keys[7] = L'1';
    b.keys[8] = L'2';
    b.keys[9] = L'3';
    b.keys[10] = L'4';
    b.keys[11] = L'5';
    b.keys[12] = L'6';
    b.keys[13] = L'7';
    b.keys[14] = L'f';
    b.keys[15] = L'b';
    b.keys[16] = L'g';
    b.keys[17] = L'n';
    return b;
}

inline std::optional<juce_wchar> HotkeyBindings::keyFor(CommandID commandID) const
{
    const int idx = indexOfHotkeyCommand(commandID);
    if (idx < 0 || idx >= kNumHotkeyCommands)
        return std::nullopt;
    return keys[(size_t) idx];
}

inline void HotkeyBindings::setKeyFor(CommandID commandID, std::optional<juce_wchar> k)
{
    const int idx = indexOfHotkeyCommand(commandID);
    if (idx < 0 || idx >= kNumHotkeyCommands)
        return;
    keys[(size_t) idx] = k;
}


class KeyPressTarget final : public Component,
    public ApplicationCommandTarget
{
public:
    KeyPressTarget() = default;

    void setHotkeyBindings(const HotkeyBindings& b) { hotkeyBindings = b; }
    const HotkeyBindings& getHotkeyBindings() const { return hotkeyBindings; }

    //==============================================================================
    ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }

    void getAllCommands(Array<CommandID>& commands) override
    {
        const CommandID ids[] = {
            KeyPressCommandIDs::btnTabUpper,
            KeyPressCommandIDs::btnTabLower,
            KeyPressCommandIDs::btnTabBass,
            KeyPressCommandIDs::btnTabSounds,
            KeyPressCommandIDs::btnTabEffects,
            KeyPressCommandIDs::btnTabMonitor,
            KeyPressCommandIDs::btnPreset0,
            KeyPressCommandIDs::btnPreset1,
            KeyPressCommandIDs::btnPreset2,
            KeyPressCommandIDs::btnPreset3,
            KeyPressCommandIDs::btnPreset4,
            KeyPressCommandIDs::btnPreset5,
            KeyPressCommandIDs::btnPreset6,
            KeyPressCommandIDs::btnPresetNext,
            KeyPressCommandIDs::btnUpperRotaryFastSlow,
            KeyPressCommandIDs::btnUpperRotaryBrake,
            KeyPressCommandIDs::btnLowerRotaryFastSlow,
            KeyPressCommandIDs::btnLowerRotaryBrake
        };
        commands.addArray(ids, (int) (sizeof(ids) / sizeof(ids[0])));
    }

    void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override
    {
        auto addShortcutInfo = [this, commandID, &result](const char* name, const char* desc)
        {
            result.setInfo(name, desc, "Shortcuts", 0);
            if (auto k = hotkeyBindings.keyFor(commandID))
                result.addDefaultKeypress((int) *k, ModifierKeys::noModifiers);
        };

        switch (commandID)
        {
            case KeyPressCommandIDs::btnTabUpper:
                addShortcutInfo("Upper tab", "Show Upper keyboard");
                break;
            case KeyPressCommandIDs::btnTabLower:
                addShortcutInfo("Lower tab", "Show Lower keyboard");
                break;
            case KeyPressCommandIDs::btnTabBass:
                addShortcutInfo("Bass tab", "Show Bass & Drums keyboard");
                break;
            case KeyPressCommandIDs::btnTabSounds:
                addShortcutInfo("Sounds tab", "Show Sounds tab");
                break;
            case KeyPressCommandIDs::btnTabEffects:
                addShortcutInfo("Effects tab", "Show Effects tab");
                break;
            case KeyPressCommandIDs::btnTabMonitor:
                addShortcutInfo("Monitor tab", "Show Monitor tab");
                break;
            case KeyPressCommandIDs::btnPreset0:
                addShortcutInfo("Manual preset", "Recall Manual preset");
                break;
            case KeyPressCommandIDs::btnPreset1:
                addShortcutInfo("Preset 1", "Recall Preset 1");
                break;
            case KeyPressCommandIDs::btnPreset2:
                addShortcutInfo("Preset 2", "Recall Preset 2");
                break;
            case KeyPressCommandIDs::btnPreset3:
                addShortcutInfo("Preset 3", "Recall Preset 3");
                break;
            case KeyPressCommandIDs::btnPreset4:
                addShortcutInfo("Preset 4", "Recall Preset 4");
                break;
            case KeyPressCommandIDs::btnPreset5:
                addShortcutInfo("Preset 5", "Recall Preset 5");
                break;
            case KeyPressCommandIDs::btnPreset6:
                addShortcutInfo("Preset 6", "Recall Preset 6");
                break;
            case KeyPressCommandIDs::btnPresetNext:
                addShortcutInfo("Next preset", "Advance to next preset (cycles 1 to 12)");
                break;
            case KeyPressCommandIDs::btnUpperRotaryFastSlow:
                addShortcutInfo("Upper rotary Fast/Slow", "Upper manual rotary Fast/Slow");
                break;
            case KeyPressCommandIDs::btnUpperRotaryBrake:
                addShortcutInfo("Upper rotary Brake", "Upper manual rotary Brake");
                break;
            case KeyPressCommandIDs::btnLowerRotaryFastSlow:
                addShortcutInfo("Lower rotary Fast/Slow", "Lower manual rotary Fast/Slow");
                break;
            case KeyPressCommandIDs::btnLowerRotaryBrake:
                addShortcutInfo("Lower rotary Brake", "Lower manual rotary Brake");
                break;
            default:
                break;
        }
    }

    bool perform(const InvocationInfo& info) override
    {
        switch (info.commandID)
        {
            case KeyPressCommandIDs::btnTabUpper:
                if (onTabUpper) onTabUpper();
                else return false;
                break;
            case KeyPressCommandIDs::btnTabLower:
                if (onTabLower) onTabLower();
                else return false;
                break;
            case KeyPressCommandIDs::btnTabBass:
                if (onTabBass) onTabBass();
                else return false;
                break;
            case KeyPressCommandIDs::btnTabSounds:
                if (onVoiceEditTabHotkeysAllowed && !onVoiceEditTabHotkeysAllowed())
                    return false;
                if (onTabSounds) onTabSounds();
                else return false;
                break;
            case KeyPressCommandIDs::btnTabEffects:
                if (onVoiceEditTabHotkeysAllowed && !onVoiceEditTabHotkeysAllowed())
                    return false;
                if (onTabEffects) onTabEffects();
                else return false;
                break;
            case KeyPressCommandIDs::btnTabMonitor:
                if (onTabMonitor) onTabMonitor();
                else return false;
                break;
            case KeyPressCommandIDs::btnPreset0:
                if (onPresetRecall) onPresetRecall(0);
                else return false;
                break;
            case KeyPressCommandIDs::btnPreset1:
                if (onPresetRecall) onPresetRecall(1);
                else return false;
                break;
            case KeyPressCommandIDs::btnPreset2:
                if (onPresetRecall) onPresetRecall(2);
                else return false;
                break;
            case KeyPressCommandIDs::btnPreset3:
                if (onPresetRecall) onPresetRecall(3);
                else return false;
                break;
            case KeyPressCommandIDs::btnPreset4:
                if (onPresetRecall) onPresetRecall(4);
                else return false;
                break;
            case KeyPressCommandIDs::btnPreset5:
                if (onPresetRecall) onPresetRecall(5);
                else return false;
                break;
            case KeyPressCommandIDs::btnPreset6:
                if (onPresetRecall) onPresetRecall(6);
                else return false;
                break;
            case KeyPressCommandIDs::btnPresetNext:
                if (onPresetNext) onPresetNext();
                else return false;
                break;
            case KeyPressCommandIDs::btnUpperRotaryFastSlow:
                if (onUpperRotaryFastSlow) onUpperRotaryFastSlow();
                else return false;
                break;
            case KeyPressCommandIDs::btnUpperRotaryBrake:
                if (onUpperRotaryBrake) onUpperRotaryBrake();
                else return false;
                break;
            case KeyPressCommandIDs::btnLowerRotaryFastSlow:
                if (onLowerRotaryFastSlow) onLowerRotaryFastSlow();
                else return false;
                break;
            case KeyPressCommandIDs::btnLowerRotaryBrake:
                if (onLowerRotaryBrake) onLowerRotaryBrake();
                else return false;
                break;
            default:
                return false;
        }

        return true;
    }

    std::function<void()> onTabUpper;
    std::function<void()> onTabLower;
    std::function<void()> onTabBass;
    std::function<void()> onTabSounds;
    std::function<void()> onTabEffects;
    std::function<void()> onTabMonitor;
    /** If set, Sounds/Effects tab hotkeys are ignored when this returns false (e.g. no voice selected yet). */
    std::function<bool()> onVoiceEditTabHotkeysAllowed;
    std::function<void(int)> onPresetRecall;
    std::function<void()> onPresetNext;
    std::function<void()> onUpperRotaryFastSlow;
    std::function<void()> onUpperRotaryBrake;
    std::function<void()> onLowerRotaryFastSlow;
    std::function<void()> onLowerRotaryBrake;

private:
    HotkeyBindings hotkeyBindings { HotkeyBindings::withDefaults() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyPressTarget)
};

/** When true, skip ApplicationCommandManager shortcut handling so keys reach text fields and modal dialogs. */
inline bool shouldDeferKeyboardShortcutsForFocusedComponent (Component* focused) noexcept
{
    if (focused == nullptr) return false;
    if (dynamic_cast<TextEditor*> (focused) != nullptr) return true;
    if (dynamic_cast<ComboBox*> (focused) != nullptr) return true;
    if (focused->findParentComponentOfClass<AlertWindow>() != nullptr) return true;
    return false;
}

inline bool shouldDeferKeyboardShortcutsForFocusedComponent() noexcept
{
    return shouldDeferKeyboardShortcutsForFocusedComponent (Component::getCurrentlyFocusedComponent());
}

/** Wraps KeyPressMappingSet so global shortcuts do not fire while typing in editors or system dialogs. */
class ShortcutRoutingKeyListener final : public KeyListener
{
public:
    explicit ShortcutRoutingKeyListener (KeyPressMappingSet* mappingSetIn) noexcept : mappings (mappingSetIn) {}

    bool keyPressed (const KeyPress& key, Component* originatingComponent) override
    {
        if (mappings == nullptr) return false;
        if (shouldDeferKeyboardShortcutsForFocusedComponent()) return false;
        return mappings->keyPressed (key, originatingComponent);
    }

    bool keyStateChanged (bool isKeyDown, Component* originatingComponent) override
    {
        if (mappings == nullptr) return false;
        if (shouldDeferKeyboardShortcutsForFocusedComponent()) return false;
        return mappings->keyStateChanged (isKeyDown, originatingComponent);
    }

private:
    KeyPressMappingSet* mappings = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShortcutRoutingKeyListener)
};


class AMidiTrayIcon final : public SystemTrayIconComponent {
public:
    AMidiTrayIcon() {

        juce::Image trayimage = juce::ImageFileFormat::loadFrom(BinaryData::keyboard_png, BinaryData::keyboard_pngSize);

        juce::SystemTrayIconComponent::setIconImage(trayimage, trayimage);
        setIconTooltip("AMidiOrgan");
    }

    ~AMidiTrayIcon() {}
};


//-------------------------------------------------------------------------
// Utilities and static functions
//-------------------------------------------------------------------------

static int check0to127(int val) {
    if (val < 0)
        val = 0;
    else if (val > 127)
        val = 127;
    return val;
}

static int check1to16(int val) {
    if (val < 1)
        val = 1;
    else if (val > 16)
        val = 16;
    return val;
}

