#include <JuceHeader.h>

using namespace juce;

#include "../AMidiUtils.h"
#include "../AMidiDevices.h"
#include "../AMidiInstruments.h"
#include "../AMidiControl.h"

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstdlib>
#include <utility>

namespace
{
    struct TestResult
    {
        std::string name;
        bool passed;
        std::string details;
    };

    bool expectEqual(int actual, int expected, const std::string& label, std::string& details)
    {
        if (actual != expected)
        {
            details = label + " expected " + std::to_string(expected) + " but got " + std::to_string(actual);
            return false;
        }
        return true;
    }

    bool expectEqualStr(const String& actual, const String& expected, const std::string& label, std::string& details)
    {
        if (actual != expected)
        {
            details = label + " expected '" + expected.toStdString() + "' but got '" + actual.toStdString() + "'";
            return false;
        }
        return true;
    }

    struct AppStateSnapshot
    {
        String instrumentdir;
        String instrumentfname;
        String panelfname;
        String configfname;
        String pnlconfigfname;
        int defaultEffectsVol = 100;
        bool configreload = false;
        bool configPanelPairingMismatchAcknowledged = false;
    };

    AppStateSnapshot takeAppStateSnapshot()
    {
        auto& state = getAppState();
        AppStateSnapshot snapshot;
        snapshot.instrumentdir = state.instrumentdir;
        snapshot.instrumentfname = state.instrumentfname;
        snapshot.panelfname = state.panelfname;
        snapshot.configfname = state.configfname;
        snapshot.pnlconfigfname = state.pnlconfigfname;
        snapshot.defaultEffectsVol = state.defaultEffectsVol;
        snapshot.configreload = state.configreload;
        snapshot.configPanelPairingMismatchAcknowledged = state.configPanelPairingMismatchAcknowledged;
        return snapshot;
    }

    void restoreAppState(const AppStateSnapshot& snapshot)
    {
        auto& state = getAppState();
        state.instrumentdir = snapshot.instrumentdir;
        state.instrumentfname = snapshot.instrumentfname;
        state.panelfname = snapshot.panelfname;
        state.configfname = snapshot.configfname;
        state.pnlconfigfname = snapshot.pnlconfigfname;
        state.defaultEffectsVol = snapshot.defaultEffectsVol;
        state.configreload = snapshot.configreload;
        state.configPanelPairingMismatchAcknowledged = snapshot.configPanelPairingMismatchAcknowledged;
    }

    bool prepareTestInstrumentJson(const String& testDirName, const String& fileName, std::string& details)
    {
        auto& state = getAppState();

        const File baseDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(testDirName);

        if (!baseDir.exists() && !baseDir.createDirectory())
        {
            details = "Failed to create test directory: " + baseDir.getFullPathName().toStdString();
            return false;
        }

        const File instrumentJson = baseDir.getChildFile(fileName);
        const String jsonText = R"({
  "Vendor": "TestVendor",
  "Instruments": [
    []
  ]
})";

        if (!instrumentJson.replaceWithText(jsonText))
        {
            details = "Failed to write test JSON: " + instrumentJson.getFullPathName().toStdString();
            return false;
        }

        state.instrumentdir = testDirName;
        state.instrumentfname = fileName;
        return true;
    }

    bool runCheck0to127(std::string& details)
    {
        return expectEqual(check0to127(-5), 0, "check0to127(-5)", details)
            && expectEqual(check0to127(0), 0, "check0to127(0)", details)
            && expectEqual(check0to127(64), 64, "check0to127(64)", details)
            && expectEqual(check0to127(127), 127, "check0to127(127)", details)
            && expectEqual(check0to127(200), 127, "check0to127(200)", details);
    }

    bool runCheck1to16(std::string& details)
    {
        return expectEqual(check1to16(0), 1, "check1to16(0)", details)
            && expectEqual(check1to16(1), 1, "check1to16(1)", details)
            && expectEqual(check1to16(8), 8, "check1to16(8)", details)
            && expectEqual(check1to16(16), 16, "check1to16(16)", details)
            && expectEqual(check1to16(99), 16, "check1to16(99)", details);
    }

    bool runVolumeMathHelpers(std::string& details)
    {
        if (!expectEqual(sliderStepToMasterCc7(0), 0, "sliderStepToMasterCc7(0)", details) ||
            !expectEqual(sliderStepToMasterCc7(8), 102, "sliderStepToMasterCc7(8)", details) ||
            !expectEqual(sliderStepToMasterCc7(10), 127, "sliderStepToMasterCc7(10)", details))
        {
            return false;
        }

        if (!expectEqual(computeEffectiveVolumeCc7(102, 100, 100), 102,
                         "computeEffectiveVolumeCc7 baseline passthrough", details) ||
            !expectEqual(computeEffectiveVolumeCc7(102, 50, 100), 51,
                         "computeEffectiveVolumeCc7 half trim", details) ||
            !expectEqual(computeEffectiveVolumeCc7(127, 127, 100), 127,
                         "computeEffectiveVolumeCc7 clamps high", details))
        {
            return false;
        }

        return expectEqual(computeEffectiveVolumeCc7(100, 100, 0), 127,
                           "computeEffectiveVolumeCc7 guards default denominator", details);
    }

    bool runModuleIdxBaselineDiffers(std::string& details)
    {
        std::array<int, numberbuttongroups> a{};
        std::array<int, numberbuttongroups> b{};

        for (int i = 0; i < numberbuttongroups; ++i)
        {
            a[(size_t) i] = i;
            b[(size_t) i] = i;
        }

        if (moduleIdxBaselineDiffersFromCurrent(a, b))
        {
            details = "identical module indices should not report a difference";
            return false;
        }

        b[0] = 99;
        if (!moduleIdxBaselineDiffersFromCurrent(a, b))
        {
            details = "changed module index should report a difference";
            return false;
        }

        return true;
    }

    bool runPanelConfigReferencingScan(std::string& details)
    {
        const File tempRoot = File::getSpecialLocation(File::tempDirectory)
            .getChildFile("AMidiOrgan_panel_cfg_scan_test");

        if (tempRoot.exists())
            tempRoot.deleteRecursively();

        if (!tempRoot.createDirectory())
        {
            details = "failed to create temp directory for panel scan test";
            return false;
        }

        struct Cleanup { File f; ~Cleanup() { f.deleteRecursively(); } } cleanup{ tempRoot };

        const File sub = tempRoot.getChildFile("nested");
        if (!sub.createDirectory())
        {
            details = "failed to create nested temp directory";
            return false;
        }

        static const Identifier instrumentPanelType("InstrumentPanel");
        static const Identifier configfilenameType("configfilename");

        auto writePanel = [&](const File& file, const String& cfgName) -> bool
        {
            ValueTree vt(instrumentPanelType);
            vt.setProperty(configfilenameType, cfgName, nullptr);
            FileOutputStream out(file);
            if (!out.openedOk())
                return false;
            vt.writeToStream(out);
            out.flush();
            return out.getStatus().wasOk();
        };

        if (!writePanel(tempRoot.getChildFile("song_a.pnl"), "shared.cfg"))
        {
            details = "failed to write song_a.pnl";
            return false;
        }

        if (!writePanel(sub.getChildFile("song_b.pnl"), "other.cfg"))
        {
            details = "failed to write song_b.pnl";
            return false;
        }

        const auto scan = countPanelsReferencingConfigFile(tempRoot, "shared.cfg");

        if (!expectEqual(scan.panelsScanned, 2, "panelsScanned", details))
            return false;

        if (!expectEqual(scan.referencingCount, 1, "referencingCount", details))
            return false;

        return true;
    }

    bool runReadEmbeddedConfigFilenameFromPanelFile(std::string& details)
    {
        const File tempRoot = File::getSpecialLocation(File::tempDirectory)
            .getChildFile("AMidiOrgan_read_embed_cfg_test");

        if (tempRoot.exists())
            tempRoot.deleteRecursively();

        if (!tempRoot.createDirectory())
        {
            details = "failed to create temp directory for read embed test";
            return false;
        }

        struct Cleanup { File f; ~Cleanup() { f.deleteRecursively(); } } cleanup{ tempRoot };

        static const Identifier instrumentPanelType("InstrumentPanel");
        static const Identifier configfilenameType("configfilename");

        ValueTree vt(instrumentPanelType);
        vt.setProperty(configfilenameType, "my_embedded_rig.cfg", nullptr);

        const File panelFile = tempRoot.getChildFile("panel_embed_test.pnl");
        FileOutputStream out(panelFile);
        if (!out.openedOk())
        {
            details = "failed to open output stream for test panel";
            return false;
        }
        vt.writeToStream(out);
        out.flush();

        if (!out.getStatus().wasOk())
        {
            details = "failed to write test panel file";
            return false;
        }

        const String got = readEmbeddedConfigFilenameFromPanelFile(panelFile);
        return expectEqualStr(got, "my_embedded_rig.cfg", "readEmbeddedConfigFilenameFromPanelFile", details);
    }

    bool runAppStateAliasBackcompat(std::string& details)
    {
        auto& state = getAppState();

        const auto oldPanelName = panelfname;
        const auto oldModuleIdx = moduleidx;

        panelfname = "testpanel.pnl";
        moduleidx = 4;

        const bool panelOk = (state.panelfname == "testpanel.pnl");
        const bool moduleOk = (state.moduleidx == 4);

        // Restore prior values to avoid side effects between tests.
        panelfname = oldPanelName;
        moduleidx = oldModuleIdx;

        if (!panelOk)
        {
            details = "panelfname alias not synchronized with AppState";
            return false;
        }

        if (!moduleOk)
        {
            details = "moduleidx alias not synchronized with AppState";
            return false;
        }

        return true;
    }

    bool runInstrumentModuleWritesAppState(std::string& details)
    {
        auto* modules = InstrumentModules::getInstance();
        if (modules == nullptr)
        {
            details = "InstrumentModules::getInstance() returned nullptr";
            return false;
        }

        auto& state = getAppState();
        const int previousModule = state.moduleidx;

        // Pick a known module id from constructor defaults.
        const int targetModule = 2;
        modules->setInstrumentModule(targetModule);

        if (!expectEqual(state.moduleidx, targetModule, "setInstrumentModule updates appState.moduleidx", details))
            return false;

        const bool hasVendor = state.vendorname.isNotEmpty();
        const bool hasFile = state.instrumentfname.isNotEmpty();
        if (!hasVendor || !hasFile)
        {
            details = "setInstrumentModule did not populate vendor/instrument file in AppState";
            return false;
        }

        // Restore previous module selection to minimize side effects.
        modules->setInstrumentModule(previousModule);
        return true;
    }

    bool runLookupPanelGroup(std::string& details)
    {
        return expectEqual(lookupPanelGroup(0), 0, "lookupPanelGroup(0)", details)
            && expectEqual(lookupPanelGroup(11), 0, "lookupPanelGroup(11)", details)
            && expectEqual(lookupPanelGroup(12), 1, "lookupPanelGroup(12)", details)
            && expectEqual(lookupPanelGroup(31), 3, "lookupPanelGroup(31)", details)
            && expectEqual(lookupPanelGroup(90), 11, "lookupPanelGroup(90)", details)
            && expectEqual(lookupPanelGroup(95), 11, "lookupPanelGroup(95)", details);
    }

    bool runInstrumentClampBehavior(std::string& details)
    {
        Instrument instrument;

        instrument.setMSB(-10);
        if (!expectEqual(instrument.getMSB(), 0, "Instrument::setMSB(-10)", details))
            return false;

        instrument.setLSB(200);
        if (!expectEqual(instrument.getLSB(), 127, "Instrument::setLSB(200)", details))
            return false;

        instrument.setFont(400);
        if (!expectEqual(instrument.getFont(), 127, "Instrument::setFont(400)", details))
            return false;

        instrument.setVol(-1);
        if (!expectEqual(instrument.getVol(), 0, "Instrument::setVol(-1)", details))
            return false;

        instrument.setPan(300);
        if (!expectEqual(instrument.getPan(), 127, "Instrument::setPan(300)", details))
            return false;

        instrument.setChannel(-1);
        if (!expectEqual(instrument.getChannel(), 1, "Instrument::setChannel(-1)", details))
            return false;

        instrument.setChannel(20);
        if (!expectEqual(instrument.getChannel(), 16, "Instrument::setChannel(20)", details))
            return false;

        instrument.setDirty(MAPVOL);
        instrument.setDirty(MAPMOD);
        const int expectedMask = MAPVOL | MAPMOD;
        return expectEqual(instrument.getDirty(), expectedMask, "Instrument::dirty bitmap OR", details);
    }

    bool runMidiDevicesDefaultMap(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        devices->clearMidiIOMap();

        for (int i = 0; i < 17; ++i)
        {
            if (!expectEqual(devices->iomap[i][0], i, "iomap[i][0] identity map", details))
                return false;

            for (int j = 1; j < 5; ++j)
            {
                if (!expectEqual(devices->iomap[i][j], 0, "iomap[i][j] cleared", details))
                    return false;
            }

            if (!expectEqual(devices->octxpose[i], 0, "octxpose default", details))
                return false;

            if (!expectEqual(devices->splitout[i], 0, "splitout default", details))
                return false;

            if (!expectEqual(devices->moduleout[i], 1, "moduleout default", details))
                return false;

            if (!expectEqual(devices->velocityout[i] ? 1 : 0, 1, "velocityout default true", details))
                return false;

            const int expectedPassthrough = (i == 16) ? 1 : 0;
            if (!expectEqual(devices->passthroughin[i] ? 1 : 0, expectedPassthrough, "passthrough defaults", details))
                return false;
        }

        return true;
    }

    bool runRewriteSendNoteMessageBasics(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        devices->clearMidiIOMap();

        const auto cc = MidiMessage::controllerEvent(1, CCVol, 100);
        if (!expectEqual(devices->rewriteSendNoteMessage(cc, 1) ? 1 : 0, 0, "rewriteSendNoteMessage rejects non-note", details))
            return false;

        const auto noteOn = MidiMessage::noteOn(1, 60, (juce::uint8) 100);
        if (!expectEqual(devices->rewriteSendNoteMessage(noteOn, 1) ? 1 : 0, 1, "rewriteSendNoteMessage accepts note on", details))
            return false;

        const auto noteOff = MidiMessage::noteOff(1, 60, (juce::uint8) 0);
        return expectEqual(devices->rewriteSendNoteMessage(noteOff, 1) ? 1 : 0, 1, "rewriteSendNoteMessage accepts note off", details);
    }

    bool runSplitRoutingBehavior(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        devices->clearMidiIOMap();

        // Lower keyboard split behavior.
        lowerchan = 3;
        lowersolo = 15;
        devices->splitout[lowersolo] = 60;

        if (!expectEqual(devices->shouldRouteLayeredNote(3, 59, 4) ? 1 : 0, 1,
                         "lower split: below split routes non-solo", details))
            return false;

        if (!expectEqual(devices->shouldRouteLayeredNote(3, 59, 15) ? 1 : 0, 0,
                         "lower split: below split blocks solo", details))
            return false;

        if (!expectEqual(devices->shouldRouteLayeredNote(3, 60, 4) ? 1 : 0, 0,
                         "lower split: at split blocks non-solo", details))
            return false;

        if (!expectEqual(devices->shouldRouteLayeredNote(3, 60, 15) ? 1 : 0, 1,
                         "lower split: at split routes solo", details))
            return false;

        // Upper keyboard split behavior.
        upperchan = 4;
        uppersolo = 1;
        devices->splitout[uppersolo] = 65;

        if (!expectEqual(devices->shouldRouteLayeredNote(4, 64, 6) ? 1 : 0, 1,
                         "upper split: below split routes non-solo", details))
            return false;

        if (!expectEqual(devices->shouldRouteLayeredNote(4, 64, 1) ? 1 : 0, 0,
                         "upper split: below split blocks solo", details))
            return false;

        if (!expectEqual(devices->shouldRouteLayeredNote(4, 65, 6) ? 1 : 0, 0,
                         "upper split: at split blocks non-solo", details))
            return false;

        return expectEqual(devices->shouldRouteLayeredNote(4, 65, 1) ? 1 : 0, 1,
                           "upper split: at split routes solo", details);
    }

    bool runTransposedNoteBounds(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        devices->clearMidiIOMap();

        // High bound clamp.
        devices->octxpose[1] = 3;
        if (!expectEqual(devices->getTransposedNoteForOutput(120, 1), 127,
                         "transpose upper bound clamps to 127", details))
            return false;

        // Low bound clamp.
        devices->octxpose[2] = -3;
        if (!expectEqual(devices->getTransposedNoteForOutput(5, 2), 0,
                         "transpose lower bound clamps to 0", details))
            return false;

        // Invalid transpose value should be ignored (treated as 0).
        devices->octxpose[3] = 99;
        if (!expectEqual(devices->getTransposedNoteForOutput(64, 3), 64,
                         "invalid transpose value is ignored", details))
            return false;

        // Invalid output channel still returns safely clamped input note.
        return expectEqual(devices->getTransposedNoteForOutput(140, 0), 127,
                           "invalid output channel uses safe clamp", details);
    }

    bool runMidiViewDuplicationGuard(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        devices->clearMidiIOMap();

        // Create a MidiView-like entry with no opened outDevice. This used to
        // risk null dereference in sendToOutputs() when midiviewidx was valid.
        MidiDeviceInfo info;
        info.identifier = "test-midiview";
        info.name = "MidiView";

        auto* midiviewEntry = new MidiDeviceListEntry(info);
        devices->midiOutputs.clear();
        devices->midiOutputs.add(midiviewEntry);
        devices->midiviewidx = 0;

        const auto noteOn = MidiMessage::noteOn(1, 60, (juce::uint8) 100);
        devices->sendToOutputs(noteOn);

        // If we reach here without crashing, the null-guard works.
        return expectEqual(1, 1, "MidiView duplication guard survives null outDevice", details);
    }

    bool runPanelPresetBounds(std::string& details)
    {
        auto* presets = PanelPresets::getInstance();
        if (presets == nullptr)
        {
            details = "PanelPresets::getInstance() returned nullptr";
            return false;
        }

        if (!expectEqual(presets->createPreset(numberpresets - 1, 0, 0, false, 0) ? 1 : 0, 1,
                         "createPreset accepts last valid preset id", details))
            return false;

        return expectEqual(presets->createPreset(numberpresets, 0, 0, false, 0) ? 1 : 0, 0,
                           "createPreset rejects out-of-range preset id", details);
    }

    bool runPanelPresetGroupAndButtonBounds(std::string& details)
    {
        auto* presets = PanelPresets::getInstance();
        if (presets == nullptr)
        {
            details = "PanelPresets::getInstance() returned nullptr";
            return false;
        }

        // Valid baseline should succeed.
        if (!expectEqual(presets->createPreset(0, 0, 0, false, 0) ? 1 : 0, 1,
                         "createPreset accepts valid baseline indices", details))
            return false;

        // Invalid button group id (< 0 and > 11) should fail.
        if (!expectEqual(presets->createPreset(0, -1, 0, false, 0) ? 1 : 0, 0,
                         "createPreset rejects negative button group", details))
            return false;

        if (!expectEqual(presets->createPreset(0, numberbuttongroups, 0, false, 0) ? 1 : 0, 0,
                         "createPreset rejects out-of-range button group", details))
            return false;

        // Invalid panel button index (< 0 and > 95) should fail.
        if (!expectEqual(presets->createPreset(0, 0, -1, false, 0) ? 1 : 0, 0,
                         "createPreset rejects negative panel button index", details))
            return false;

        return expectEqual(presets->createPreset(0, 0, numbervoicebuttons, false, 0) ? 1 : 0, 0,
                           "createPreset rejects out-of-range panel button index", details);
    }

    bool runInstrumentPanelConfigRoundtrip(std::string& details)
    {
        auto* midiInstruments = MidiInstruments::getInstance();
        auto* instrumentPanel = InstrumentPanel::getInstance();

        if (midiInstruments == nullptr || instrumentPanel == nullptr)
        {
            details = "MidiInstruments/InstrumentPanel singleton not available";
            return false;
        }

        const auto snapshot = takeAppStateSnapshot();
        auto& state = getAppState();

        const String testDir = "AMidiOrganTestData";
        const String testInstrumentFile = "integration_test_instruments.json";
        const String testPanelFile = "integration_roundtrip.pnl";

        if (!prepareTestInstrumentJson(testDir, testInstrumentFile, details))
            return false;

        if (!midiInstruments->loadMidiInstruments(state.instrumentfname))
        {
            details = "Failed to load test instrument JSON";
            restoreAppState(snapshot);
            return false;
        }

        state.panelfname = testPanelFile;
        state.configfname = "integration_config_A.cfg";
        state.pnlconfigfname = "";
        state.configreload = false;

        if (!instrumentPanel->initInstrumentPanel(testDir, testPanelFile, false))
        {
            details = "initInstrumentPanel(fromdisk=false) failed";
            restoreAppState(snapshot);
            return false;
        }

        if (!instrumentPanel->loadInstrumentPanel(testDir, testPanelFile, false))
        {
            details = "First loadInstrumentPanel failed";
            restoreAppState(snapshot);
            return false;
        }

        if (!expectEqualStr(state.pnlconfigfname, "integration_config_A.cfg",
                            "panel stores config file name", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        if (!expectEqual(state.configreload ? 1 : 0, 0,
                         "matching config does not require reload", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        state.configfname = "integration_config_B.cfg";
        if (!instrumentPanel->loadInstrumentPanel(testDir, testPanelFile, false))
        {
            details = "Second loadInstrumentPanel failed";
            restoreAppState(snapshot);
            return false;
        }

        const bool reloadExpected = state.configreload;
        restoreAppState(snapshot);
        return expectEqual(reloadExpected ? 1 : 0, 1,
                           "config mismatch sets configreload", details);
    }

    bool runPresetRecallPersistenceAcrossGroups(std::string& details)
    {
        auto* midiInstruments = MidiInstruments::getInstance();
        auto* instrumentPanel = InstrumentPanel::getInstance();
        auto* presets = PanelPresets::getInstance();

        if (midiInstruments == nullptr || instrumentPanel == nullptr || presets == nullptr)
        {
            details = "Required singleton unavailable";
            return false;
        }

        const auto snapshot = takeAppStateSnapshot();
        auto& state = getAppState();

        const String testDir = "AMidiOrganTestData";
        const String testInstrumentFile = "integration_test_instruments.json";
        const String panelFile = "integration_presets.pnl";
        const int presetIdx = 3;

        if (!prepareTestInstrumentJson(testDir, testInstrumentFile, details))
            return false;

        if (!midiInstruments->loadMidiInstruments(state.instrumentfname))
        {
            details = "Failed to load test instrument JSON";
            restoreAppState(snapshot);
            return false;
        }

        state.panelfname = panelFile;
        state.configfname = "preset_test.cfg";

        if (!instrumentPanel->initInstrumentPanel(testDir, panelFile, false))
        {
            details = "initInstrumentPanel for preset persistence failed";
            restoreAppState(snapshot);
            return false;
        }

        // Persist a representative spread of button groups/buttons and flags.
        presets->setPanelButtonIdx(presetIdx, 0, 0);
        presets->setPanelButtonIdx(presetIdx, 5, 40);
        presets->setPanelButtonIdx(presetIdx, 11, 90);
        presets->setMuteStatus(presetIdx, 0, true);
        presets->setMuteStatus(presetIdx, 5, false);
        presets->setMuteStatus(presetIdx, 11, true);
        presets->setRotaryStatus(presetIdx, 0, 2);
        presets->setRotaryStatus(presetIdx, 5, 1);
        presets->setRotaryStatus(presetIdx, 11, 0);

        if (!instrumentPanel->saveInstrumentPanel(testDir, panelFile))
        {
            details = "saveInstrumentPanel for preset persistence failed";
            restoreAppState(snapshot);
            return false;
        }

        // Overwrite with different values to ensure load restores saved data.
        presets->setPanelButtonIdx(presetIdx, 0, 2);
        presets->setPanelButtonIdx(presetIdx, 5, 44);
        presets->setPanelButtonIdx(presetIdx, 11, 84);
        presets->setMuteStatus(presetIdx, 0, false);
        presets->setMuteStatus(presetIdx, 5, true);
        presets->setMuteStatus(presetIdx, 11, false);
        presets->setRotaryStatus(presetIdx, 0, 0);
        presets->setRotaryStatus(presetIdx, 5, 0);
        presets->setRotaryStatus(presetIdx, 11, 0);

        if (!instrumentPanel->loadInstrumentPanel(testDir, panelFile, false))
        {
            details = "loadInstrumentPanel for preset persistence failed";
            restoreAppState(snapshot);
            return false;
        }

        if (!expectEqual(presets->getPanelButtonIdx(presetIdx, 0), 0, "preset button idx group 0", details) ||
            !expectEqual(presets->getPanelButtonIdx(presetIdx, 5), 40, "preset button idx group 5", details) ||
            !expectEqual(presets->getPanelButtonIdx(presetIdx, 11), 90, "preset button idx group 11", details) ||
            !expectEqual(presets->getMuteStatus(presetIdx, 0) ? 1 : 0, 1, "preset mute group 0", details) ||
            !expectEqual(presets->getMuteStatus(presetIdx, 5) ? 1 : 0, 0, "preset mute group 5", details) ||
            !expectEqual(presets->getMuteStatus(presetIdx, 11) ? 1 : 0, 1, "preset mute group 11", details) ||
            !expectEqual(presets->getRotaryStatus(presetIdx, 0), 2, "preset rotary group 0", details) ||
            !expectEqual(presets->getRotaryStatus(presetIdx, 5), 1, "preset rotary group 5", details) ||
            !expectEqual(presets->getRotaryStatus(presetIdx, 11), 0, "preset rotary group 11", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        restoreAppState(snapshot);
        return true;
    }

    bool runPanelSavePairingMismatchGuard(std::string& details)
    {
        auto* instrumentPanel = InstrumentPanel::getInstance();
        if (instrumentPanel == nullptr)
        {
            details = "InstrumentPanel unavailable";
            return false;
        }

        const auto snapshot = takeAppStateSnapshot();
        auto& state = getAppState();

        const String testDir = "AMidiOrganTestData";
        const String panelFile = "pairing_gate_test.pnl";

        state.panelfname = panelFile;
        state.configfname = "pairing_test.cfg";
        state.pnlconfigfname = "pairing_test.cfg";

        if (!prepareTestInstrumentJson(testDir, "integration_test_instruments.json", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        if (!instrumentPanel->initInstrumentPanel(testDir, panelFile, false))
        {
            details = "initInstrumentPanel for pairing gate test failed";
            restoreAppState(snapshot);
            return false;
        }

        state.configPanelPairingMismatchAcknowledged = true;

        if (instrumentPanel->saveInstrumentPanel(testDir, panelFile))
        {
            details = "saveInstrumentPanel should be blocked when pairing mismatch acknowledged";
            state.configPanelPairingMismatchAcknowledged = false;
            restoreAppState(snapshot);
            return false;
        }

        state.configPanelPairingMismatchAcknowledged = false;
        if (!instrumentPanel->saveInstrumentPanel(testDir, panelFile))
        {
            details = "saveInstrumentPanel should succeed after clearing pairing flag";
            restoreAppState(snapshot);
            return false;
        }

        state.configPanelPairingMismatchAcknowledged = true;
        if (!instrumentPanel->saveInstrumentPanel(testDir, panelFile, true))
        {
            details = "saveInstrumentPanel with allowPairingMismatchSave should succeed";
            restoreAppState(snapshot);
            return false;
        }

        restoreAppState(snapshot);
        return true;
    }

    bool runMidiDeviceOpenCloseBounds(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        const auto oldInputs = devices->midiInputs;
        const auto oldOutputs = devices->midiOutputs;
        const int oldMidiViewIdx = devices->midiviewidx;

        devices->midiInputs.clear();
        devices->midiOutputs.clear();
        devices->midiviewidx = 255;

        MidiDeviceInfo inInfo;
        inInfo.identifier = "test-input-missing";
        inInfo.name = "Test Input";
        devices->midiInputs.add(new MidiDeviceListEntry(inInfo));

        MidiDeviceInfo outInfo;
        outInfo.identifier = "test-output-missing";
        outInfo.name = "Test Output";
        devices->midiOutputs.add(new MidiDeviceListEntry(outInfo));

        if (!expectEqual(devices->closeDevice(true, 0) ? 1 : 0, 1, "closeDevice input valid index", details) ||
            !expectEqual(devices->closeDevice(false, 0) ? 1 : 0, 1, "closeDevice output valid index", details))
        {
            devices->midiInputs = oldInputs;
            devices->midiOutputs = oldOutputs;
            devices->midiviewidx = oldMidiViewIdx;
            return false;
        }

        if (!expectEqual(devices->openDevice(true, 999) ? 1 : 0, 0, "openDevice rejects invalid input index", details) ||
            !expectEqual(devices->openDevice(false, 999) ? 1 : 0, 0, "openDevice rejects invalid output index", details) ||
            !expectEqual(devices->closeDevice(true, -1) ? 1 : 0, 0, "closeDevice rejects negative input index", details) ||
            !expectEqual(devices->closeDevice(false, -1) ? 1 : 0, 0, "closeDevice rejects negative output index", details))
        {
            devices->midiInputs = oldInputs;
            devices->midiOutputs = oldOutputs;
            devices->midiviewidx = oldMidiViewIdx;
            return false;
        }

        // Missing test identifiers should fail to open safely without crashing.
        if (!expectEqual(devices->openDevice(true, 0) ? 1 : 0, 0, "openDevice fails safely for missing input identifier", details) ||
            !expectEqual(devices->openDevice(false, 0) ? 1 : 0, 0, "openDevice fails safely for missing output identifier", details))
        {
            devices->midiInputs = oldInputs;
            devices->midiOutputs = oldOutputs;
            devices->midiviewidx = oldMidiViewIdx;
            return false;
        }

        devices->midiInputs = oldInputs;
        devices->midiOutputs = oldOutputs;
        devices->midiviewidx = oldMidiViewIdx;
        return true;
    }

    bool runResetAllControllersEmission(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        std::vector<MidiMessage> sentMessages;
        devices->testSendHook = [&sentMessages](const MidiMessage& message)
        {
            sentMessages.push_back(message);
        };

        devices->resetAllControllers();
        devices->testSendHook = nullptr;

        if (!expectEqual(static_cast<int>(sentMessages.size()), 32, "resetAllControllers emits 32 messages", details))
            return false;

        for (int chan = 1; chan <= 16; ++chan)
        {
            const auto& resetCc = sentMessages[static_cast<size_t>(chan - 1)];
            if (!expectEqual(resetCc.getChannel(), chan, "reset CC channel sequencing", details) ||
                !expectEqual(resetCc.getControllerNumber(), 121, "reset CC controller number", details) ||
                !expectEqual(resetCc.getControllerValue(), 0, "reset CC value", details))
            {
                return false;
            }

            const auto& allNotesOff = sentMessages[static_cast<size_t>(16 + chan - 1)];
            if (!expectEqual(allNotesOff.getChannel(), chan, "all-notes-off channel sequencing", details) ||
                !expectEqual(allNotesOff.getControllerNumber(), 123, "all-notes-off controller number", details) ||
                !expectEqual(allNotesOff.getControllerValue(), 0, "all-notes-off value", details))
            {
                return false;
            }
        }

        return true;
    }

    bool runRewriteSendVelocityAndTranspose(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        devices->clearMidiIOMap();
        std::vector<MidiMessage> sentMessages;
        devices->testSendHook = [&sentMessages](const MidiMessage& message)
        {
            sentMessages.push_back(message);
        };

        devices->octxpose[5] = 1;
        devices->velocityout[5] = false;

        const auto noteOn = MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100));
        if (!expectEqual(devices->rewriteSendNoteMessage(noteOn, 5) ? 1 : 0, 1, "rewriteSendNoteMessage note-on succeeds", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        if (!expectEqual(static_cast<int>(sentMessages.size()), 1, "one note-on message emitted", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        const auto readVelocityByte = [](const MidiMessage& message) -> int
        {
            if (message.getRawDataSize() >= 3)
                return static_cast<int>(message.getRawData()[2]);
            return 0;
        };

        const auto& rewrittenOn = sentMessages[0];
        if (!expectEqual(rewrittenOn.getChannel(), 5, "rewritten note-on channel", details) ||
            !expectEqual(rewrittenOn.getNoteNumber(), 72, "rewritten note-on transpose", details) ||
            !expectEqual(readVelocityByte(rewrittenOn), defvelocityout, "rewritten note-on fixed velocity", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        const auto noteOff = MidiMessage::noteOff(1, 60, static_cast<juce::uint8>(45));
        if (!expectEqual(devices->rewriteSendNoteMessage(noteOff, 5) ? 1 : 0, 1, "rewriteSendNoteMessage note-off succeeds", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        if (!expectEqual(static_cast<int>(sentMessages.size()), 2, "note-off message emitted", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        const auto& rewrittenOff = sentMessages[1];
        if (!expectEqual(rewrittenOff.getChannel(), 5, "rewritten note-off channel", details) ||
            !expectEqual(rewrittenOff.getNoteNumber(), 72, "rewritten note-off transpose", details) ||
            !expectEqual(readVelocityByte(rewrittenOff), 45, "rewritten note-off keeps velocity", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        devices->testSendHook = nullptr;
        return true;
    }

    bool runInstrumentVoiceAndEffectsRoundtrip(std::string& details)
    {
        auto* midiInstruments = MidiInstruments::getInstance();
        auto* instrumentPanel = InstrumentPanel::getInstance();
        if (midiInstruments == nullptr || instrumentPanel == nullptr)
        {
            details = "MidiInstruments/InstrumentPanel singleton not available";
            return false;
        }

        const auto snapshot = takeAppStateSnapshot();
        auto& state = getAppState();

        const String testDir = "AMidiOrganTestData";
        const String testInstrumentFile = "integration_test_instruments.json";
        const String panelFile = "integration_voice_effects.pnl";
        const int panelButtonIdxUnderTest = 22;

        if (!prepareTestInstrumentJson(testDir, testInstrumentFile, details))
            return false;

        if (!midiInstruments->loadMidiInstruments(state.instrumentfname))
        {
            details = "Failed to load test instrument JSON";
            restoreAppState(snapshot);
            return false;
        }

        state.panelfname = panelFile;
        state.configfname = "voice_effects.cfg";

        if (!instrumentPanel->initInstrumentPanel(testDir, panelFile, false))
        {
            details = "initInstrumentPanel for voice/effects roundtrip failed";
            restoreAppState(snapshot);
            return false;
        }

        auto* voiceButton = instrumentPanel->getVoiceButton(panelButtonIdxUnderTest);
        auto* instrument = voiceButton->getInstrumentPtr();

        instrument->setMSB(122);
        instrument->setLSB(33);
        instrument->setFont(87);
        instrument->setVol(111);
        instrument->setExp(95);
        instrument->setRev(44);
        instrument->setCho(31);
        instrument->setMod(27);
        instrument->setTim(61);
        instrument->setAtk(12);
        instrument->setRel(71);
        instrument->setBri(89);
        instrument->setPan(64);

        if (!instrumentPanel->saveInstrumentPanel(testDir, panelFile))
        {
            details = "saveInstrumentPanel for voice/effects roundtrip failed";
            restoreAppState(snapshot);
            return false;
        }

        instrument->setMSB(1);
        instrument->setLSB(2);
        instrument->setFont(3);
        instrument->setVol(4);
        instrument->setExp(5);
        instrument->setRev(6);
        instrument->setCho(7);
        instrument->setMod(8);
        instrument->setTim(9);
        instrument->setAtk(10);
        instrument->setRel(11);
        instrument->setBri(12);
        instrument->setPan(13);

        if (!instrumentPanel->loadInstrumentPanel(testDir, panelFile, false))
        {
            details = "loadInstrumentPanel for voice/effects roundtrip failed";
            restoreAppState(snapshot);
            return false;
        }

        auto* loadedInstrument = instrumentPanel->getVoiceButton(panelButtonIdxUnderTest)->getInstrumentPtr();
        if (!expectEqual(loadedInstrument->getMSB(), 122, "roundtrip MSB", details) ||
            !expectEqual(loadedInstrument->getLSB(), 33, "roundtrip LSB", details) ||
            !expectEqual(loadedInstrument->getFont(), 87, "roundtrip font/program", details) ||
            !expectEqual(loadedInstrument->getVol(), 111, "roundtrip volume", details) ||
            !expectEqual(loadedInstrument->getExp(), 95, "roundtrip expression", details) ||
            !expectEqual(loadedInstrument->getRev(), 44, "roundtrip reverb", details) ||
            !expectEqual(loadedInstrument->getCho(), 31, "roundtrip chorus", details) ||
            !expectEqual(loadedInstrument->getMod(), 27, "roundtrip mod", details) ||
            !expectEqual(loadedInstrument->getTim(), 61, "roundtrip timbre", details) ||
            !expectEqual(loadedInstrument->getAtk(), 12, "roundtrip attack", details) ||
            !expectEqual(loadedInstrument->getRel(), 71, "roundtrip release", details) ||
            !expectEqual(loadedInstrument->getBri(), 89, "roundtrip brightness", details) ||
            !expectEqual(loadedInstrument->getPan(), 64, "roundtrip pan", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        restoreAppState(snapshot);
        return true;
    }

    bool runMidiRoutingSplitLayerEdgeCases(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        devices->clearMidiIOMap();

        // Configure lower manual split with a layered output and solo output.
        lowerchan = 3;
        lowersolo = 15;
        devices->splitout[lowersolo] = 60;
        devices->iomap[3][0] = 3;
        devices->iomap[3][1] = 4;
        devices->iomap[3][2] = 15;
        devices->octxpose[4] = 1;
        devices->octxpose[15] = -1;
        devices->velocityout[4] = true;
        devices->velocityout[15] = false;

        // Below split: layer output routes, solo output blocked.
        if (!expectEqual(devices->shouldRouteLayeredNote(3, 59, 4) ? 1 : 0, 1,
                         "below split routes non-solo layer", details) ||
            !expectEqual(devices->shouldRouteLayeredNote(3, 59, 15) ? 1 : 0, 0,
                         "below split blocks solo output", details))
            return false;

        // At split: layer output blocked, solo output routes.
        if (!expectEqual(devices->shouldRouteLayeredNote(3, 60, 4) ? 1 : 0, 0,
                         "at split blocks non-solo layer", details) ||
            !expectEqual(devices->shouldRouteLayeredNote(3, 60, 15) ? 1 : 0, 1,
                         "at split routes solo output", details))
            return false;

        // Verify transpose edge behavior for the layered channels used above.
        if (!expectEqual(devices->getTransposedNoteForOutput(59, 4), 71,
                         "layered non-solo transpose +1 octave", details) ||
            !expectEqual(devices->getTransposedNoteForOutput(60, 15), 48,
                         "solo transpose -1 octave", details))
            return false;

        // End-to-end rewrite/send should safely process both target channels.
        const auto noteOn = MidiMessage::noteOn(3, 60, (juce::uint8) 100);
        if (!expectEqual(devices->rewriteSendNoteMessage(noteOn, 4) ? 1 : 0, 1,
                         "rewrite/send accepts layered non-solo output", details))
            return false;

        return expectEqual(devices->rewriteSendNoteMessage(noteOn, 15) ? 1 : 0, 1,
                           "rewrite/send accepts solo output", details);
    }

}

int main()
{
    std::vector<TestResult> results;

    {
        std::string details;
        results.push_back({ "check0to127 clamps values", runCheck0to127(details), details });
    }

    {
        std::string details;
        results.push_back({ "check1to16 clamps values", runCheck1to16(details), details });
    }

    {
        std::string details;
        results.push_back({ "Volume helper math and clamping", runVolumeMathHelpers(details), details });
    }

    {
        std::string details;
        results.push_back({ "moduleIdx baseline differs detects per-group module changes", runModuleIdxBaselineDiffers(details), details });
    }

    {
        std::string details;
        results.push_back({ "Panel scan counts configfilename references under AMidiOrgan tree", runPanelConfigReferencingScan(details), details });
    }

    {
        std::string details;
        results.push_back({ "readEmbeddedConfigFilenameFromPanelFile reads root configfilename", runReadEmbeddedConfigFilenameFromPanelFile(details), details });
    }

    {
        std::string details;
        results.push_back({ "AppState aliases remain compatible", runAppStateAliasBackcompat(details), details });
    }

    {
        std::string details;
        results.push_back({ "InstrumentModules writes AppState", runInstrumentModuleWritesAppState(details), details });
    }

    {
        std::string details;
        results.push_back({ "lookupPanelGroup maps ranges", runLookupPanelGroup(details), details });
    }

    {
        std::string details;
        results.push_back({ "Instrument setters clamp and map", runInstrumentClampBehavior(details), details });
    }

    {
        std::string details;
        results.push_back({ "MidiDevices default map initialization", runMidiDevicesDefaultMap(details), details });
    }

    {
        std::string details;
        results.push_back({ "MidiDevices note rewrite basic behavior", runRewriteSendNoteMessageBasics(details), details });
    }

    {
        std::string details;
        results.push_back({ "MidiDevices split routing behavior", runSplitRoutingBehavior(details), details });
    }

    {
        std::string details;
        results.push_back({ "MidiDevices transpose note bounds", runTransposedNoteBounds(details), details });
    }

    {
        std::string details;
        results.push_back({ "MidiDevices MidiView duplication guard", runMidiViewDuplicationGuard(details), details });
    }

    {
        std::string details;
        results.push_back({ "PanelPresets validates preset id bounds", runPanelPresetBounds(details), details });
    }

    {
        std::string details;
        results.push_back({ "PanelPresets validates group/button bounds", runPanelPresetGroupAndButtonBounds(details), details });
    }

    {
        std::string details;
        results.push_back({ "InstrumentPanel panel/config roundtrip load-save-load", runInstrumentPanelConfigRoundtrip(details), details });
    }

    {
        std::string details;
        results.push_back({ "Preset recall persists across groups/buttons", runPresetRecallPersistenceAcrossGroups(details), details });
    }

    {
        std::string details;
        results.push_back({ "Panel save blocked when pairing mismatch acknowledged", runPanelSavePairingMismatchGuard(details), details });
    }

    {
        std::string details;
        results.push_back({ "MidiDevices open/close index guards and lifecycle safety", runMidiDeviceOpenCloseBounds(details), details });
    }

    {
        std::string details;
        results.push_back({ "MidiDevices reset sends full channel controller sweep", runResetAllControllersEmission(details), details });
    }

    {
        std::string details;
        results.push_back({ "MidiDevices rewrite applies velocity policy and transpose", runRewriteSendVelocityAndTranspose(details), details });
    }

    {
        std::string details;
        results.push_back({ "InstrumentPanel roundtrip preserves MSB/LSB/program/effects", runInstrumentVoiceAndEffectsRoundtrip(details), details });
    }

    {
        std::string details;
        results.push_back({ "MIDI routing split/layer edge cases", runMidiRoutingSplitLayerEdgeCases(details), details });
    }


    int failedCount = 0;
    for (const auto& r : results)
    {
        if (r.passed)
        {
            std::cout << "[PASS] " << r.name << "\n";
        }
        else
        {
            ++failedCount;
            std::cout << "[FAIL] " << r.name << " :: " << r.details << "\n";
        }
    }

    std::cout << "\nTotal: " << results.size()
              << ", Passed: " << (results.size() - static_cast<size_t>(failedCount))
              << ", Failed: " << failedCount << "\n";

    // Avoid shutdown-time singleton teardown crashes in this header-heavy app.
    // Preserve test result status while exiting immediately.
    std::cout.flush();
    std::_Exit(failedCount == 0 ? 0 : 1);
}
