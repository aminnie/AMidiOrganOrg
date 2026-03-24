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
static const int numberpresets = 7;

static const int defvelocityout = 64;

static const int numbermodules = 6;

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
    /** Manual Leslie Fast/Slow + Brake UI on Lower keyboard tab (saved on panel root). */
    bool lowerManualRotaryFast = true;
    bool lowerManualRotaryBrake = false;
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

/** Optional: refresh Sounds/Effects tab enabled state when voice-edit gating changes. */
inline std::function<void()> gNotifyVoiceEditTabAccessChanged;

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
    PTHelp = 8,
    PTExit = 9
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
    btnPreset0 = 10,
    btnPreset1 = 11,
    btnPreset2 = 12,
    btnPreset3 = 13,
    btnPreset4 = 14,
    btnPreset5 = 15,
    btnPreset6 = 16,
    btnUpperRotaryFastSlow = 20,
    btnUpperRotaryBrake = 21,
    btnLowerRotaryFastSlow = 22,
    btnLowerRotaryBrake = 23
};

constexpr int kNumHotkeyCommands = 16;

inline constexpr std::array<KeyPressCommandIDs, kNumHotkeyCommands> kHotkeyCommandOrder = {
    KeyPressCommandIDs::btnTabUpper,
    KeyPressCommandIDs::btnTabLower,
    KeyPressCommandIDs::btnTabBass,
    KeyPressCommandIDs::btnTabSounds,
    KeyPressCommandIDs::btnTabEffects,
    KeyPressCommandIDs::btnPreset0,
    KeyPressCommandIDs::btnPreset1,
    KeyPressCommandIDs::btnPreset2,
    KeyPressCommandIDs::btnPreset3,
    KeyPressCommandIDs::btnPreset4,
    KeyPressCommandIDs::btnPreset5,
    KeyPressCommandIDs::btnPreset6,
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
    b.keys[5] = L'0';
    b.keys[6] = L'1';
    b.keys[7] = L'2';
    b.keys[8] = L'3';
    b.keys[9] = L'4';
    b.keys[10] = L'5';
    b.keys[11] = L'6';
    b.keys[12] = L'f';
    b.keys[13] = L'b';
    b.keys[14] = L'g';
    b.keys[15] = L'n';
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
            KeyPressCommandIDs::btnPreset0,
            KeyPressCommandIDs::btnPreset1,
            KeyPressCommandIDs::btnPreset2,
            KeyPressCommandIDs::btnPreset3,
            KeyPressCommandIDs::btnPreset4,
            KeyPressCommandIDs::btnPreset5,
            KeyPressCommandIDs::btnPreset6,
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
    /** If set, Sounds/Effects tab hotkeys are ignored when this returns false (e.g. no voice selected yet). */
    std::function<bool()> onVoiceEditTabHotkeysAllowed;
    std::function<void(int)> onPresetRecall;
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

