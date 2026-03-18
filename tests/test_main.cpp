#include <JuceHeader.h>

using namespace juce;

#include "../AMidiUtils.h"
#include "../AMidiDevices.h"
#include "../AMidiInstruments.h"
#include "../AMidiControl.h"

#include <iostream>
#include <string>
#include <vector>

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
        results.push_back({ "AppState aliases remain compatible", runAppStateAliasBackcompat(details), details });
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

    return failedCount == 0 ? 0 : 1;
}
