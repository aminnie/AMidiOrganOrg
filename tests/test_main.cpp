#include <JuceHeader.h>

using namespace juce;

#include "../AMidiUtils.h"
#include "../AMidiDevices.h"
#include "../AMidiInstruments.h"
#include "../AMidiControl.h"
#include "../midi_file_player/MidiFilePlaybackEngine.h"
#include "../midi_file_player/PlayerMidiIdentity.h"
#include "../midi_file_player/PlayerSongProfile.h"
#include "../midi_file_player/PlayerSongProfileStore.h"
#include "../midi_file_player/PlayerStripCcMerge.h"

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

    bool expectTrue(bool value, const std::string& label, std::string& details)
    {
        if (!value)
        {
            details = label + " expected true but was false";
            return false;
        }
        return true;
    }

    struct AppStateSnapshot
    {
        String instrumentdir;
        String paneldir;
        String instrumentfname;
        String panelfname;
        String configfname;
        String pnlconfigfname;
        int defaultEffectsVol = 100;
        int defaultEffectsBri = 30;
        int defaultEffectsExp = 127;
        int defaultEffectsRev = 20;
        int defaultEffectsCho = 10;
        int defaultEffectsMod = 0;
        int defaultEffectsTim = 0;
        int defaultEffectsAtk = 0;
        int defaultEffectsRel = 0;
        int defaultEffectsPan = 64;
        int presetMidiPcInputChannel = 16;
        int presetMidiPcValue = 0;
        String uiProfileId = "1480x320";
        bool configreload = false;
        bool configPanelPairingMismatchAcknowledged = false;
        bool upperManualRotaryFast = true;
        bool upperManualRotaryBrake = false;
        int upperManualRotaryTargetGroup = 1;
        bool lowerManualRotaryFast = true;
        bool lowerManualRotaryBrake = false;
        int lowerManualRotaryTargetGroup = 1;
    };

    AppStateSnapshot takeAppStateSnapshot()
    {
        auto& state = getAppState();
        AppStateSnapshot snapshot;
        snapshot.instrumentdir = state.instrumentdir;
        snapshot.paneldir = state.paneldir;
        snapshot.instrumentfname = state.instrumentfname;
        snapshot.panelfname = state.panelfname;
        snapshot.configfname = state.configfname;
        snapshot.pnlconfigfname = state.pnlconfigfname;
        snapshot.defaultEffectsVol = state.defaultEffectsVol;
        snapshot.defaultEffectsBri = state.defaultEffectsBri;
        snapshot.defaultEffectsExp = state.defaultEffectsExp;
        snapshot.defaultEffectsRev = state.defaultEffectsRev;
        snapshot.defaultEffectsCho = state.defaultEffectsCho;
        snapshot.defaultEffectsMod = state.defaultEffectsMod;
        snapshot.defaultEffectsTim = state.defaultEffectsTim;
        snapshot.defaultEffectsAtk = state.defaultEffectsAtk;
        snapshot.defaultEffectsRel = state.defaultEffectsRel;
        snapshot.defaultEffectsPan = state.defaultEffectsPan;
        snapshot.presetMidiPcInputChannel = state.presetMidiPcInputChannel;
        snapshot.presetMidiPcValue = state.presetMidiPcValue;
        snapshot.uiProfileId = state.uiProfileId;
        snapshot.configreload = state.configreload;
        snapshot.configPanelPairingMismatchAcknowledged = state.configPanelPairingMismatchAcknowledged;
        snapshot.upperManualRotaryFast = state.upperManualRotaryFast;
        snapshot.upperManualRotaryBrake = state.upperManualRotaryBrake;
        snapshot.upperManualRotaryTargetGroup = state.upperManualRotaryTargetGroup;
        snapshot.lowerManualRotaryFast = state.lowerManualRotaryFast;
        snapshot.lowerManualRotaryBrake = state.lowerManualRotaryBrake;
        snapshot.lowerManualRotaryTargetGroup = state.lowerManualRotaryTargetGroup;
        return snapshot;
    }

    void restoreAppState(const AppStateSnapshot& snapshot)
    {
        auto& state = getAppState();
        state.instrumentdir = snapshot.instrumentdir;
        state.paneldir = snapshot.paneldir;
        state.instrumentfname = snapshot.instrumentfname;
        state.panelfname = snapshot.panelfname;
        state.configfname = snapshot.configfname;
        state.pnlconfigfname = snapshot.pnlconfigfname;
        state.defaultEffectsVol = snapshot.defaultEffectsVol;
        state.defaultEffectsBri = snapshot.defaultEffectsBri;
        state.defaultEffectsExp = snapshot.defaultEffectsExp;
        state.defaultEffectsRev = snapshot.defaultEffectsRev;
        state.defaultEffectsCho = snapshot.defaultEffectsCho;
        state.defaultEffectsMod = snapshot.defaultEffectsMod;
        state.defaultEffectsTim = snapshot.defaultEffectsTim;
        state.defaultEffectsAtk = snapshot.defaultEffectsAtk;
        state.defaultEffectsRel = snapshot.defaultEffectsRel;
        state.defaultEffectsPan = snapshot.defaultEffectsPan;
        state.presetMidiPcInputChannel = snapshot.presetMidiPcInputChannel;
        state.presetMidiPcValue = snapshot.presetMidiPcValue;
        state.uiProfileId = snapshot.uiProfileId;
        state.configreload = snapshot.configreload;
        state.configPanelPairingMismatchAcknowledged = snapshot.configPanelPairingMismatchAcknowledged;
        state.upperManualRotaryFast = snapshot.upperManualRotaryFast;
        state.upperManualRotaryBrake = snapshot.upperManualRotaryBrake;
        state.upperManualRotaryTargetGroup = snapshot.upperManualRotaryTargetGroup;
        state.lowerManualRotaryFast = snapshot.lowerManualRotaryFast;
        state.lowerManualRotaryBrake = snapshot.lowerManualRotaryBrake;
        state.lowerManualRotaryTargetGroup = snapshot.lowerManualRotaryTargetGroup;
    }

    bool runManualRotaryTargetDefaults(std::string& details)
    {
        auto& state = getAppState();
        if (!expectEqual(state.upperManualRotaryTargetGroup, 1, "upper rotary target defaults to group 1", details))
            return false;
        return expectEqual(state.lowerManualRotaryTargetGroup, 1, "lower rotary target defaults to group 1", details);
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

    /** Valid catalog with one category and one voice (regression: voice index 0 must not crash). */
    bool prepareTestInstrumentJsonOneVoice(const String& testDirName, const String& fileName, std::string& details)
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
    ["Cat1", [3, 0, 121, 8, "VoiceOne"]]
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

    bool runMidiInstrumentsVoiceLookupBounds(std::string& details)
    {
        auto* midiInstruments = MidiInstruments::getInstance();
        if (midiInstruments == nullptr)
        {
            details = "MidiInstruments::getInstance() returned nullptr";
            return false;
        }

        const auto snapshot = takeAppStateSnapshot();
        auto& state = getAppState();

        const String testDir = "AMidiOrganTestData";
        const String testFile = "midi_voice_bounds_test.json";

        if (!prepareTestInstrumentJsonOneVoice(testDir, testFile, details))
        {
            restoreAppState(snapshot);
            return false;
        }

        if (!midiInstruments->loadMidiInstruments(state.instrumentfname))
        {
            details = "Failed to load one-voice test instrument JSON";
            restoreAppState(snapshot);
            return false;
        }

        if (!expectEqual(midiInstruments->getCategoryCount(), 1, "getCategoryCount", details) ||
            !expectEqual(midiInstruments->getCategoryVoiceCount(0), 1, "getCategoryVoiceCount(0)", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        // Index 0 within a category is the category title, not a voice row — must not crash.
        if (!expectEqualStr(midiInstruments->getVoice(0, 0), "No Voice",
                            "getVoice(0,0) category slot", details) ||
            !expectEqualStr(midiInstruments->getVoice(0, 1), "VoiceOne",
                            "getVoice(0,1) first voice", details) ||
            !expectEqualStr(midiInstruments->getVoice(0, 2), "No Voice",
                            "getVoice(0,2) out of range", details) ||
            !expectEqualStr(midiInstruments->getVoice(50, 1), "No Voice",
                            "getVoice bad category", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        if (!expectEqual(midiInstruments->getMSB(0, 0), 0, "getMSB(0,0)", details) ||
            !expectEqual(midiInstruments->getMSB(0, 1), 121, "getMSB(0,1)", details) ||
            !expectEqual(midiInstruments->getLSB(0, 0), 0, "getLSB(0,0)", details) ||
            !expectEqual(midiInstruments->getLSB(0, 1), 8, "getLSB(0,1)", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        restoreAppState(snapshot);
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

    bool runConfiguredLabelColourHelpers(std::string& details)
    {
        const auto configured = getConfiguredLabelTextColour(true);
        const auto unconfigured = getConfiguredLabelTextColour(false);
        if (!(configured == juce::Colours::black))
        {
            details = "configured label color should be black";
            return false;
        }
        if (!(unconfigured == juce::Colour(0xff888888)))
        {
            details = "unconfigured label color should be mid-grey";
            return false;
        }
        return true;
    }

    bool runPresetDisplayBankHelpers(std::string& details)
    {
        if (!expectEqual(getPresetDisplayBankForRealPresetIdx(1), 0, "preset 1 in bank 0", details) ||
            !expectEqual(getPresetDisplayBankForRealPresetIdx(6), 0, "preset 6 in bank 0", details) ||
            !expectEqual(getPresetDisplayBankForRealPresetIdx(7), 1, "preset 7 in bank 1", details) ||
            !expectEqual(getPresetDisplayBankForRealPresetIdx(12), 1, "preset 12 in bank 1", details))
        {
            return false;
        }

        if (!expectEqual(getBankStartPresetIdxForDisplayBank(0), 1, "bank 0 starts at preset 1", details) ||
            !expectEqual(getBankStartPresetIdxForDisplayBank(1), 7, "bank 1 starts at preset 7", details))
        {
            return false;
        }

        if (!expectEqual(getDisplaySlotForRealPresetIdx(1, 0), 0, "preset 1 slot in bank 0", details) ||
            !expectEqual(getDisplaySlotForRealPresetIdx(6, 0), 5, "preset 6 slot in bank 0", details) ||
            !expectEqual(getDisplaySlotForRealPresetIdx(7, 1), 0, "preset 7 slot in bank 1", details) ||
            !expectEqual(getDisplaySlotForRealPresetIdx(12, 1), 5, "preset 12 slot in bank 1", details) ||
            !expectEqual(getDisplaySlotForRealPresetIdx(7, 0), -1, "preset 7 not visible in bank 0", details) ||
            !expectEqual(getDisplaySlotForRealPresetIdx(2, 1), -1, "preset 2 not visible in bank 1", details))
        {
            return false;
        }

        return true;
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

    bool runPlayerStripCcMergeHelpers(std::string& details)
    {
        if (!expectEqual(PlayerStripCcMerge::mergeUnipolarCc7Bit(100, 127), 100,
                         "mergeUnipolarCc7Bit keeps file value when strip is 127", details)
            || !expectEqual(PlayerStripCcMerge::mergeUnipolarCc7Bit(100, 64), 50,
                            "mergeUnipolarCc7Bit scales by strip trim", details)
            || !expectEqual(PlayerStripCcMerge::mergeUnipolarCc7Bit(127, 127), 127,
                            "mergeUnipolarCc7Bit clamps at upper bound", details)
            || !expectEqual(PlayerStripCcMerge::mergeUnipolarCc7Bit(-3, 90), 0,
                            "mergeUnipolarCc7Bit clamps negative source values", details))
        {
            return false;
        }

        if (PlayerStripCcMerge::shouldMergeControllerNumber(CCPan))
        {
            details = "CCPan should be passthrough and not whitelisted for scaling";
            return false;
        }

        Instrument strip;
        strip.setVol(64);
        juce::MidiMessage mergedMessage;
        const auto merged = PlayerStripCcMerge::mergeControllerWithStripIfApplicable(
            juce::MidiMessage::controllerEvent(1, CCVol, 100), strip, mergedMessage);
        if (!merged)
        {
            details = "Expected CCVol merge to apply for whitelisted controller";
            return false;
        }

        if (!expectEqual(mergedMessage.getControllerValue(), 50,
                         "Merged CCVol value should be scaled by strip volume", details))
        {
            return false;
        }

        const auto panMerged = PlayerStripCcMerge::mergeControllerWithStripIfApplicable(
            juce::MidiMessage::controllerEvent(1, CCPan, 80), strip, mergedMessage);
        if (panMerged)
        {
            details = "CCPan should not be transformed by player-strip CC merge";
            return false;
        }

        return true;
    }

    bool runPlayerSongProfileCodecRoundtrip(std::string& details)
    {
        PlayerSongProfile profile;
        profile.profileId = "profile-1";
        profile.displayName = "Song A - Integra";
        profile.createdUtc = "2026-04-18T10:00:00Z";
        profile.updatedUtc = "2026-04-18T10:10:00Z";
        profile.midiIdentity.midiKey = "songA|123|456";
        profile.midiIdentity.fileName = "songA.mid";
        profile.midiIdentity.originalPath = "C:/temp/songA.mid";
        profile.moduleIdx = 2;
        profile.moduleDisplayName = "Integra-7";
        profile.enableProgramChangeRemap = true;
        profile.enablePlayerStripCcScaling = true;
        profile.soloChannel = 3;
        profile.mutedChannels[(size_t) 5] = true;
        profile.channels[0].midiChannel = 1;
        profile.channels[0].configured = true;
        profile.channels[0].voice = "Organ 1";
        profile.channels[0].program = 42;
        profile.channels[0].vol = 96;
        profile.channels[0].effectsDirty = MAPVOL | MAPMOD;
        profile.selectedChannelIdx = 0;

        const auto json = PlayerSongProfileCodec::toJsonString(profile);
        PlayerSongProfile decoded;
        juce::String error;
        if (!PlayerSongProfileCodec::fromJsonString(json, decoded, error))
        {
            details = "fromJsonString failed: " + error.toStdString();
            return false;
        }

        if (!expectEqualStr(decoded.profileId, profile.profileId, "profileId roundtrip", details)
            || !expectEqualStr(decoded.displayName, profile.displayName, "displayName roundtrip", details)
            || !expectEqual(decoded.moduleIdx, profile.moduleIdx, "moduleIdx roundtrip", details)
            || !expectEqual(decoded.soloChannel, profile.soloChannel, "soloChannel roundtrip", details)
            || !expectEqual(decoded.channels[0].program, profile.channels[0].program, "program roundtrip", details)
            || !expectEqual(decoded.channels[0].vol, profile.channels[0].vol, "vol roundtrip", details)
            || !expectEqual(decoded.channels[0].effectsDirty, profile.channels[0].effectsDirty, "effectsDirty roundtrip", details))
        {
            return false;
        }

        if (!expectTrue(decoded.mutedChannels[(size_t) 5], "muted channel 5 roundtrip", details))
            return false;

        return true;
    }

    bool runPlayerSongProfilesIndexRoundtrip(std::string& details)
    {
        PlayerSongProfilesIndex index;
        index.schemaVersion = 1;
        PlayerSongProfileIndexEntry entry;
        entry.profileId = "p1";
        entry.displayName = "Song A - Deebach";
        entry.midiKey = "songA|1|2";
        entry.moduleDisplayName = "Deebach";
        entry.profileFile = "p1.playerprofile.json";
        entry.updatedUtc = "2026-04-18T11:00:00Z";
        index.entries.push_back(entry);
        index.lastUsedByMidiKey[entry.midiKey] = entry.profileId;

        const auto encoded = PlayerSongProfileStore::toVar(index);
        const auto decoded = PlayerSongProfileStore::fromVar(encoded);
        if (!expectEqual((int) decoded.entries.size(), 1, "decoded index entries size", details))
            return false;
        if (!expectEqualStr(decoded.entries[0].profileId, "p1", "decoded entry profileId", details))
            return false;
        if (!expectEqualStr(PlayerSongProfileStore::getLastUsedProfileIdForMidiKey(decoded, entry.midiKey),
                            "p1",
                            "lastUsedByMidiKey lookup",
                            details))
        {
            return false;
        }

        const auto filtered = PlayerSongProfileStore::getProfilesForMidiKey(decoded, entry.midiKey);
        return expectEqual((int) filtered.size(), 1, "filtered profiles for midi key", details);
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

    bool runModuleMatchAliasBehavior(std::string& details)
    {
        StringArray aliases;
        aliases.add("EVM");
        aliases.add("MIDI GADGET");

        const StringArray normalizedAliases = normalizeModuleMatchStrings(aliases, "KETRON");
        if (!expectEqual(normalizedAliases.size(), 3, "normalizeModuleMatchStrings keeps aliases + legacy fallback", details))
            return false;

        if (!expectEqual(doesAnyModuleMatcherMatchDeviceName(normalizedAliases, "MIDI Gadget") ? 1 : 0, 1,
                         "alias matcher recognizes MIDI driver name", details))
            return false;

        if (!expectEqual(doesAnyModuleMatcherMatchDeviceName(normalizedAliases, "Ketron EVM MIDI 1") ? 1 : 0, 1,
                         "alias matcher recognizes existing EVM naming", details))
            return false;

        StringArray genericMidiMatcher;
        genericMidiMatcher.add("MIDI");
        if (!expectEqual(doesAnyModuleMatcherMatchDeviceName(genericMidiMatcher, "Integra-7 MIDI 1") ? 1 : 0, 0,
                         "generic MIDI matcher avoids broad substring matches", details))
            return false;

        const StringArray legacyOnly = normalizeModuleMatchStrings({}, "Blackbox");
        return expectEqual(doesAnyModuleMatcherMatchDeviceName(legacyOnly, "Deebach Blackbox Port A") ? 1 : 0, 1,
                           "legacy moduleIdString remains supported", details);
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
            for (int j = 0; j < 5; ++j)
            {
                if (!expectEqual(devices->iomap[i][j], 0, "iomap[i][j] cleared", details))
                    return false;
            }

            if (!expectEqual(devices->octxpose[i], 0, "octxpose default", details))
                return false;

            if (!expectEqual(devices->splitout[i], 0, "splitout default", details))
                return false;

            if (!expectEqual(devices->moduleout[i].size(), 0, "moduleout default empty", details))
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

    bool runModuleFanoutByOutputChannel(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        devices->clearMidiIOMap();

        MidiDeviceInfo outA;
        outA.identifier = "fanout-out-1";
        outA.name = "FanoutOut1";

        MidiDeviceInfo outB;
        outB.identifier = "fanout-out-2";
        outB.name = "FanoutOut2";

        devices->midiOutputs.clear();
        devices->midiOutputs.add(new MidiDeviceListEntry(outA));
        devices->midiOutputs.add(new MidiDeviceListEntry(outB));

        devices->moduleout[1].clearQuick();
        devices->moduleout[1].add(0);
        devices->moduleout[1].add(1);

        int monitorHits = 0;
        devices->setOutgoingMidiMonitor([&monitorHits](const MidiMessage&, const juce::String&)
            {
                ++monitorHits;
            });

        const auto noteOn = MidiMessage::noteOn(1, 60, (juce::uint8)100);
        devices->sendToOutputs(noteOn);

        devices->setOutgoingMidiMonitor({});

        return expectEqual(monitorHits, 2, "channel fanout emits one routed send per mapped module", details);
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
        state.upperManualRotaryTargetGroup = 2;
        state.lowerManualRotaryTargetGroup = 1;

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

        if (!expectEqual(state.upperManualRotaryTargetGroup, 2,
                         "panel restores upper rotary target group", details) ||
            !expectEqual(state.lowerManualRotaryTargetGroup, 1,
                         "panel restores lower rotary target group", details))
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
        const int presetIdx = 10;

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
        presets->setMasterVolStep(presetIdx, 0, 2);
        presets->setMasterVolStep(presetIdx, 5, 7);
        presets->setMasterVolStep(presetIdx, 11, 10);

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
        presets->setMasterVolStep(presetIdx, 0, 9);
        presets->setMasterVolStep(presetIdx, 5, 1);
        presets->setMasterVolStep(presetIdx, 11, 0);

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
            !expectEqual(presets->getRotaryStatus(presetIdx, 11), 0, "preset rotary group 11", details) ||
            !expectEqual(presets->getMasterVolStep(presetIdx, 0), 2, "preset volume step group 0", details) ||
            !expectEqual(presets->getMasterVolStep(presetIdx, 5), 7, "preset volume step group 5", details) ||
            !expectEqual(presets->getMasterVolStep(presetIdx, 11), 10, "preset volume step group 11", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        restoreAppState(snapshot);
        return true;
    }

    bool runPresetNextCycleBoundaries(std::string& details)
    {
        auto nextPresetIdx = [](int activePresetIdx, int displayBank)
        {
            if (activePresetIdx == 0)
                return displayBank == 1 ? 7 : 1;
            return activePresetIdx >= (numberpresets - 1) ? 1 : (activePresetIdx + 1);
        };

        if (!expectEqual(nextPresetIdx(6, 0), 7, "next from preset 6 enters preset 7 bank", details))
            return false;
        if (!expectEqual(nextPresetIdx(12, 1), 1, "next from preset 12 wraps to preset 1", details))
            return false;
        if (!expectEqual(nextPresetIdx(0, 0), 1, "manual next in bank 1 goes to preset 1", details))
            return false;

        return expectEqual(nextPresetIdx(0, 1), 7, "manual next in bank 2 goes to preset 7", details);
    }

    bool runLegacyPresetLoadCompatibility(std::string& details)
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
        const String fullPanelFile = "integration_presets_full.pnl";
        const String legacyPanelFile = "integration_presets_legacy.pnl";

        if (!prepareTestInstrumentJson(testDir, testInstrumentFile, details))
            return false;

        if (!midiInstruments->loadMidiInstruments(state.instrumentfname))
        {
            details = "Failed to load test instrument JSON";
            restoreAppState(snapshot);
            return false;
        }

        state.panelfname = fullPanelFile;
        state.configfname = "legacy_preset_test.cfg";

        if (!instrumentPanel->initInstrumentPanel(testDir, fullPanelFile, false))
        {
            details = "initInstrumentPanel for legacy preset test failed";
            restoreAppState(snapshot);
            return false;
        }

        // Seed one low preset and one high preset to validate legacy truncation behavior.
        presets->setPanelButtonIdx(3, 0, 40);
        presets->setMuteStatus(3, 0, true);
        presets->setRotaryStatus(3, 0, 2);
        presets->setMasterVolStep(3, 0, 6);
        presets->setPanelButtonIdx(10, 0, 84);
        presets->setMuteStatus(10, 0, true);
        presets->setRotaryStatus(10, 0, 1);
        presets->setMasterVolStep(10, 0, 9);

        if (!instrumentPanel->saveInstrumentPanel(testDir, fullPanelFile))
        {
            details = "saveInstrumentPanel for full preset payload failed";
            restoreAppState(snapshot);
            return false;
        }

        const File baseDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(state.paneldir);
        const File fullPanelPath = baseDir.getChildFile(fullPanelFile);
        const File legacyPanelPath = baseDir.getChildFile(legacyPanelFile);

        FileInputStream in(fullPanelPath);
        if (!in.openedOk())
        {
            details = "Failed to open full panel file for legacy conversion";
            restoreAppState(snapshot);
            return false;
        }

        ValueTree tree = ValueTree::readFromStream(in);
        if (!tree.isValid())
        {
            details = "Failed to parse full panel file ValueTree";
            restoreAppState(snapshot);
            return false;
        }

        ValueTree presetsNode = tree.getChild(numbervoicebuttons);
        if (!presetsNode.isValid())
        {
            details = "ValueTree missing Presets child";
            restoreAppState(snapshot);
            return false;
        }

        while (presetsNode.getNumChildren() > 7)
            presetsNode.removeChild(presetsNode.getNumChildren() - 1, nullptr);

        // Simulate pre-volume legacy payload: no mastervolstep on any preset button group nodes.
        for (int p = 0; p < presetsNode.getNumChildren(); ++p)
        {
            ValueTree presetNode = presetsNode.getChild(p);
            for (int bg = 0; bg < presetNode.getNumChildren(); ++bg)
            {
                ValueTree groupNode = presetNode.getChild(bg);
                groupNode.removeProperty("mastervolstep", nullptr);
            }
        }

        {
            FileOutputStream out(legacyPanelPath);
            if (!out.openedOk())
            {
                details = "Failed to create legacy panel file";
                restoreAppState(snapshot);
                return false;
            }
            tree.writeToStream(out);
        }

        // Poison preset 10 in-memory; legacy load should reset missing high presets to defaults.
        presets->setPanelButtonIdx(10, 0, 90);
        presets->setMuteStatus(10, 0, false);
        presets->setRotaryStatus(10, 0, 2);
        presets->setMasterVolStep(10, 0, 1);

        if (!instrumentPanel->loadInstrumentPanel(testDir, legacyPanelFile, false))
        {
            details = "loadInstrumentPanel failed for legacy preset payload";
            restoreAppState(snapshot);
            return false;
        }

        if (!expectEqual(presets->getPanelButtonIdx(3, 0), 40, "legacy load keeps preset 3 payload", details) ||
            !expectEqual(presets->getMuteStatus(3, 0) ? 1 : 0, 1, "legacy load keeps preset 3 mute", details) ||
            !expectEqual(presets->getRotaryStatus(3, 0), 2, "legacy load keeps preset 3 rotary", details) ||
            !expectEqual(presets->getMasterVolStep(3, 0), 8, "legacy load defaults preset 3 volume step", details) ||
            !expectEqual(presets->getPanelButtonIdx(10, 0), 0, "legacy load resets missing preset 10 button", details) ||
            !expectEqual(presets->getMuteStatus(10, 0) ? 1 : 0, 0, "legacy load resets missing preset 10 mute", details) ||
            !expectEqual(presets->getRotaryStatus(10, 0), 0, "legacy load resets missing preset 10 rotary", details) ||
            !expectEqual(presets->getMasterVolStep(10, 0), 8, "legacy load resets missing preset 10 volume step", details))
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

    bool runVoiceSoundConfiguredPersistenceRoundtrip(std::string& details)
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
        const String panelFile = "integration_sound_configured_roundtrip.pnl";

        if (!prepareTestInstrumentJson(testDir, testInstrumentFile, details))
            return false;

        if (!midiInstruments->loadMidiInstruments(state.instrumentfname))
        {
            details = "Failed to load test instrument JSON";
            restoreAppState(snapshot);
            return false;
        }

        state.panelfname = panelFile;
        state.configfname = "sound_configured_test.cfg";

        if (!instrumentPanel->initInstrumentPanel(testDir, panelFile, false))
        {
            details = "initInstrumentPanel for soundConfigured test failed";
            restoreAppState(snapshot);
            return false;
        }

        auto* vb0 = instrumentPanel->getVoiceButton(0);
        auto* vb1 = instrumentPanel->getVoiceButton(1);
        if (vb0 == nullptr || vb1 == nullptr)
        {
            details = "Failed to get test voice buttons";
            restoreAppState(snapshot);
            return false;
        }

        vb0->setSoundConfigured(true);
        vb1->setSoundConfigured(false);

        if (!instrumentPanel->saveInstrumentPanel(testDir, panelFile))
        {
            details = "saveInstrumentPanel for soundConfigured roundtrip failed";
            restoreAppState(snapshot);
            return false;
        }

        // Poison state so load must restore persisted values.
        vb0->setSoundConfigured(false);
        vb1->setSoundConfigured(true);

        if (!instrumentPanel->loadInstrumentPanel(testDir, panelFile, false))
        {
            details = "loadInstrumentPanel for soundConfigured roundtrip failed";
            restoreAppState(snapshot);
            return false;
        }

        if (!expectEqual(vb0->isSoundConfigured() ? 1 : 0, 1, "soundConfigured persisted true", details) ||
            !expectEqual(vb1->isSoundConfigured() ? 1 : 0, 0, "soundConfigured persisted false", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        restoreAppState(snapshot);
        return true;
    }

    bool runVoiceSoundConfiguredLegacyDefaultFalse(std::string& details)
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
        const String fullPanelFile = "integration_sound_configured_full.pnl";
        const String legacyPanelFile = "integration_sound_configured_legacy.pnl";

        if (!prepareTestInstrumentJson(testDir, testInstrumentFile, details))
            return false;

        if (!midiInstruments->loadMidiInstruments(state.instrumentfname))
        {
            details = "Failed to load test instrument JSON";
            restoreAppState(snapshot);
            return false;
        }

        state.panelfname = fullPanelFile;
        state.configfname = "sound_configured_legacy.cfg";

        if (!instrumentPanel->initInstrumentPanel(testDir, fullPanelFile, false))
        {
            details = "initInstrumentPanel for soundConfigured legacy test failed";
            restoreAppState(snapshot);
            return false;
        }

        auto* vb0 = instrumentPanel->getVoiceButton(0);
        auto* vb10 = instrumentPanel->getVoiceButton(10);
        if (vb0 == nullptr || vb10 == nullptr)
        {
            details = "Failed to get test voice buttons";
            restoreAppState(snapshot);
            return false;
        }

        vb0->setSoundConfigured(true);
        vb10->setSoundConfigured(true);

        if (!instrumentPanel->saveInstrumentPanel(testDir, fullPanelFile))
        {
            details = "saveInstrumentPanel for soundConfigured legacy test failed";
            restoreAppState(snapshot);
            return false;
        }

        const File baseDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(state.paneldir);
        const File fullPanelPath = baseDir.getChildFile(fullPanelFile);
        const File legacyPanelPath = baseDir.getChildFile(legacyPanelFile);

        FileInputStream in(fullPanelPath);
        if (!in.openedOk())
        {
            details = "Failed to open full panel file for legacy conversion";
            restoreAppState(snapshot);
            return false;
        }

        ValueTree tree = ValueTree::readFromStream(in);
        if (!tree.isValid())
        {
            details = "Failed to parse full panel ValueTree for legacy conversion";
            restoreAppState(snapshot);
            return false;
        }

        static const Identifier instrumentType("Instrument");
        static const Identifier soundConfiguredType("soundConfigured");
        for (int i = 0; i < numbervoicebuttons; ++i)
        {
            ValueTree instrumentNode = tree.getChild(i);
            if (instrumentNode.isValid() && instrumentNode.hasType(instrumentType))
                instrumentNode.removeProperty(soundConfiguredType, nullptr);
        }

        {
            FileOutputStream out(legacyPanelPath);
            if (!out.openedOk())
            {
                details = "Failed to write legacy soundConfigured panel file";
                restoreAppState(snapshot);
                return false;
            }
            tree.writeToStream(out);
        }

        // Poison in-memory state to ensure missing legacy property reverts to default false.
        vb0->setSoundConfigured(true);
        vb10->setSoundConfigured(true);

        if (!instrumentPanel->loadInstrumentPanel(testDir, legacyPanelFile, false))
        {
            details = "loadInstrumentPanel failed for legacy soundConfigured panel";
            restoreAppState(snapshot);
            return false;
        }

        if (!expectEqual(vb0->isSoundConfigured() ? 1 : 0, 0, "legacy panel defaults soundConfigured false (button 0)", details) ||
            !expectEqual(vb10->isSoundConfigured() ? 1 : 0, 0, "legacy panel defaults soundConfigured false (button 10)", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        restoreAppState(snapshot);
        return true;
    }

    bool runPanelOptionalFieldFallbacks(std::string& details)
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
        const String sourcePanelFile = "integration_optional_fields_source.pnl";
        const String legacyPanelFile = "integration_optional_fields_legacy.pnl";

        if (!prepareTestInstrumentJson(testDir, testInstrumentFile, details))
            return false;

        if (!midiInstruments->loadMidiInstruments(state.instrumentfname))
        {
            details = "Failed to load test instrument JSON";
            restoreAppState(snapshot);
            return false;
        }

        state.panelfname = sourcePanelFile;
        state.configfname = "optional_fields.cfg";
        if (!instrumentPanel->initInstrumentPanel(testDir, sourcePanelFile, false))
        {
            details = "initInstrumentPanel for optional field fallback test failed";
            restoreAppState(snapshot);
            return false;
        }

        auto* vb0 = instrumentPanel->getVoiceButton(0);
        if (vb0 == nullptr)
        {
            details = "Failed to get voice button 0";
            restoreAppState(snapshot);
            return false;
        }

        vb0->setSoundConfigured(true);
        state.upperManualRotaryTargetGroup = 2;
        state.lowerManualRotaryTargetGroup = 2;
        if (!instrumentPanel->saveInstrumentPanel(testDir, sourcePanelFile))
        {
            details = "saveInstrumentPanel failed in optional field fallback test";
            restoreAppState(snapshot);
            return false;
        }

        const File baseDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(state.paneldir);
        const File sourcePath = baseDir.getChildFile(sourcePanelFile);
        const File legacyPath = baseDir.getChildFile(legacyPanelFile);

        FileInputStream in(sourcePath);
        if (!in.openedOk())
        {
            details = "Failed to open optional-fields source panel";
            restoreAppState(snapshot);
            return false;
        }

        ValueTree tree = ValueTree::readFromStream(in);
        if (!tree.isValid())
        {
            details = "Failed to parse optional-fields source panel";
            restoreAppState(snapshot);
            return false;
        }

        static const juce::Identifier instrumentType("Instrument");
        static const juce::Identifier soundConfiguredType("soundConfigured");
        static const juce::Identifier manualRotaryUpperTargetGroupId("manualRotaryUpperTargetGroup");
        static const juce::Identifier manualRotaryLowerTargetGroupId("manualRotaryLowerTargetGroup");

        // Simulate legacy/partial data:
        // - remove one optional root property (upper target group)
        // - set another root property to an out-of-range value (lower target group)
        // - remove per-instrument soundConfigured flag.
        tree.removeProperty(manualRotaryUpperTargetGroupId, nullptr);
        tree.setProperty(manualRotaryLowerTargetGroupId, 99, nullptr);
        ValueTree firstInstrument = tree.getChild(0);
        if (firstInstrument.isValid() && firstInstrument.hasType(instrumentType))
            firstInstrument.removeProperty(soundConfiguredType, nullptr);

        {
            FileOutputStream out(legacyPath);
            if (!out.openedOk())
            {
                details = "Failed to write legacy optional-fields panel";
                restoreAppState(snapshot);
                return false;
            }
            tree.writeToStream(out);
        }

        // Poison current state, then verify load applies defaults/clamps from partial panel.
        state.upperManualRotaryTargetGroup = 2;
        state.lowerManualRotaryTargetGroup = 1;
        vb0->setSoundConfigured(true);

        if (!instrumentPanel->loadInstrumentPanel(testDir, legacyPanelFile, false))
        {
            details = "loadInstrumentPanel failed for optional field fallback test";
            restoreAppState(snapshot);
            return false;
        }

        if (!expectEqual(state.upperManualRotaryTargetGroup, 1, "missing upper target group defaults to 1", details) ||
            !expectEqual(state.lowerManualRotaryTargetGroup, 2, "invalid lower target group clamps to 2", details) ||
            !expectEqual(vb0->isSoundConfigured() ? 1 : 0, 0, "missing soundConfigured defaults false", details))
        {
            restoreAppState(snapshot);
            return false;
        }

        restoreAppState(snapshot);
        return true;
    }

    bool runMidiEndpointSmoke(std::string& details)
    {
        const auto inputs = juce::MidiInput::getAvailableDevices();
        const auto outputs = juce::MidiOutput::getAvailableDevices();

        // Presence can vary by CI/host. Enumerating without crash is useful signal already.
        if (inputs.isEmpty() && outputs.isEmpty())
        {
            details = "No MIDI endpoints present; smoke enumeration only";
            return true;
        }

        if (!outputs.isEmpty())
        {
            auto out = juce::MidiOutput::openDevice(outputs[0].identifier);
            if (out != nullptr)
            {
                auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
                noteOn.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                out->sendMessageNow(noteOn);
            }
        }

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

    bool runChannelizedNonNoteStrictRouting(std::string& details)
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

        // Channel accepted in, but with no configured mapping it should not forward.
        devices->passthroughin[3] = true;
        const auto ccNoRoute = MidiMessage::controllerEvent(3, CCVol, 100);
        devices->handleIncomingMidiMessage(nullptr, ccNoRoute);
        if (!expectEqual(static_cast<int>(sentMessages.size()), 0, "unmapped non-note channel is blocked", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        // Configure two explicit routes and verify fan-out rewrites channel.
        devices->iomap[3][0] = 4;
        devices->iomap[3][1] = 15;
        const auto ccMapped = MidiMessage::controllerEvent(3, CCExp, 91);
        devices->handleIncomingMidiMessage(nullptr, ccMapped);

        if (!expectEqual(static_cast<int>(sentMessages.size()), 2, "mapped non-note fan-out count", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        if (!expectEqual(sentMessages[0].getChannel(), 4, "first mapped non-note output channel", details) ||
            !expectEqual(sentMessages[1].getChannel(), 15, "second mapped non-note output channel", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        devices->testSendHook = nullptr;
        return true;
    }

    bool runConfigManagedSysExThroughRouting(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        auto* panel = InstrumentPanel::getInstance();
        if (devices == nullptr || panel == nullptr)
        {
            details = "MidiDevices/InstrumentPanel singleton not available";
            return false;
        }

        if (!expectEqual(panel->getButtonGroup(0)->sysexThrough ? 1 : 0, 0, "button group defaults SysEx Through to false", details)
            || !expectEqual(panel->getButtonGroup(0)->sysexInputIdentifier.isEmpty() ? 1 : 0, 1, "button group defaults SysEx input identifier to empty", details))
            return false;

        devices->clearSysExThroughRoutes();
        devices->midiOutputs.clear();

        MidiDeviceInfo outA;
        outA.identifier = "sysex-out-a";
        outA.name = "SysEx Out A";
        MidiDeviceInfo outB;
        outB.identifier = "sysex-out-b";
        outB.name = "SysEx Out B";
        devices->midiOutputs.add(new MidiDeviceListEntry(outA));
        devices->midiOutputs.add(new MidiDeviceListEntry(outB));

        juce::Array<int> routeOutA;
        routeOutA.add(0);
        devices->addSysExThroughRouteForInputIdentifier("sysex-input-a", routeOutA);

        std::vector<MidiMessage> sentMessages;
        devices->testSendHook = [&sentMessages](const MidiMessage& message)
        {
            sentMessages.push_back(message);
        };
        int monitorHits = 0;
        devices->setOutgoingMidiMonitor([&monitorHits](const MidiMessage& message, const juce::String&)
        {
            if (message.isSysEx())
                ++monitorHits;
        });

        const juce::uint8 sysexData[] { 0xF0, 0x7D, 0x33, 0x44, 0xF7 };
        const auto sysex = MidiMessage::createSysExMessage(sysexData, static_cast<int>(std::size(sysexData)));

        if (!expectEqual(devices->routeIncomingSysExForInputIdentifier("sysex-input-a", sysex) ? 1 : 0, 1,
                         "matching SysEx input routes to output", details)
            || !expectEqual(static_cast<int>(sentMessages.size()), 1, "matching SysEx emits test hook once", details)
            || !expectEqual(monitorHits, 1, "matching SysEx emits one monitor event", details))
        {
            devices->setOutgoingMidiMonitor({});
            devices->testSendHook = nullptr;
            return false;
        }

        juce::Array<int> routeOutBAgainA;
        routeOutBAgainA.add(1);
        routeOutBAgainA.add(0);
        devices->addSysExThroughRouteForInputIdentifier("sysex-input-a", routeOutBAgainA);
        sentMessages.clear();
        monitorHits = 0;

        if (!expectEqual(devices->routeIncomingSysExForInputIdentifier("sysex-input-a", sysex) ? 1 : 0, 1,
                         "matching SysEx fanout routes", details)
            || !expectEqual(static_cast<int>(sentMessages.size()), 1, "fanout SysEx still emits one test hook event", details)
            || !expectEqual(monitorHits, 2, "fanout SysEx emits monitor events per output route", details))
        {
            devices->setOutgoingMidiMonitor({});
            devices->testSendHook = nullptr;
            return false;
        }

        sentMessages.clear();
        monitorHits = 0;
        if (!expectEqual(devices->routeIncomingSysExForInputIdentifier("other-input", sysex) ? 1 : 0, 0,
                         "non-matching SysEx input is dropped", details)
            || !expectEqual(static_cast<int>(sentMessages.size()), 0, "non-matching SysEx emits no test hook", details)
            || !expectEqual(monitorHits, 0, "non-matching SysEx emits no monitor event", details))
        {
            devices->setOutgoingMidiMonitor({});
            devices->testSendHook = nullptr;
            return false;
        }

        devices->setOutgoingMidiMonitor({});
        devices->testSendHook = nullptr;
        return true;
    }

    bool runProgramChangePresetNextTriggerAndConsume(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        const auto snapshot = takeAppStateSnapshot();
        auto& state = getAppState();

        state.presetMidiPcInputChannel = 16;
        state.presetMidiPcValue = 0;

        devices->clearMidiIOMap();
        devices->iomap[16][0] = 4;
        devices->passthroughin[16] = false; // Match check must ignore pass-through gate.

        int triggerHits = 0;
        std::vector<MidiMessage> sentMessages;
        devices->setPresetNextProgramChangeTrigger([&triggerHits]() { ++triggerHits; });
        devices->testSendHook = [&sentMessages](const MidiMessage& message)
        {
            sentMessages.push_back(message);
        };

        const auto matchingPc = MidiMessage::programChange(16, 0);
        devices->handleIncomingMidiMessage(nullptr, matchingPc);

        if (!expectEqual(triggerHits, 1, "matching Program Change triggers preset next callback", details) ||
            !expectEqual(static_cast<int>(sentMessages.size()), 0, "matching Program Change is consumed", details))
        {
            devices->setPresetNextProgramChangeTrigger({});
            devices->testSendHook = nullptr;
            restoreAppState(snapshot);
            return false;
        }

        // Non-matching PC should follow normal non-note routing behavior.
        devices->passthroughin[16] = true;
        const auto nonMatchingPc = MidiMessage::programChange(16, 1);
        devices->handleIncomingMidiMessage(nullptr, nonMatchingPc);

        if (!expectEqual(triggerHits, 1, "non-matching Program Change does not trigger preset next", details) ||
            !expectEqual(static_cast<int>(sentMessages.size()), 1, "non-matching Program Change still routes", details) ||
            !expectEqual(sentMessages[0].getChannel(), 4, "non-matching Program Change routed output channel", details))
        {
            devices->setPresetNextProgramChangeTrigger({});
            devices->testSendHook = nullptr;
            restoreAppState(snapshot);
            return false;
        }

        devices->setPresetNextProgramChangeTrigger({});
        devices->testSendHook = nullptr;
        restoreAppState(snapshot);
        return true;
    }

    bool runIncomingMonitorCapturesRawAndDropped(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        const auto snapshot = takeAppStateSnapshot();
        auto& state = getAppState();
        state.presetMidiPcInputChannel = 16;
        state.presetMidiPcValue = 0;

        devices->clearMidiIOMap();

        std::vector<MidiMessage> inboundMessages;
        std::vector<juce::String> inboundSources;
        std::vector<MidiMessage> sentMessages;
        devices->setIncomingMidiMonitor([&inboundMessages, &inboundSources](const MidiMessage& message, const juce::String& sourceName)
        {
            inboundMessages.push_back(message);
            inboundSources.push_back(sourceName);
        });
        devices->testSendHook = [&sentMessages](const MidiMessage& message)
        {
            sentMessages.push_back(message);
        };

        // Consumed preset-next Program Change still appears in incoming monitor.
        devices->passthroughin[16] = false;
        const auto matchingPc = MidiMessage::programChange(16, 0);
        devices->handleIncomingMidiMessage(nullptr, matchingPc);
        if (!expectEqual(static_cast<int>(inboundMessages.size()), 1, "incoming monitor logs consumed Program Change", details)
            || !expectEqual(static_cast<int>(sentMessages.size()), 0, "consumed Program Change does not emit output", details))
        {
            devices->setIncomingMidiMonitor({});
            devices->testSendHook = nullptr;
            restoreAppState(snapshot);
            return false;
        }

        // Passthrough-blocked channelized message is still visible in incoming monitor.
        const auto blockedCc = MidiMessage::controllerEvent(3, CCVol, 100);
        devices->passthroughin[3] = false;
        devices->handleIncomingMidiMessage(nullptr, blockedCc);
        if (!expectEqual(static_cast<int>(inboundMessages.size()), 2, "incoming monitor logs passthrough-blocked message", details)
            || !expectEqual(static_cast<int>(sentMessages.size()), 0, "blocked channelized message does not emit output", details))
        {
            devices->setIncomingMidiMonitor({});
            devices->testSendHook = nullptr;
            restoreAppState(snapshot);
            return false;
        }

        // Unmapped SysEx drop should still be visible in incoming monitor.
        const juce::uint8 sysexData[] { 0xF0, 0x7D, 0x05, 0x01, 0xF7 };
        const auto sysex = MidiMessage::createSysExMessage(sysexData, static_cast<int>(std::size(sysexData)));
        devices->handleIncomingMidiMessage(nullptr, sysex);
        if (!expectEqual(static_cast<int>(inboundMessages.size()), 3, "incoming monitor logs unmapped SysEx", details)
            || !expectEqual(inboundMessages.back().isSysEx() ? 1 : 0, 1, "incoming monitor preserved SysEx payload", details)
            || !expectEqual(static_cast<int>(sentMessages.size()), 0, "unmapped SysEx does not emit output", details))
        {
            devices->setIncomingMidiMonitor({});
            devices->testSendHook = nullptr;
            restoreAppState(snapshot);
            return false;
        }

        // Routed non-note still appears incoming once and outputs once.
        devices->passthroughin[3] = true;
        devices->iomap[3][0] = 4;
        const auto routedCc = MidiMessage::controllerEvent(3, CCExp, 90);
        devices->handleIncomingMidiMessage(nullptr, routedCc);
        if (!expectEqual(static_cast<int>(inboundMessages.size()), 4, "incoming monitor logs routed channelized message", details)
            || !expectEqual(static_cast<int>(sentMessages.size()), 1, "routed channelized message emits output once", details)
            || !expectEqual(sentMessages[0].getChannel(), 4, "routed channelized message rewrites output channel", details))
        {
            devices->setIncomingMidiMonitor({});
            devices->testSendHook = nullptr;
            restoreAppState(snapshot);
            return false;
        }

        for (const auto& sourceName : inboundSources)
        {
            if (!expectEqual(sourceName == "<unknown-input>" ? 1 : 0, 1, "null source input is labeled unknown", details))
            {
                devices->setIncomingMidiMonitor({});
                devices->testSendHook = nullptr;
                restoreAppState(snapshot);
                return false;
            }
        }

        devices->setIncomingMidiMonitor({});
        devices->testSendHook = nullptr;
        restoreAppState(snapshot);
        return true;
    }

    bool runOutputMuteGateBypassForPlayerPath(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        devices->clearMidiIOMap();
        devices->setOutputChannelMuted(3, true);

        std::vector<MidiMessage> sentMessages;
        devices->testSendHook = [&sentMessages](const MidiMessage& message)
        {
            sentMessages.push_back(message);
        };

        const auto mutedPathMsg = MidiMessage::noteOn(3, 60, (juce::uint8) 100);
        devices->sendToOutputs(mutedPathMsg);
        if (!expectEqual(static_cast<int>(sentMessages.size()), 0, "default sendToOutputs respects output mute gate", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        const auto bypassMsg = MidiMessage::noteOn(3, 62, (juce::uint8) 100);
        devices->sendToOutputs(bypassMsg, true);
        if (!expectEqual(static_cast<int>(sentMessages.size()), 1, "bypass sendToOutputs ignores output mute gate", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }
        if (!expectEqual(sentMessages[0].getChannel(), 3, "bypass keeps original output channel", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }
        if (!expectEqual(sentMessages[0].getNoteNumber(), 62, "bypass forwards expected note payload", details))
        {
            devices->testSendHook = nullptr;
            return false;
        }

        devices->testSendHook = nullptr;
        return true;
    }

    bool runAmtestPlaybackEmitsAllChannelsAndNotes(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        // Ensure deterministic single-stream observation independent of previous routing tests.
        devices->clearMidiIOMap();

        const auto repoRoot = File(__FILE__).getParentDirectory().getParentDirectory();
        const auto amtest = repoRoot.getChildFile("docs").getChildFile("midi").getChildFile("amtest.mid");
        if (!amtest.existsAsFile())
        {
            details = "amtest.mid not found at " + amtest.getFullPathName().toStdString();
            return false;
        }

        MidiFilePlaybackEngine engine;
        juce::String loadError;
        if (!engine.loadFromFile(amtest, loadError))
        {
            details = "MidiFilePlaybackEngine load failed: " + loadError.toStdString();
            return false;
        }

        std::array<int, 17> pcCount {};
        std::array<int, 17> noteOnCount {};
        std::array<int, 17> firstProgram {};
        std::array<std::vector<int>, 17> noteOnSequence {};
        firstProgram.fill(-1);

        devices->setOutgoingMidiMonitor([&](const MidiMessage& msg, const juce::String&)
            {
                const int ch = msg.getChannel();
                if (ch < 1 || ch > 16)
                    return;

                if (msg.isProgramChange())
                {
                    ++pcCount[(size_t) ch];
                    if (firstProgram[(size_t) ch] < 0)
                        firstProgram[(size_t) ch] = msg.getProgramChangeNumber();
                }
                else if (msg.isNoteOn())
                {
                    ++noteOnCount[(size_t) ch];
                    noteOnSequence[(size_t) ch].push_back(msg.getNoteNumber());
                }
            });

        engine.start(0.0);
        const auto processed = engine.processUntil(1.0e12, [&](const MidiMessage& msg)
            {
                devices->sendToOutputs(msg);
            });

        devices->setOutgoingMidiMonitor({});
        juce::ignoreUnused(processed);

        static const std::array<int, 8> expectedPattern { 60, 62, 64, 65, 67, 69, 71, 72 };
        static constexpr int expectedNotesPerChannel = 800;

        for (int ch = 1; ch <= 16; ++ch)
        {
            if (!expectEqual(pcCount[(size_t) ch], 1, "program change count channel " + std::to_string(ch), details))
                return false;
            if (!expectEqual(firstProgram[(size_t) ch], ch - 1, "first program value channel " + std::to_string(ch), details))
                return false;
            if (!expectEqual(noteOnCount[(size_t) ch], expectedNotesPerChannel, "note-on count channel " + std::to_string(ch), details))
                return false;

            const auto& sequence = noteOnSequence[(size_t) ch];
            if (!expectEqual((int) sequence.size(), expectedNotesPerChannel, "note sequence size channel " + std::to_string(ch), details))
                return false;

            for (int i = 0; i < expectedNotesPerChannel; ++i)
            {
                const int expectedNote = expectedPattern[(size_t) (i % (int) expectedPattern.size())];
                if (sequence[(size_t) i] != expectedNote)
                {
                    details = "note pattern mismatch channel " + std::to_string(ch)
                        + " index " + std::to_string(i)
                        + " expected " + std::to_string(expectedNote)
                        + " got " + std::to_string(sequence[(size_t) i]);
                    return false;
                }
            }
        }

        return true;
    }

    bool runType0MidiLoadAndPlaybackRegression(std::string& details)
    {
        auto* devices = MidiDevices::getInstance();
        if (devices == nullptr)
        {
            details = "MidiDevices::getInstance() returned nullptr";
            return false;
        }

        devices->clearMidiIOMap();

        const auto tempRoot = File::getSpecialLocation(File::tempDirectory)
            .getChildFile("AMidiOrganTests");
        if (!tempRoot.exists() && !tempRoot.createDirectory())
        {
            details = "Failed to create temporary directory: " + tempRoot.getFullPathName().toStdString();
            return false;
        }

        const auto midiFilePath = tempRoot.getNonexistentChildFile("type0-regression", ".mid");

        MidiFile type0Write;
        type0Write.setTicksPerQuarterNote(480);
        MidiMessageSequence track;
        track.addEvent(MidiMessage::programChange(1, 10), 0.0);
        track.addEvent(MidiMessage::noteOn(1, 60, (uint8) 100), 0.0);
        track.addEvent(MidiMessage::noteOff(1, 60), 480.0);
        type0Write.addTrack(track);

        {
            FileOutputStream out(midiFilePath);
            if (!out.openedOk())
            {
                details = "Failed to open Type 0 MIDI output file: " + midiFilePath.getFullPathName().toStdString();
                return false;
            }

            if (!type0Write.writeTo(out, 0))
            {
                details = "Failed to write Type 0 MIDI file: " + midiFilePath.getFullPathName().toStdString();
                return false;
            }
        }

        {
            FileInputStream in(midiFilePath);
            if (!in.openedOk())
            {
                details = "Failed to reopen generated Type 0 MIDI file: " + midiFilePath.getFullPathName().toStdString();
                return false;
            }

            MidiFile parsed;
            int midiType = -1;
            if (!parsed.readFrom(in, true, &midiType))
            {
                details = "JUCE failed to parse generated Type 0 MIDI file";
                return false;
            }

            if (!expectEqual(midiType, 0, "generated MIDI file type", details))
                return false;
            if (!expectEqual(parsed.getNumTracks(), 1, "generated Type 0 track count", details))
                return false;
        }

        MidiFilePlaybackEngine engine;
        juce::String loadError;
        if (!engine.loadFromFile(midiFilePath, loadError))
        {
            details = "MidiFilePlaybackEngine failed to load Type 0 MIDI: " + loadError.toStdString();
            return false;
        }

        int programChanges = 0;
        int noteOns = 0;
        int noteOffs = 0;
        int firstProgram = -1;
        int firstNote = -1;

        devices->setOutgoingMidiMonitor([&](const MidiMessage& msg, const juce::String&)
            {
                if (msg.getChannel() != 1)
                    return;

                if (msg.isProgramChange())
                {
                    ++programChanges;
                    if (firstProgram < 0)
                        firstProgram = msg.getProgramChangeNumber();
                }
                else if (msg.isNoteOn())
                {
                    ++noteOns;
                    if (firstNote < 0)
                        firstNote = msg.getNoteNumber();
                }
                else if (msg.isNoteOff())
                {
                    ++noteOffs;
                }
            });

        engine.start(0.0);
        engine.processUntil(1.0e9, [&](const MidiMessage& msg)
            {
                devices->sendToOutputs(msg);
            });
        devices->setOutgoingMidiMonitor({});

        if (!expectEqual(programChanges, 1, "Type 0 program change count", details))
            return false;
        if (!expectEqual(firstProgram, 10, "Type 0 first program value", details))
            return false;
        if (!expectEqual(noteOns, 1, "Type 0 note-on count", details))
            return false;
        if (!expectEqual(noteOffs, 1, "Type 0 note-off count", details))
            return false;
        if (!expectEqual(firstNote, 60, "Type 0 first note number", details))
            return false;

        return true;
    }

    bool runShortcutFocusDeferralGuard(std::string& details)
    {
        TextEditor editor;
        ComboBox combo;
        TextButton button ("btn");

        if (!shouldDeferKeyboardShortcutsForFocusedComponent (&editor))
        {
            details = "TextEditor should defer shortcuts";
            return false;
        }

        if (!shouldDeferKeyboardShortcutsForFocusedComponent (&combo))
        {
            details = "ComboBox should defer shortcuts";
            return false;
        }

        if (shouldDeferKeyboardShortcutsForFocusedComponent (&button))
        {
            details = "TextButton should not defer shortcuts";
            return false;
        }

        if (shouldDeferKeyboardShortcutsForFocusedComponent (nullptr))
        {
            details = "nullptr focus should not defer shortcuts";
            return false;
        }

        return true;
    }

    bool runHotkeyDuplicateAssignmentGuard(std::string& details)
    {
        HotkeyBindings b = HotkeyBindings::withDefaults();

        if (hasDuplicateHotkeyAssignments(b))
        {
            details = "built-in defaults must not contain duplicates";
            return false;
        }

        b.keys[0] = L'x';
        b.keys[1] = L'x';

        if (!hasDuplicateHotkeyAssignments(b))
        {
            details = "duplicate keys should be detected";
            return false;
        }

        b.keys[0] = std::nullopt;
        b.keys[1] = std::nullopt;

        if (hasDuplicateHotkeyAssignments(b))
        {
            details = "multiple (None) assignments should not count as duplicates";
            return false;
        }

        return true;
    }

    bool runPendingExitSavePromptHelpers(std::string& details)
    {
        clearPendingExitSavePrompt();

        bpendingSoundEdit = true;
        bpendingEffectsEdit = true;
        bpendingPresetSet = true;
        if (!expectEqual(hasPendingExitSavePrompt() ? 1 : 0, 1, "pending prompt starts true", details))
            return false;

        clearPendingSoundEditPrompt();
        if (!expectEqual(bpendingSoundEdit ? 1 : 0, 0, "clearPendingSoundEditPrompt clears sound flag", details))
            return false;
        if (!expectEqual(bpendingEffectsEdit ? 1 : 0, 1, "clearPendingSoundEditPrompt preserves effects flag", details))
            return false;
        if (!expectEqual(bpendingPresetSet ? 1 : 0, 1, "clearPendingSoundEditPrompt preserves preset flag", details))
            return false;

        clearPendingEffectsEditPrompt();
        if (!expectEqual(bpendingEffectsEdit ? 1 : 0, 0, "clearPendingEffectsEditPrompt clears effects flag", details))
            return false;
        if (!expectEqual(bpendingPresetSet ? 1 : 0, 1, "clearPendingEffectsEditPrompt preserves preset flag", details))
            return false;

        if (!expectEqual(hasPendingExitSavePrompt() ? 1 : 0, 1, "pending still true while preset pending", details))
            return false;

        clearPendingExitSavePrompt();
        if (!expectEqual(hasPendingExitSavePrompt() ? 1 : 0, 0, "clearPendingExitSavePrompt clears all flags", details))
            return false;

        return true;
    }

    bool runType2RotaryActionResolution(std::string& details)
    {
        const auto brakeOn = resolveType2RotaryAction(true, true);
        if (!expectEqual(static_cast<int>(brakeOn), static_cast<int>(Type2RotaryAction::hardStop),
                         "type-2 brake-on resolves to hard stop", details))
            return false;

        const auto brakeOffFast = resolveType2RotaryAction(true, false);
        if (!expectEqual(static_cast<int>(brakeOffFast), static_cast<int>(Type2RotaryAction::startFastRamp),
                         "type-2 brake-off fast resolves to fast ramp", details))
            return false;

        const auto brakeOffSlow = resolveType2RotaryAction(false, false);
        return expectEqual(static_cast<int>(brakeOffSlow), static_cast<int>(Type2RotaryAction::restoreSlowTarget),
                           "type-2 brake-off slow resolves to slow restore target", details);
    }

    bool runType2RotarySlowRestoreConstant(std::string& details)
    {
        auto* modules = InstrumentModules::getInstance();
        if (modules == nullptr)
        {
            details = "InstrumentModules::getInstance() returned nullptr";
            return false;
        }

        constexpr int kDeebachModuleId = 1;
        const int type2SlowValue = modules->getRotorSlow(kDeebachModuleId);
        const int type2FastValue = modules->getRotorFast(kDeebachModuleId);

        if (!expectEqual(type2SlowValue, 20, "type-2 slow restore value", details))
            return false;

        const auto action = resolveType2RotaryAction(false, false);
        if (!expectEqual(static_cast<int>(action), static_cast<int>(Type2RotaryAction::restoreSlowTarget),
                         "slow restore action selected", details))
            return false;

        // Behavioral proxy: slow restore path is a single deterministic target value.
        return expectEqual(type2SlowValue == type2FastValue ? 1 : 0, 0,
                           "slow restore target avoids transient fast spike value", details);
    }

    bool runType2RotaryRapidToggleDecisionStability(std::string& details)
    {
        const std::array<std::pair<bool, bool>, 6> toggles{ {
            { true, false },   // fast
            { false, false },  // slow
            { true, false },   // fast
            { false, true },   // brake on
            { false, false },  // brake off slow
            { true, false }    // fast
        } };

        int hardStops = 0;
        int fastRamps = 0;
        int slowRestores = 0;
        for (const auto& step : toggles)
        {
            const auto action = resolveType2RotaryAction(step.first, step.second);
            if (action == Type2RotaryAction::hardStop) ++hardStops;
            if (action == Type2RotaryAction::startFastRamp) ++fastRamps;
            if (action == Type2RotaryAction::restoreSlowTarget) ++slowRestores;
        }

        if (!expectEqual(hardStops, 1, "rapid toggles include one hard-stop action", details))
            return false;
        if (!expectEqual(fastRamps, 3, "rapid toggles include three fast-ramp actions", details))
            return false;
        return expectEqual(slowRestores, 2, "rapid toggles include two slow-restore actions", details);
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
        results.push_back({ "Configured label colour helper returns black/grey as expected", runConfiguredLabelColourHelpers(details), details });
    }

    {
        std::string details;
        results.push_back({ "Preset display-bank helpers map presets to visible slots correctly", runPresetDisplayBankHelpers(details), details });
    }

    {
        std::string details;
        results.push_back({ "MidiInstruments voice lookup bounds (no crash on category slot index)", runMidiInstrumentsVoiceLookupBounds(details), details });
    }

    {
        std::string details;
        results.push_back({ "Volume helper math and clamping", runVolumeMathHelpers(details), details });
    }

    {
        std::string details;
        results.push_back({ "Player strip CC merge helper math and whitelist behavior", runPlayerStripCcMergeHelpers(details), details });
    }

    {
        std::string details;
        results.push_back({ "Player song profile codec roundtrip preserves key fields", runPlayerSongProfileCodecRoundtrip(details), details });
    }

    {
        std::string details;
        results.push_back({ "Player profiles index roundtrip and midi-key lookup", runPlayerSongProfilesIndexRoundtrip(details), details });
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
        results.push_back({ "Module match aliases and legacy fallback", runModuleMatchAliasBehavior(details), details });
    }

    {
        std::string details;
        results.push_back({ "lookupPanelGroup maps ranges", runLookupPanelGroup(details), details });
    }

    {
        std::string details;
        results.push_back({ "Shortcut focus deferral (TextEditor/ComboBox vs controls)", runShortcutFocusDeferralGuard(details), details });
    }

    {
        std::string details;
        results.push_back({ "Hotkey duplicate detection (non-empty vs (None))", runHotkeyDuplicateAssignmentGuard(details), details });
    }

    {
        std::string details;
        results.push_back({ "Pending-save helper flags clear only intended scopes", runPendingExitSavePromptHelpers(details), details });
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
        results.push_back({ "MidiDevices fanout routes one channel to multiple modules", runModuleFanoutByOutputChannel(details), details });
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
        results.push_back({ "Manual rotary target defaults to group 1", runManualRotaryTargetDefaults(details), details });
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
        results.push_back({ "Preset Next boundaries include bank transitions", runPresetNextCycleBoundaries(details), details });
    }

    {
        std::string details;
        results.push_back({ "Panel load supports legacy 7-preset payload", runLegacyPresetLoadCompatibility(details), details });
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
        results.push_back({ "VoiceButton soundConfigured persists across panel save/load", runVoiceSoundConfiguredPersistenceRoundtrip(details), details });
    }

    {
        std::string details;
        results.push_back({ "Legacy panel without soundConfigured defaults all voice flags false", runVoiceSoundConfiguredLegacyDefaultFalse(details), details });
    }

    {
        std::string details;
        results.push_back({ "Partial legacy panel fields fallback to safe defaults", runPanelOptionalFieldFallbacks(details), details });
    }

    {
        std::string details;
        results.push_back({ "MIDI endpoint smoke: enumerate/open/send without crash", runMidiEndpointSmoke(details), details });
    }

    {
        std::string details;
        results.push_back({ "MIDI routing split/layer edge cases", runMidiRoutingSplitLayerEdgeCases(details), details });
    }

    {
        std::string details;
        results.push_back({ "MIDI non-note routing follows configured channels only", runChannelizedNonNoteStrictRouting(details), details });
    }

    {
        std::string details;
        results.push_back({ "Config-managed SysEx through routes by input identifier with fanout dedupe", runConfigManagedSysExThroughRouting(details), details });
    }

    {
        std::string details;
        results.push_back({ "MIDI Program Change can trigger consumed preset next", runProgramChangePresetNextTriggerAndConsume(details), details });
    }

    {
        std::string details;
        results.push_back({ "MIDI IN monitor captures raw traffic including consumed and dropped paths", runIncomingMonitorCapturesRawAndDropped(details), details });
    }

    {
        std::string details;
        results.push_back({ "Output mute gate bypass allows Player-path notes while preserving default mute gate", runOutputMuteGateBypassForPlayerPath(details), details });
    }

    {
        std::string details;
        results.push_back({ "amtest.mid playback emits all channels and expected note pattern", runAmtestPlaybackEmitsAllChannelsAndNotes(details), details });
    }

    {
        std::string details;
        results.push_back({ "Type 0 MIDI regression: parse + playback emits expected channel events", runType0MidiLoadAndPlaybackRegression(details), details });
    }

    {
        std::string details;
        results.push_back({ "Type-2 rotary action branch selection", runType2RotaryActionResolution(details), details });
    }

    {
        std::string details;
        results.push_back({ "Type-2 rotary slow restore avoids transient fast value", runType2RotarySlowRestoreConstant(details), details });
    }

    {
        std::string details;
        results.push_back({ "Type-2 rotary rapid-toggle decision stability", runType2RotaryRapidToggleDecisionStability(details), details });
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
