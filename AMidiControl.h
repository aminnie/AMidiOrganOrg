/*
==============================================================================
 AMidiOrgan — main UI and orchestration (tabs, panels, MIDI routing, presets).
 See README.md for features and build; AGENTS.md for contributor notes.
==============================================================================
*/

#pragma once

#include "AMidiUtils.h"
#include "AMidiHotkeys.h"
#include "AMidiDevices.h"
#include "AMidiInstruments.h"
#include "AMidiButtons.h"
#include "AMidiRotors.h"

#include <functional>
#include <atomic>
#include <map>
#include <memory>
#include <vector>
#include<iostream>
#include<fstream>

#ifndef AMIDIORGAN_PROJECT_VERSION
#define AMIDIORGAN_PROJECT_VERSION "1.0.0"
#endif

#ifndef AMIDIORGAN_BUILD_NUMBER
#define AMIDIORGAN_BUILD_NUMBER "local"
#endif

// Logging to a file
// https://forum.juce.com/t/logging-to-file-in-plugin/30950
// Widgets to use in future release
// https://foleysfinest.com/developer/pluginguimagic/
// MidiPlayer from Juce Forum
// https://forum.juce.com/t/improving-how-midi-is-played-in-an-audioappcomponent/58825


//==============================================================================
// Class: MidiPanels - Singleton and buttons JSON object.
//==============================================================================
class MidiPanels final : public Component
{

public:
    // Get the singleton instance object of this class
    static MidiPanels& getInstance()
    {
        static MidiPanels instance; // Guaranteed to be destroyed, instantiated on first use.
        return instance;
    }

    bool loadPanel() {
        // Read the Panel JSON file from disk into a string
        //juce::File panelFile("C:/amidi/midifiles/panel_master.json");
        juce::File panelFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile("panel_master.json");
        if (!panelFile.existsAsFile()) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Panel file not found!", "panel_master.json");

            juce::Logger::writeToLog("Panel file not found: panel_master.json");
            return false;
        }
        auto panelText = panelFile.loadFileAsString();

        // Parse the JSON string into a var object.
        jsonPanel = juce::JSON::parse(panelText);

        return true;
    }

    bool savePanel() {

        return true;
    }

private:
    // Private constructor so only one objects instance can be created.
    MidiPanels() {}

    ~MidiPanels() {
    };

    juce::var jsonPanel;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiPanels)
};


//==============================================================================
// Class: Singleton PanelPresets - 7 x Panel Presets for Upper, Lower and Bass Keyboards
//==============================================================================
static int zinstcntPanelPresets = 0;
class PanelPresets final : Component
{
public:
    ~PanelPresets() {
        DBG("=S= PanelPresets(): Destructor " + std::to_string(--zinstcntPanelPresets));

        // Ensure no dangling pointers are left when Singleton is deleted.
        clearSingletonInstance();
    };

    bool createPreset(int presetid, int buttongroupid, int panelbuttonidx, bool mutestate, int rotarystate) {

        if ((presetid < 0) || (presetid >= numberpresets)) {
            return false;
        }

        if ((buttongroupid < 0) || (buttongroupid > 11)) {
            return false;
        }

        if ((panelbuttonidx < 0) || (panelbuttonidx > 95)) {
            return false;
        }

        presets[presetid]->setActiveButton(buttongroupid, panelbuttonidx);
        presets[presetid]->setMuteStatus(buttongroupid, mutestate);
        presets[presetid]->setRotaryStatus(buttongroupid, rotarystate);

        return true;
    };

    int getPanelButtonIdx(int presetid, int buttongroupid) {
        return presets[presetid]->getActiveButton(buttongroupid);
    }

    void setPanelButtonIdx(int presetid, int buttongroupid, int panelbuttonidx) {
        presets[presetid]->setActiveButton(buttongroupid, panelbuttonidx);
    }

    bool getMuteStatus(int presetid, int buttongroupid) {
        return presets[presetid]->getMuteStatus(buttongroupid);
    }

    void setMuteStatus(int presetid, int buttongroupid, bool mutestatus) {
        presets[presetid]->setMuteStatus(buttongroupid, mutestatus);
    }

    int getRotaryStatus(int presetid, int buttongroupid) {
        return presets[presetid]->getRotaryStatus(buttongroupid);
    }

    void setRotaryStatus(int presetid, int buttongroupid, int rotarystate) {
        presets[presetid]->setRotaryStatus(buttongroupid, rotarystate);
    }

    // Track Last Preset Button Pressed
    int getActivePresetIdx() {
        return activepresetidx;
    }

    void setActivePresetIdx(int pstidx) {
        activepresetidx = pstidx;
    }

    juce_DeclareSingleton(PanelPresets, true)

private:
    // Constructor Private
    PanelPresets() {
        juce::Logger::writeToLog("=S= PanelPresets(): Constructor " + std::to_string(zinstcntPanelPresets++));

        // Create and default 7 presets: 1 x Manual + 6 x Custom Presets
        for (int i = 0; i < numberpresets; i++) {
            presets.add(new Preset());

            createPreset(i, 0, 0, false, 0);
            createPreset(i, 1, 12, true, 0);
            createPreset(i, 2, 20, true, 0);
            createPreset(i, 3, 26, true, 0);

            createPreset(i, 4, 32, false, 0);
            createPreset(i, 5, 44, true, 0);
            createPreset(i, 6, 52, true, 0);
            createPreset(i, 7, 58, true, 0);

            createPreset(i, 8, 64, false, 0);
            createPreset(i, 9, 76, true, 0);
            createPreset(i, 10, 84, true, 0);
            createPreset(i, 11, 90, true, 0);
        }
    };

    // Each Preset captures all the clicked/acthive VoiceButtons, Mute and Rotary States in each of the 12 Button Groups
    class Preset final {

    public:
        Preset() {};

        ~Preset() {};

        void setPresetLoaded(bool bpresetloaded) {
            presetloaded = bpresetloaded;
        }

        bool getPresetLoaded() {
            return presetloaded;
        }

        int getActiveButton(int buttongroupid) {
            return activebuttons[buttongroupid];
        };

        void setActiveButton(int buttongroupid, int panelbuttonidx) {
            activebuttons[buttongroupid] = panelbuttonidx;

            presetloaded = true;
        };

        bool getMuteStatus(int buttongroupid) {
            return mutestatus[buttongroupid];
        };

        void setMuteStatus(int buttongroupid, bool mutestate) {
            mutestatus[buttongroupid] = mutestate;

            presetloaded = true;
        };

        int getRotaryStatus(int buttongroupid) {
            return rotarystatus[buttongroupid];
        };

        void setRotaryStatus(int buttongroupid, int rotarystate) {
            rotarystatus[buttongroupid] = rotarystate;

            presetloaded = true;
        };

    private:
        bool presetloaded = false;

        std::array<int, 12> activebuttons;
        std::array<bool, 12> mutestatus;
        std::array<int, 12> rotarystatus;
    };

    int activepresetidx = 0;
    OwnedArray<Preset> presets;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelPresets)
};
juce_ImplementSingleton(PanelPresets)


//==============================================================================
// Class: ButtonGroup - Buttons Groups Upper, Lower and Keyboard Manuals
//==============================================================================
struct ButtonGroup final : Component
{
    ButtonGroup() {};

    ~ButtonGroup() {};

    ButtonGroup(int keyboard, String name, int count, int in, int out, bool mute) {
        midikeyboard = keyboard;
        groupname = name;
        buttoncount = count;
        midiin = in;
        midiout = out;
        muted = mute;
        velocity = true;
        rotary = 0;
        moduleidx = 1;
    }

    // Midi Module selection by Button Group
    // Intended to allow for different midi out modules by Button Group and midout channel
    int getMidiModule() {
        return moduleidx;
    }

    void setMidiModule(int mmoduleidx) {
        moduleidx = mmoduleidx;
    }

    // Track last pressed Button in Group for Preset purposes
    int getActiveVoiceButton() {
        return activevoicebutton;
    }

    void setActiveVoiceButton(int panelbuttonidx) {
        activevoicebutton = panelbuttonidx;
    }

    int getMasterVolStep() {
        return mastervolstep;
    }

    void setMasterVolStep(int sliderstep) {
        mastervolstep = juce::jlimit(0, 10, sliderstep);
    }

    // Track Velocity in Button Group
    bool getVelocityButtonStatus() {
        return velocity;
    }

    void setVelocityButtonStatus(bool mvelocity) {
        velocity = mvelocity;
    }

    // Track Muted in Button Group for Preset purposes
    bool getMuteButtonStatus() {
        return muted;
    }

    // Track Muted in Button Group for Preset purposes
    bool isEffectDirty(int effnum, int effval) {

        if ((effnum < 0) || (effnum > 9)) return false;

        if (cureffectval[effnum] != effval) {
            cureffectval[effnum] = effval;
            return true;
        }

        return false;
    }

    void setMuteButtonStatus(bool bmute) {
        muted = bmute;
    }

    // Link between Buttons on the Display Component and Button Group
    MuteButton* getMuteButtonPtr() {
        return mutebuttonptr;
    }

    void setMuteButtonPtr(MuteButton* mutebtnptr) {
        mutebuttonptr = mutebtnptr;
    }

    // Link between Buttons on the Display Component and Button Group
    GroupComponent* getGroupComponentPtr() {
        return groupcomponentptr;
    }

    void setGroupComponentPtr(GroupComponent* grpcommponentptr) {
        groupcomponentptr = grpcommponentptr;
    }

    //Defaults. Config pages overrides this

    // Default Midi instrument module associated with Button Group.
    // To do: Support different midi out module by Button Group
    int moduleidx = 1;
    String modulealias = "";

    int midikeyboard = KBDUPPER;
    String groupname = "Organ";
    int buttoncount = 12;

    int midiin = 1;
    int midiout = 1;

    int rotary = 0;
    bool muted = true;
    bool velocity = true;

    int octxpose = 0;
    int splitout = 0;
    String splitoutname = "--";

    int cureffectval[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int mastervolstep = 8;

    int activevoicebutton = 0;
    Component::SafePointer<MuteButton>mutebuttonptr;
    Component::SafePointer<GroupComponent>groupcomponentptr;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonGroup)
};


//==============================================================================
// Class: InstrumentPanel Singleton
//==============================================================================
static int zinstcntInstrumentPanel = 0;
class InstrumentPanel
    : public Component
{
public:

    ~InstrumentPanel()
    {
        DBG("=S= InstrumentPanel(): Destructor " + std::to_string(--zinstcntInstrumentPanel));

        // Ensure no dangling pointers are left when Singleton is deleted.
        clearSingletonInstance();
    }

    //-------------------------------------------------------------------------
    // Return the VoiceButton at Panel OwnedArray index
    //-------------------------------------------------------------------------
    VoiceButton* getVoiceButton(int panelbtnidx) {

        int pidx = panelbtnidx;
        //const int idx = panelvoicebuttons.size();
        if ((panelbtnidx < 0) || (panelbtnidx > (numbervoicebuttons-1))) {
            DBG("*** getVoiceButton(): Error Loading InstrumentPanel: panelvoicebutton index is " + std::to_string(pidx));;
            pidx = 0;
        }
        VoiceButton* pvb = (VoiceButton*)panelvoicebuttons.getUnchecked(pidx);

        return pvb;
    }

    //-------------------------------------------------------------------------
    bool setVoiceButton(int panelbtnidx, VoiceButton vb) {

        if (!(panelbtnidx >= 0) && (panelbtnidx < numbervoicebuttons))
            return false;

        VoiceButton* pvb = panelvoicebuttons[panelbtnidx];

        pvb->setInstrument(vb.getInstrument());
        pvb->setButtonText(vb.getInstrument().getVoice());

        // To do: Add button group indexes
        pvb->setButtonGroupId(vb.getButtonGroupId());
        pvb->setButtonId(vb.getButtonId());
        pvb->setPanelButtonIdx(vb.getPanelButtonIdx());

        return true;
    }

    // Return the requested ButtonGroup with associated details
    ButtonGroup* getButtonGroup(int groupidx) {
        ButtonGroup* bgroup = buttongroups[groupidx];
        return bgroup;
    }

    /** Copy active preset rotary snapshot into each ButtonGroup::rotary (used before Save / after panel load). */
    void syncButtonGroupRotaryFromActivePreset()
    {
        if (panelpresets == nullptr)
            return;

        const int pst = juce::jlimit(0, numberpresets - 1, panelpresets->getActivePresetIdx());
        for (int i = 0; i < numberbuttongroups; ++i)
        {
            const int r = juce::jlimit(0, 2, panelpresets->getRotaryStatus(pst, i));
            getButtonGroup(i)->rotary = r;
        }
    }

    /** Send rotor CC for each button group from current ButtonGroup::rotary (e.g. after preset recall). */
    void sendMidiForStoredButtonGroupRotaryState()
    {
        auto* mod = InstrumentModules::getInstance();
        auto* dev = MidiDevices::getInstance();
        if (mod == nullptr || dev == nullptr)
            return;

        for (int i = 0; i < numberbuttongroups; ++i)
        {
            ButtonGroup* bg = getButtonGroup(i);
            const int r = juce::jlimit(0, 2, bg->rotary);
            const int midx = bg->moduleidx;
            if (mod->getRotorType(midx) < 1)
                continue;

            const int mout = bg->midiout;
            const int cc = mod->getRotorCC(midx);
            int val = mod->getRotorSlow(midx);
            if (r == 2)
                val = mod->getRotorOff(midx);
            else if (r == 1)
                val = mod->getRotorFast(midx);

            auto ccMessage = juce::MidiMessage::controllerEvent(mout, cc, val);
            ccMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            dev->sendToOutputs(ccMessage);
        }
    }

    //-------------------------------------------------------------------------
    bool initInstrumentPanel(String instrumentDirectoryName, const String& panelFileName, bool fromdisk) 
    {
        juce::Logger::writeToLog("*** initInstrumentPanel(): Loading " + panelFileName + " instrument panel file");

        //-------------------------------------------------------------------------
        // Create the Master Panel from scratch with default voices and make it 
        // available for panel loads from disk to overwrite/update in place.

        // Create Instruments on the Panel directly from Vendor Instrument File
        panelbuttonidx = 0;
        int instrumentgroup = 0;
        int voiceidx = 0;

        int buttongroupidx = 0;
        ButtonGroup* ptrbuttongroup = getButtonGroup(buttongroupidx);
        int buttongroupmidiout = ptrbuttongroup->midiout;

        for (int i = 0; i < numbervoicebuttons; ++i)
        {
            // Pre-load instruments from JSON Instrument doc
            // Upper
            if (i == 0) {
                instrumentgroup = 0;    // 2 Upper Organ
                voiceidx = 0;
            }
            else if (i == 12) {
                instrumentgroup = 0;    // 5 Upper Orchestral
                voiceidx = 0;
            }
            else if (i == 20) {
                instrumentgroup = 0;    // 7 Upper Symphonic
                voiceidx = 0;
            }
            else if (i == 26) {
                instrumentgroup = 0;    // 9 Upper Solo
                voiceidx = 0;
            }
            // Lower
            else if (i == 32) {
                instrumentgroup = 0;    // 2 Lower Organ
                voiceidx = 0;
            }
            else if (i == 44) {
                instrumentgroup = 0;    // 0 Lower Orchestral
                voiceidx = 0;
            }
            else if (i == 52) {
                instrumentgroup = 0;    // 14 Lower Symphonic
                voiceidx = 0;
            }
            else if (i == 58) {
                instrumentgroup = 0;    // 9 Lower Solo
                voiceidx = 0;
            }
            // Bass
            else if (i == 64) {
                instrumentgroup = 0;    // 2 Bass Organ
                voiceidx = 0;
            }
            else if (i == 76) {
                instrumentgroup = 0;    // 6 Bass Orchestral
                voiceidx = 0;
            }
            else if (i == 84) {
                instrumentgroup = 0;    // 18 Drums 1
                voiceidx = 0;
            }
            else if (i == 90) {
                instrumentgroup = 0;    // 19 Drums 2
                voiceidx = 0;
            }

            // Create a new Instrument object to add to the Panel
            // For categories with less instruments than Panel Group Buttons, default Sound
            //Instrument instrument = midiinstruments->getInstrument(instrumentgroup, voiceidx + 1);
            //int ccount = midiinstruments->getCountforCategory(instrumentgroup);
            Instrument instrument;

            if (voiceidx >= midiinstruments->getCategoryVoiceCount(instrumentgroup)) {
                instrument.setMSB(sdefMSB);
                instrument.setLSB(sdefLSB);
                instrument.setFont(sdefFont);
                instrument.setVoice(sdefVoice);
            }
            else {
                instrument.setMSB(midiinstruments->getMSB(instrumentgroup, voiceidx + 1));
                instrument.setLSB(midiinstruments->getLSB(instrumentgroup, voiceidx + 1));
                instrument.setFont(midiinstruments->getFont(instrumentgroup, voiceidx + 1));
                instrument.setVoice(midiinstruments->getVoice(instrumentgroup, voiceidx + 1));
            }

            instrument.setVol(appState.defaultEffectsVol);
            instrument.setBri(appState.defaultEffectsBri);
            instrument.setExp(appState.defaultEffectsExp);
            instrument.setRev(appState.defaultEffectsRev);
            instrument.setCho(appState.defaultEffectsCho);
            instrument.setMod(appState.defaultEffectsMod);
            instrument.setTim(appState.defaultEffectsTim);
            instrument.setAtk(appState.defaultEffectsAtk);
            instrument.setRel(appState.defaultEffectsRel);
            instrument.setPan(appState.defaultEffectsPan);

            instrument.setChannel(buttongroupmidiout);
            instrument.setButtonIdx(panelbuttonidx);

            VoiceButton* vb = new VoiceButton();
            panelvoicebuttons.add(vb);

            vb->setInstrument(instrument);

            // To do: Seem redundant. See if we can remove
            vb->setButtonText(instrument.getVoice());

            // To do: Add button group indexes
            ////panelvoicebuttons[i]->setButtonGroupId(1); //change all back to this if ot ownedarray
            vb->setButtonGroupId(1);
            vb->setButtonId(i);
            vb->setPanelButtonIdx(panelbuttonidx);

            // Increase, but do not reset between sound groups as it is the vector index for all button groups
            voiceidx++;
            panelbuttonidx++;
        }

        // Next we proceed to load an instument panel file from disk
        if (fromdisk) {
            if (!loadInstrumentPanel(instrumentDirectoryName, panelFileName, false))
            {
                DBG("*** initInstrumentPanel(): Load InstrumentPanel from disk failed");
                juce::Logger::writeToLog("*** initInstrumentPanel(): Failed to load " + panelFileName + " instrument panel file");

                // To do: Add some other error handling

                return false;
            }
        }
        else {
            // When not loading from disk, we recreate the masterpanel file - for now
            // and until custom panels supported.
            saveInstrumentPanel(instrumentDirectoryName, panelFileName, true);
        }

        return true;
    }

    //-------------------------------------------------------------------------
    // Load selected Instrument Panel from Disk
    // https://www.includehelp.com/code-snippets/cpp-program-to-write-and-read-an-object-in-from-a-binary-file.aspx
    // https://forum.juce.com/t/example-for-creating-a-file-and-doing-something-with-it/31998/2
    // https://forum.juce.com/t/how-to-get-path-from-working-directory-and-use-it/3932
    //  String filePath = File::getCurrentWorkingDirectory().getFullPathName(); 
    //  myFile = new File(filePath + T("temp.txt"));
    // https://forum.juce.com/t/getting-a-path-to-my-running-app/38025/21
    //  File::getSpecialLocation (File::SpecialLocationType::currentExecutableFile).getSiblingFile ("MyData")
    //        .getChildFile ("MySpecialFile").getFullPathName();
    //-------------------------------------------------------------------------
    bool loadInstrumentPanel(String instrumentDirectoryName, const String& panelFileName, bool bfullpath) {

        DBG("*** loadInstrumentPanel(): Loading Instrument Panel from disk");

        //ValueTree vtinstrumentpanel = createVTInstrumentPanel(panelvoicebuttons);

        {   // Scoping
            // Reload Value tree to test
            // Prepare to read Instrument Panel to Disk
            File inputFile;

            if (!bfullpath) {
                const auto organRoot = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    .getChildFile(organdir);
                const auto canonicalPanelDir = organRoot.getChildFile(appState.paneldir);
                const auto canonicalPath = canonicalPanelDir.getChildFile(panelFileName);

                // Prefer canonical panel location under Documents/AMidiOrgan/panels.
                inputFile = canonicalPath;

                // Legacy: panel files previously stored under instruments/
                if (!inputFile.existsAsFile())
                    inputFile = organRoot.getChildFile(appState.instrumentdir).getChildFile(panelFileName);

                // Legacy fallback: module-named folder under org root.
                if (!inputFile.existsAsFile() && instrumentDirectoryName.isNotEmpty())
                    inputFile = organRoot.getChildFile(instrumentDirectoryName).getChildFile(panelFileName);

                // Legacy fallback: panel file directly under org root.
                if (!inputFile.existsAsFile())
                    inputFile = organRoot.getChildFile(panelFileName);

                // Keep future saves on the canonical path regardless of where we loaded from.
                appState.panelfullpathname = canonicalPath.getFullPathName();
            } else {
                inputFile = File(panelFileName);
            }

            if (!inputFile.existsAsFile())
            {
                juce::Logger::writeToLog("loadInstrumentPanel(): Panel file " + panelFileName + " does not exists! We will revert to the Master Panel File");

                // Final fallback for master panel in canonical panels directory.
                inputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    .getChildFile(organdir)
                    .getChildFile(appState.paneldir)
                    .getChildFile(panelFileName);
            }

            FileInputStream input(inputFile);
            if (input.failedToOpen())
            {
                juce::Logger::writeToLog("loadInstrumentPanel(): Panel file " + panelFileName + " did not open! Need to recover from this...");

                // To do: Add some other error handling
                return false;
            }
            else
            {
                static Identifier instrumentpanelType("InstrumentPanel");   // Pre-create an Identifier
                ValueTree temp_vtinstrumentpanel(instrumentpanelType);

                // static ValueTree ValueTree::readFromStream(InputStream & input)
                temp_vtinstrumentpanel = temp_vtinstrumentpanel.readFromStream(input);

                if ((!temp_vtinstrumentpanel.isValid()) | !(temp_vtinstrumentpanel.getNumChildren() > 0))
                {
                    juce::Logger::writeToLog("loadInstrumentPanel(): Panel file read into temp ValueTree failed! No change to panel");

                    // To do: Add some other error handling
                    return false;
                }
                else
                    vtinstrumentpanel = temp_vtinstrumentpanel;

                // Temp check of contents
                //int nodescount = vtinstrumentpanel.getNumProperties();
                //int instrumentcount = vtinstrumentpanel.getNumChildren();

                loadVTInstrumentPanel();

                syncButtonGroupRotaryFromActivePreset();

                if (gNotifyManualRotarySyncFromAppState)
                    gNotifyManualRotarySyncFromAppState();
            }
        }

        setPanelUpdated(true);

        return true;
    }

    //-------------------------------------------------------------------------
    // https://forum.juce.com/t/example-for-creating-a-file-and-doing-something-with-it/31998
    // Save current Instrument Panel to Disk
    //-------------------------------------------------------------------------
    bool saveInstrumentPanel(String instrumentDirectoryName, const String& panelFileName, bool allowPairingMismatchSave = false) {

        juce::Logger::writeToLog("*** saveInstrumentPanel(): Saving Instrument Panel " + panelFileName);

        if (appState.configPanelPairingMismatchAcknowledged && !allowPairingMismatchSave)
        {
            juce::Logger::writeToLog("saveInstrumentPanel(): Blocked (config/panel pairing mismatch not confirmed for save)");
            return false;
        }

        {   // Scoping
            // Prepare to save Instrument Panel to Disk
            juce::ignoreUnused(instrumentDirectoryName);
            File outputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                .getChildFile(organdir)
                .getChildFile(appState.paneldir)
                .getChildFile(panelFileName);

            if (!outputFile.getParentDirectory().exists() && !outputFile.getParentDirectory().createDirectory())
            {
                juce::Logger::writeToLog("saveInstrumentPanel(): Failed to create panel directory " + outputFile.getParentDirectory().getFullPathName());
                return false;
            }

            if (outputFile.existsAsFile())
            {
                DBG("saveInstrumentPanel(): Panel file exists! Delete to store new copy");

                outputFile.deleteFile();
            }
            FileOutputStream output(outputFile);

            vtinstrumentpanel = createVTInstrumentPanel();

            //void ValueTree::writeToStream(OutputStream & output)	const
            vtinstrumentpanel.writeToStream(output);
            output.flush();
            if (output.getStatus().failed())
            {
                juce::Logger::writeToLog("saveInstrumentPanel(): An error occurred in FileOutputStream " + panelFileName);

                // To do: Add more error handling
                return false;
            }
        }

        appState.panelfname = panelFileName;
        appState.panelfullpathname = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(appState.paneldir)
            .getChildFile(panelFileName)
            .getFullPathName();

        appState.pnlconfigfname = appState.configfname;
        clearConfigPanelPairingMismatchIfAligned(appState);

        if (gNotifyPanelSaveAvailabilityChanged)
            gNotifyPanelSaveAvailabilityChanged();

        return true;
    }

    // Save Panel with Full Path Name
    bool saveInstrumentPanel(const String& panelfullpathfname, bool allowPairingMismatchSave = false) {

        juce::Logger::writeToLog("*** saveInstrumentPanel(): Saving Instrument Panel " + panelfullpathfname);

        if (appState.configPanelPairingMismatchAcknowledged && !allowPairingMismatchSave)
        {
            juce::Logger::writeToLog("saveInstrumentPanel(): Blocked (config/panel pairing mismatch not confirmed for save)");
            return false;
        }

        File outputFile;
        {   // Scoping
            // Prepare to save Instrument Panel to Disk
            outputFile = File(panelfullpathfname);
            if (panelfullpathfname.isEmpty())
            {
                outputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    .getChildFile(organdir)
                    .getChildFile(appState.paneldir)
                    .getChildFile(appState.panelfname);
            }

            if (!outputFile.getParentDirectory().exists() && !outputFile.getParentDirectory().createDirectory())
            {
                juce::Logger::writeToLog("saveInstrumentPanel(): Failed to create panel directory " + outputFile.getParentDirectory().getFullPathName());
                return false;
            }

            if (outputFile.existsAsFile())
            {
                DBG("saveInstrumentPanel(): Panel file exists! Delete to store new copy");

                outputFile.deleteFile();
            }
            FileOutputStream output(outputFile);

            vtinstrumentpanel = createVTInstrumentPanel();

            //void ValueTree::writeToStream(OutputStream & output)	const
            vtinstrumentpanel.writeToStream(output);
            output.flush();
            if (output.getStatus().failed())
            {
                juce::Logger::writeToLog("saveInstrumentPanel(): An error occurred in FileOutputStream " + panelfullpathfname);

                // To do: Add more error handling
                return false;
            }
        }

        appState.panelfname = outputFile.getFileName();
        appState.panelfullpathname = outputFile.getFullPathName();

        appState.pnlconfigfname = appState.configfname;
        clearConfigPanelPairingMismatchIfAligned(appState);

        if (gNotifyPanelSaveAvailabilityChanged)
            gNotifyPanelSaveAvailabilityChanged();

        return true;
    }

    // Track Panel updates in order to refresh Voice Button Text Voices
    bool getPanelUpdated() {
        return bpanelupdated;
    }

    void setPanelUpdated(bool bupdated) {
        bpanelupdated = bupdated;
    }

    // For direct access 
    PanelPresets* panelpresets = nullptr;

    juce_DeclareSingleton(InstrumentPanel, true)

private:
    MidiDevices* mididevices = nullptr;
    MidiInstruments* midiinstruments = nullptr;
    AppState& appState;

    int panelbuttonidx = 0;
    bool bpanelupdated = false;

    OwnedArray<VoiceButton> panelvoicebuttons;

    ValueTree vtinstrumentpanel;

    //-------------------------------------------------------------------------
    // Buttons Groups Upper, Lower and Keyboard Manuals

    // Bass Keyboard Panel
    ButtonGroup bassgroup1{ KBDBASS, "Organ", 12, 1, 1 , false };
    ButtonGroup bassgroup2{ KBDBASS, "Orchestral", 8, 1, 2, true };
    ButtonGroup bassgroup3{ KBDBASS, "Drums 1", 6, 10, 10, true };
    ButtonGroup bassgroup4{ KBDBASS, "Drums 2", 6, 10, 11, true };

    // Lower Keyboard Panel
    ButtonGroup lowergroup1{ KBDLOWER, "Organ", 12, 3, 3, false };
    ButtonGroup lowergroup2{ KBDLOWER, "Orchestral", 8, 3, 5, true };
    ButtonGroup lowergroup3{ KBDLOWER, "Symphonic", 6, 3, 6, true };
    ButtonGroup lowergroup4{ KBDLOWER, "Solo", 6, 5, 7, true };

    // Upper Keyboard Panel
    ButtonGroup uppergroup1{ KBDUPPER, "Organ", 12, 4, 4, false };
    ButtonGroup uppergroup2{ KBDUPPER, "Orchestral", 8, 4, 8, true };
    ButtonGroup uppergroup3{ KBDUPPER, "Symphonic", 6, 4, 9, true };
    ButtonGroup uppergroup4{ KBDUPPER, "Solo", 6, 5, 12, true };

    OwnedArray<ButtonGroup>buttongroups{&uppergroup1, &uppergroup2, &uppergroup3, &uppergroup4,
        &lowergroup1, &lowergroup2, &lowergroup3, &lowergroup4,
        &bassgroup1, &bassgroup2, &bassgroup3,& bassgroup4 };

    //-------------------------------------------------------------------------
    // Constructor made private
    InstrumentPanel() :
        mididevices(MidiDevices::getInstance()), // Creates the singleton if there isn't already one
        midiinstruments(MidiInstruments::getInstance()),
        panelpresets(PanelPresets::getInstance()),
        appState(getAppState())
    {
        juce::Logger::writeToLog("=S= InstrumentPanel(): Constructor " + std::to_string(zinstcntInstrumentPanel++));

        // Instrument ValueTree
        static Identifier instrumentpanelType("InstrumentPanel");   // Pre-create an Identifier
        vtinstrumentpanel = ValueTree(instrumentpanelType);
    }

    //-------------------------------------------------------------------------
    // https://docs.juce.com/master/classValueTree.html#a93d639299ef9dfedc651544e05f06693
    // https://cpp.hotexamples.com/examples/-/ValueTree/-/cpp-valuetree-class-examples.html
    //-------------------------------------------------------------------------
    ValueTree createVTInstrumentPanel()
    {
        DBG("createVTInstrumentPanel(): Creating Value Tree to Save to Disk...");

        static Identifier instrumentpanelType("InstrumentPanel");   // Pre-create an Identifier
        static Identifier instrumentType("Instrument");

        static Identifier filenameType("filename");
        static Identifier vendorType("vendor");
        static Identifier configfilenameType("configfilename");

        static Identifier svoiceType("svoice");
        static Identifier msbType("ccmsb");
        static Identifier lsbType("cclsb");
        static Identifier fontType("ccfont");

        static Identifier volType("ccvol");
        static Identifier expType("ccexp");
        static Identifier revType("ccrev");
        static Identifier choType("ccho");
        static Identifier modType("ccmod");
        static Identifier timType("cctim");
        static Identifier atkType("ccatk");
        static Identifier relType("ccrel");
        static Identifier briType("ccbri");
        static Identifier panType("ccpan");

        static Identifier dirtyType("isdirty");
        static Identifier buttonidxType("buttonidx");

        // Instrument ValueTree
        ValueTree panelTree(instrumentpanelType);

        panelTree.setProperty(filenameType, appState.panelfname, nullptr);
        panelTree.setProperty(vendorType, vendorname, nullptr);
        panelTree.setProperty(configfilenameType, appState.configfname, nullptr);

        static const juce::Identifier manualRotaryUpperFastId("manualRotaryUpperFast");
        static const juce::Identifier manualRotaryUpperBrakeId("manualRotaryUpperBrake");
        static const juce::Identifier manualRotaryLowerFastId("manualRotaryLowerFast");
        static const juce::Identifier manualRotaryLowerBrakeId("manualRotaryLowerBrake");
        panelTree.setProperty(manualRotaryUpperFastId, appState.upperManualRotaryFast, nullptr);
        panelTree.setProperty(manualRotaryUpperBrakeId, appState.upperManualRotaryBrake, nullptr);
        panelTree.setProperty(manualRotaryLowerFastId, appState.lowerManualRotaryFast, nullptr);
        panelTree.setProperty(manualRotaryLowerBrakeId, appState.lowerManualRotaryBrake, nullptr);

        // Add VoiceButton Instruments from Panel to ValueTree
        for (int i = 0; i < numbervoicebuttons; i++)
        {
            // Add Instrument on Button to ValueTree
            ////Instrument instrument = panelvoicebuttons[i].getInstrument();
            ////VoiceButton* pvb = new VoiceButton();
            VoiceButton* pvb = (VoiceButton*)panelvoicebuttons.getUnchecked(i);

            Instrument instrument = pvb->getInstrument();

            ValueTree ins(instrumentType); 

            ins.setProperty(buttonidxType, instrument.getButtonIdx(), nullptr);

            ins.setProperty(svoiceType, instrument.getVoice(), nullptr);
            ins.setProperty(msbType, instrument.getMSB(), nullptr);
            ins.setProperty(lsbType, instrument.getLSB(), nullptr);
            ins.setProperty(fontType, instrument.getFont(), nullptr);

            ins.setProperty(volType, instrument.getVol(), nullptr);
            ins.setProperty(expType, instrument.getExp(), nullptr);
            ins.setProperty(revType, instrument.getRev(), nullptr);
            ins.setProperty(choType, instrument.getCho(), nullptr);
            ins.setProperty(modType, instrument.getMod(), nullptr);
            ins.setProperty(timType, instrument.getTim(), nullptr);
            ins.setProperty(atkType, instrument.getAtk(), nullptr);
            ins.setProperty(relType, instrument.getRel(), nullptr);
            ins.setProperty(briType, instrument.getBri(), nullptr);
            ins.setProperty(panType, instrument.getPan(), nullptr);

            ins.setProperty(dirtyType, instrument.getDirty(), nullptr);

            if (ins.isValid()) {
                panelTree.appendChild(ins, nullptr);
            }
            else {
                DBG("createVTInstrumentPanel(): An error occurred while creating Instrument Panel ValueTree item: " + std::to_string(i));
                // To do: Add more error handling
            }
        }

        // Add all Presets for the current Instrument Panel
        static Identifier presetsType("Presets");
        ValueTree vtpresets(presetsType);

        for (int i = 0; i < numberpresets; i++) {

            static Identifier presetType("Preset");
            ValueTree vtpreset(presetType);

            String presetno = "preset" + std::to_string(i);
            vtpreset.setProperty(presetno, i, nullptr);

            // Add Button Group Details
            for (int j = 0; j < numberbuttongroups; j++) {

                static Identifier buttonGroupType("ButtonGroup");
                ValueTree vtbuttongroup(buttonGroupType);

                String buttongroupno = "buttongroup" + std::to_string(j);
                vtbuttongroup.setProperty(buttongroupno, j, nullptr);

                int pbtnidx = panelpresets->getPanelButtonIdx(i, j);
                vtbuttongroup.setProperty("button", pbtnidx, nullptr);

                bool bmute = panelpresets->getMuteStatus(i, j);
                vtbuttongroup.setProperty("mute", bmute, nullptr);

                int irotary = panelpresets->getRotaryStatus(i, j);
                vtbuttongroup.setProperty("rotary", irotary, nullptr);

                DBG("createVTInstrumentPanel(): Preset: " + std::to_string(i) + " ButtonGroup: "
                    + std::to_string(j) + " Button: " + std::to_string(pbtnidx));

                if (vtbuttongroup.isValid()) {
                    vtpreset.appendChild(vtbuttongroup, nullptr);
                }
                else {
                    DBG("createVTInstrumentPanel(): An error occurred while creating Instrument Panel ValueTree item: Button Group " + std::to_string(j));
                    // To do: Add more error handling
                }
            }

            if (vtpreset.isValid()) {
                vtpresets.appendChild(vtpreset, nullptr);
            }
            else {
                DBG("createVTInstrumentPanel(): An error occurred while creating Instrument Panel ValueTree item: Preset " + std::to_string(i));
                // To do: Add more error handling
            }
        }

        if (vtpresets.isValid()) {
            panelTree.appendChild(vtpresets, nullptr);
        }
        else {
            DBG("createVTInstrumentPanel(): An error occurred while creating Instrument Panel ValueTree item: Presets");
            // To do: Add more error handling
        }

        DBG("*** createVTInstrumentPanel(): Instrument Panel ValueTree created!");
        return panelTree;
    }

    //-------------------------------------------------------------------------
    // Demo Instrument Panel Load from known ValueTree
    //-------------------------------------------------------------------------
    bool loadVTInstrumentPanel()
    {
        DBG("loadVTInstrumentPanel(): Loading saved Value Tree from Disk...");

        if (!vtinstrumentpanel.isValid())
            return false;

        static Identifier instrumentType("Instrument");

        static Identifier filenameType("filename");
        static Identifier vendorType("vendor");
        static Identifier configfilenameType("configfilename");

        static Identifier svoiceType("svoice");
        static Identifier msbType("ccmsb");
        static Identifier lsbType("cclsb");
        static Identifier fontType("ccfont");

        static Identifier volType("ccvol");
        static Identifier expType("ccexp");
        static Identifier revType("ccrev");
        static Identifier choType("ccho");
        static Identifier modType("ccmod");
        static Identifier timType("cctim");
        static Identifier atkType("ccatk");
        static Identifier relType("ccrel");
        static Identifier briType("ccbri");
        static Identifier panType("ccpan");

        static Identifier dirtyType("isdirty");
        static Identifier buttonidxType("buttonidx");

        String filename = vtinstrumentpanel.getProperty(filenameType);
        String vendor = vtinstrumentpanel.getProperty(vendorType);
        appState.pnlconfigfname = vtinstrumentpanel.getProperty(configfilenameType);
        // Flag a Config reload needed based on config paramter in Panel File
        appState.configreload = (appState.configfname.compare(appState.pnlconfigfname) != 0);

        static const juce::Identifier manualRotaryUpperFastId("manualRotaryUpperFast");
        static const juce::Identifier manualRotaryUpperBrakeId("manualRotaryUpperBrake");
        static const juce::Identifier manualRotaryLowerFastId("manualRotaryLowerFast");
        static const juce::Identifier manualRotaryLowerBrakeId("manualRotaryLowerBrake");
        appState.upperManualRotaryFast = (bool) vtinstrumentpanel.getProperty(manualRotaryUpperFastId, true);
        appState.upperManualRotaryBrake = (bool) vtinstrumentpanel.getProperty(manualRotaryUpperBrakeId, false);
        appState.lowerManualRotaryFast = (bool) vtinstrumentpanel.getProperty(manualRotaryLowerFastId, true);
        appState.lowerManualRotaryBrake = (bool) vtinstrumentpanel.getProperty(manualRotaryLowerBrakeId, false);

        panelbuttonidx = 0;

        int i;  // i tracks all Instruments and then used to move on to Presets
        for (i = 0; i < numbervoicebuttons; ++i)
        {
            // Temporary instrument to read the nodes into
            Instrument instrument;

            // Get Instrument Node from ValueTree at Index
            ValueTree childNode = vtinstrumentpanel.getChild(i);

            if ((childNode.isValid()) && (childNode.getType() == instrumentType))
            {
                int ival = childNode.getProperty(buttonidxType);
                instrument.setButtonIdx(ival);

                String sval = childNode.getProperty(svoiceType);
                instrument.setVoice(sval);

                ival = childNode.getProperty(msbType);
                instrument.setMSB(ival);
                ival = childNode.getProperty(lsbType);
                instrument.setLSB(ival);
                ival = childNode.getProperty(fontType);
                instrument.setFont(ival);

                ival = childNode.getProperty(volType);
                instrument.setVol(childNode.getProperty(volType));
                ival = childNode.getProperty(expType);
                instrument.setExp(ival);

                ival = childNode.getProperty(revType);
                instrument.setRev(ival);
                ival = childNode.getProperty(choType);
                instrument.setCho(ival);

                ival = childNode.getProperty(modType);
                instrument.setMod(ival);
                ival = childNode.getProperty(timType);
                instrument.setTim(ival);

                ival = childNode.getProperty(atkType);
                instrument.setAtk(ival);
                ival = childNode.getProperty(relType);
                instrument.setRel(ival);

                ival = childNode.getProperty(briType);
                instrument.setBri(ival);
                ival = childNode.getProperty(panType);
                instrument.setPan(ival);

                ival = childNode.getProperty(dirtyType);
                instrument.setDirty(ival);
            }

            ////VoiceButton* pvb = panelvoicebuttons[i];
            VoiceButton* pvb = (VoiceButton*)panelvoicebuttons.getUnchecked(i);
            pvb->setInstrument(instrument);
            pvb->setButtonText(instrument.getVoice());

            // To do: Add button group indexes
            pvb->setButtonGroupId(1);
            pvb->setButtonId(i);
            pvb->setPanelButtonIdx(panelbuttonidx);

            // Increase, but do not reset between sound groups as it is the vector index for all button groups
            panelbuttonidx++;
        }

        // Load all Presets for this Instrument Panel
        // Note: Var i currently set to next node following all 96 Instruments loaded
        static Identifier presetsType("Presets");
        ValueTree vtpresets = vtinstrumentpanel.getChild(i);

        if ((vtpresets.isValid()) && (vtpresets.getType() == presetsType)) {

            for (int presetIdx = 0; presetIdx < numberpresets; presetIdx++) {

                static Identifier presetType("Preset");
                ValueTree vtpreset = vtpresets.getChild(presetIdx);
                if (!(vtpreset.isValid()) || !(vtpreset.getType() == presetType)) {
                    DBG("loadVTInstrumentPanel(): Failed to load Preset " + std::to_string(presetIdx));
                    continue;
                }

                String presetno = "preset" + std::to_string(presetIdx);
                String preset = vtpreset.getProperty(presetno);
                //DBG("loadVTInstrumentPanel(): Preset " + std::to_string(presetIdx));

                for (int j = 0; j < numberbuttongroups; j++) {

                    static Identifier buttongroupType("ButtonGroup");
                    ValueTree vtbuttongroup = vtpreset.getChild(j);
                    if (!(vtbuttongroup.isValid()) || !(vtbuttongroup.getType() == buttongroupType)) {
                        DBG("loadVTInstrumentPanel(): Failed to load Preset Button " + std::to_string(presetIdx) + ", "  + std::to_string(j));
                        continue;
                    }

                    String buttongroupno = "buttongroup" + std::to_string(j);
                    String buttongroup = vtbuttongroup.getProperty(buttongroupno);
                    //DBG("loadVTInstrumentPanel(): ButtonGroup " + buttongroup);

                    int pbtnidx = vtbuttongroup.getProperty("button");
                    DBG("loadVTInstrumentPanel(): Preset: " + std::to_string(presetIdx) + " ButtonGroup: "
                        + std::to_string(j) + " Button: " + std::to_string(pbtnidx));
                    panelpresets->setPanelButtonIdx(presetIdx, j, pbtnidx);

                    bool bmute = vtbuttongroup.getProperty("mute");
                    panelpresets->setMuteStatus(presetIdx, j, bmute);

                    int irotary = vtbuttongroup.getProperty("rotary");
                    panelpresets->setRotaryStatus(presetIdx, j, irotary);
                }
            }

        }
        else {
            DBG("loadVTInstrumentPanel(): An error occurred while loading Instrument Panel ValueTree item: No Presets");
            // To do: Add more error handling
        }

        DBG("*** loadVTInstrumentPanel(): Instrument Panel ValueTree created!");
        return true;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentPanel)
};
juce_ImplementSingleton(InstrumentPanel)


//=============================================================================
// Class: Instrument Voices Page
//=============================================================================
static int zinstcntvoices = 0;
class VoicesPage final : public Component
{
public:
    VoicesPage(TabbedComponent& tabs) :
        startTime(juce::Time::getMillisecondCounterHiRes() * 0.001),
        midiInstruments(MidiInstruments::getInstance()), // Creates the singleton if there isn't already one
        mididevices(MidiDevices::getInstance()),
        instrumentpanel(InstrumentPanel::getInstance())
    {
        juce::Logger::writeToLog("== VoicesPage(): Constructor " + std::to_string(zinstcntvoices++));

        //int bwidth = 150, bheight = 30;

        auto margin = 20;

        int xgroup = 220, ygroup = 10, gwidth = 1030, gheight = 260;
        group = addToList(new GroupComponent("group", "Voice Button Config"));
        group->setColour(GroupComponent::outlineColourId, Colours::grey.darker());
        group->setBounds(xgroup, ygroup, gwidth, gheight);

        selvoice = -1;
        selvoicebank = -1;

        lblskeyboard.setText("Keyboard:", {});
        addLabelAndSetStyle(lblskeyboard, true);
        lblskeyboard.setBounds(margin, margin, 150, 20);

        lblskeyboardtxt.setText("No Keyboard", {});
        addLabelAndSetStyle(lblskeyboardtxt, false);
        lblskeyboardtxt.setBounds(margin, margin + 10, 150, 50);

        lblsgroup.setText("Sound Group:", {});
        addLabelAndSetStyle(lblsgroup, true);
        lblsgroup.setBounds(margin, margin + 50, 150, 20);

        lblsgrouptxt.setText("No Group", {});
        addLabelAndSetStyle(lblsgrouptxt, false);
        lblsgrouptxt.setBounds(margin, margin + 60, 160, 50);

        lblsvoice.setText("Button Voice:", {});
        addLabelAndSetStyle(lblsvoice, true);
        lblsvoice.setBounds(margin, margin + 100, 150, 20);

        lblsvoicetxt.setText("No Voice", {});
        addLabelAndSetStyle(lblsvoicetxt, false);
        lblsvoicetxt.setBounds(margin, margin + 110, 160, 50);

        lblsvendornametxt.setText("Sound Filename", {});
        addLabelAndSetStyle(lblsvendornametxt, false);
        // Keep module/vendor context in the left column to free horizontal space for voice buttons.
        lblsvendornametxt.setBounds(margin, margin + 150, 180, 24);

        tbroute = addToList(new TextButton("To Upper"));
        tbroute->setColour(TextButton::textColourOffId, Colours::black);
        tbroute->setColour(TextButton::textColourOnId, Colours::black);
        tbroute->setColour(TextButton::buttonColourId, Colours::lightgrey);
        tbroute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);

        tbroute->setClickingTogglesState(false);
        tbroute->setBounds(margin, 225, 80, 30);
        tbroute->onClick = [=, &tabs]()
        {
            // Check if we still waiting for async voice select
            if ((selvoicebank < 0) || (selvoice < 0)) return;

            // Prepare to enable play testing of the selected instrument
            Instrument instrument;
            instrument.setMSB(midiInstruments->getMSB(selvoicebank, selvoice));
            instrument.setLSB(midiInstruments->getLSB(selvoicebank, selvoice));
            instrument.setFont(midiInstruments->getFont(selvoicebank, selvoice));
            instrument.setVoice(midiInstruments->getVoice(selvoicebank, selvoice));
            instrument.setChannel(mchannel);
            auto& st = getAppState();
            instrument.setVol(st.defaultEffectsVol);
            instrument.setBri(st.defaultEffectsBri);
            instrument.setExp(st.defaultEffectsExp);
            instrument.setRev(st.defaultEffectsRev);
            instrument.setCho(st.defaultEffectsCho);
            instrument.setMod(st.defaultEffectsMod);
            instrument.setTim(st.defaultEffectsTim);
            instrument.setAtk(st.defaultEffectsAtk);
            instrument.setRel(st.defaultEffectsRel);
            instrument.setPan(st.defaultEffectsPan);

            int controllerNumber = CCMSB; // MSB
            juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
                buttongroupmidiout,
                controllerNumber,
                instrument.getMSB()
            );
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            // Send the MIDI CC message to MIDI output(s)
            //mididevices->sendMessageNow(ccMessage);
            mididevices->sendToOutputs(ccMessage);

            controllerNumber = CCLSB;    // LSB
            ccMessage = juce::MidiMessage::controllerEvent(
                buttongroupmidiout,
                controllerNumber,
                instrument.getLSB()
            );
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);

            // Send PC
            ccMessage = juce::MidiMessage::programChange(
                buttongroupmidiout,
                instrument.getFont()
            );
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);

            // Used by Listener to trigger and update Button Text
            bvoiceupdated = true;

            // Populate the Voices Page with last Button Instrument pressed and switch to tab
            tabs.setCurrentTabIndex(currenttabidx, true);
            //String sname = tabs.getCurrentTabName();

            // Set the input Panel Button to the newly selected instrument
            VoiceButton* pvbtn = instrumentpanel->getVoiceButton(panelbuttonidx);
            pvbtn->setInstrument(instrument);

            // Tab redraw: https://forum.juce.com/t/how-to-capture-tabbedcomponents-tab-active/29203/9
            pvbtn->setButtonText(instrument.getVoice());
            pvbtn->triggerClick();

            syncVoiceSnapshotFromInstrument();
            selvoice = -1;
            selvoicebank = -1;
            updateVoicesRouteButtonDirtyStyle();
        };
        // Disable the button until Voice or Effects button selected in Keyboard Panels
        tbroute->setEnabled(false);

        tbcancel = addToList(new TextButton("Cancel"));
        tbcancel->setColour(TextButton::textColourOffId, Colours::black);
        tbcancel->setColour(TextButton::textColourOnId, Colours::black);
        tbcancel->setColour(TextButton::buttonColourId, Colours::lightgrey);
        tbcancel->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);

        tbcancel->setClickingTogglesState(false);
        tbcancel->setBounds(margin + 100, 225, 80, 30);
        tbcancel->onClick = [=, &tabs]()
            {
                // Populate the Effects Page with last Button Instrument pressed and switch to tab
                tabs.setCurrentTabIndex(currenttabidx, true);
                //String sname = tabs.getCurrentTabName();

                // Tab redraw: https://forum.juce.com/t/how-to-capture-tabbedcomponents-tab-active/29203/9
                VoiceButton* pvbtn = instrumentpanel->getVoiceButton(panelbuttonidx);
                pvbtn->triggerClick();

                lblsvoicetxt.setText(pvbtn->getInstrument().getVoice(), {});
                syncVoiceSnapshotFromInstrument();
                selvoice = -1;
                selvoicebank = -1;
                updateVoicesRouteButtonDirtyStyle();
            };

        addAndMakeVisible(voiceBrowserGroup);
        voiceBrowserGroup.setText("Voice Categories");
        voiceBrowserGroup.setColour(GroupComponent::outlineColourId, Colours::grey.darker());
        voiceBrowserGroup.setBounds(margin + 220, 35, 920, 185);

        addAndMakeVisible(browserBackButton);
        browserBackButton.setButtonText("Back");
        browserBackButton.setColour(TextButton::textColourOffId, Colours::black);
        browserBackButton.setColour(TextButton::textColourOnId, Colours::black);
        browserBackButton.setColour(TextButton::buttonColourId, Colours::lightgrey);
        browserBackButton.setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
        browserBackButton.setBounds(margin + 220, 225, 80, 30);
        browserBackButton.onClick = [this]()
        {
            browserLevel = BrowserLevel::categories;
            browserPage = 0;
            renderVoiceBrowser();
        };

        addAndMakeVisible(browserPrevButton);
        browserPrevButton.setButtonText("Prev");
        browserPrevButton.setColour(TextButton::textColourOffId, Colours::black);
        browserPrevButton.setColour(TextButton::textColourOnId, Colours::black);
        browserPrevButton.setColour(TextButton::buttonColourId, Colours::lightgrey);
        browserPrevButton.setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
        browserPrevButton.setBounds(margin + 960, 225, 80, 30);
        browserPrevButton.onClick = [this]()
        {
            if (browserPage > 0)
            {
                --browserPage;
                renderVoiceBrowser();
            }
        };

        addAndMakeVisible(browserNextButton);
        browserNextButton.setButtonText("Next");
        browserNextButton.setColour(TextButton::textColourOffId, Colours::black);
        browserNextButton.setColour(TextButton::textColourOnId, Colours::black);
        browserNextButton.setColour(TextButton::buttonColourId, Colours::lightgrey);
        browserNextButton.setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
        browserNextButton.setBounds(margin + 1060, 225, 80, 30);
        browserNextButton.onClick = [this]()
        {
            const int pageCount = getBrowserPageCount();
            if (browserPage + 1 < pageCount)
            {
                ++browserPage;
                renderVoiceBrowser();
            }
        };

        addAndMakeVisible(browserPageLabel);
        browserPageLabel.setJustificationType(juce::Justification::centred);
        browserPageLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        browserPageLabel.setBounds(margin + 840, 225, 110, 30);

        setVoiceBrowserInteractive(false);
    }

    //-------------------------------------------------------------------------
    // Entry into Voice Panel: Parameters for last panel button pressed
    bool setPanelButton(int panelbtnidx, String sgroup, String sfilename, int midiout, int tabindex) {

        panelbuttonidx = panelbtnidx;
        currenttabidx = tabindex;
        buttongroupmidiout = midiout;

        {
            if (currenttabidx == PTLower)
                group->setColour(GroupComponent::outlineColourId, Colours::palegreen.darker());
            else if (currenttabidx == PTBass)
                group->setColour(GroupComponent::outlineColourId, Colours::antiquewhite.darker());
            else
                group->setColour(GroupComponent::outlineColourId, Colours::palevioletred.darker());
        }

        lblsgrouptxt.setText(sgroup, {});

        String totabtext;
        if (currenttabidx == PTLower) {
            totabtext = "To Lower";
            lblskeyboardtxt.setText("Lower", {});
        }
        else if (currenttabidx == PTBass) {
            totabtext = "To Bass";
            lblskeyboardtxt.setText("Bass&Drums", {});
        }
        else {
            totabtext = "To Upper";
            lblskeyboardtxt.setText("Upper", {});
        }
        tbroute->setButtonText(totabtext);

        ptrvoicebutton = instrumentpanel->getVoiceButton(panelbuttonidx);
        String svoice1 = ptrvoicebutton->getInstrument().getVoice();
        lblsvoicetxt.setText(svoice1, {});

        // Load JSON Instrument Sound File for current Voice Button Group if not loaded
        if ((sfilename != instrumentfname) && (sfilename != "")) {
            instrumentfname = sfilename;

            midiInstruments->loadMidiInstruments(instrumentfname);
        }
        lblsvendornametxt.setText(midiInstruments->getVendor(), {});

        setVoiceBrowserInteractive(true);
        browserLevel = BrowserLevel::categories;
        selectedCategoryIdx = -1;
        browserPage = 0;
        renderVoiceBrowser();

        syncVoiceSnapshotFromInstrument();
        selvoice = -1;
        selvoicebank = -1;
        updateVoicesRouteButtonDirtyStyle();
        hasPanelContextFlag = true;

        return true;
    }

    bool hasPanelContext() const { return hasPanelContextFlag; }

    //-------------------------------------------------------------------------
    ~VoicesPage() {
        DBG("=== VoicesPage(): Destructor " + std::to_string(--zinstcntvoices));
    }

private:
    // Volume bar with fixed stacked gradient (bottom stays green as level rises).
    class VolumeGradientSlider final : public Slider
    {
    public:
        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            auto inner = bounds.reduced(2.0f);

            g.setColour(juce::Colours::black.withAlpha(0.25f));
            g.fillRoundedRectangle(bounds, 2.5f);

            const auto min = (float)getMinimum();
            const auto max = (float)getMaximum();
            const auto value = (float)getValue();
            const float value01 = (max > min) ? juce::jlimit(0.0f, 1.0f, (value - min) / (max - min)) : 0.0f;

            const float filledHeight = inner.getHeight() * value01;
            auto fillArea = juce::Rectangle<float>(
                inner.getX(),
                inner.getBottom() - filledHeight,
                inner.getWidth(),
                filledHeight);

            juce::ColourGradient gradient(
                juce::Colours::green.darker(), inner.getCentreX(), inner.getBottom(),
                juce::Colours::red.darker(), inner.getCentreX(), inner.getY(),
                false);
            gradient.addColour(0.65, juce::Colours::orange.darker());

            g.setGradientFill(gradient);
            g.fillRect(fillArea);

            g.setColour(juce::Colours::black.withAlpha(0.55f));
            g.drawRect(inner, 1.0f);

            if (!isEnabled())
            {
                g.setColour(juce::Colours::black.withAlpha(0.35f));
                g.fillRect(inner);
            }
        }
    };

    OwnedArray<Component> components;

    double startTime;

    MidiDevices* mididevices = nullptr;
    InstrumentPanel* instrumentpanel = nullptr;
    MidiInstruments* midiInstruments = nullptr;

    GroupComponent* group;

    TextButton* tbroute;
    TextButton* tbcancel;
    VoiceButton* ptrvoicebutton;
    Instrument midinstr;
    int buttongroupmidiout = 1;

    int currenttabidx = 0;
    int panelbuttonidx = 0;

    int mchannel = 1;
    int selvoice = 0;
    int selvoicebank = 0;
    bool hasPanelContextFlag = false;

    /** MSB/LSB/PC snapshot from the voice button when Sounds tab opens; menu pick compares to these. */
    int snapMSB = 0;
    int snapLSB = 0;
    int snapFont = 0;

    void syncVoiceSnapshotFromInstrument()
    {
        if (ptrvoicebutton == nullptr)
            return;

        Instrument inst = ptrvoicebutton->getInstrument();
        snapMSB = inst.getMSB();
        snapLSB = inst.getLSB();
        snapFont = inst.getFont();
    }

    bool voicesSelectionDiffersFromSnapshot()
    {
        if (midiInstruments == nullptr)
            return false;
        if (selvoicebank < 0 || selvoice < 0)
            return false;

        return midiInstruments->getMSB(selvoicebank, selvoice) != snapMSB
            || midiInstruments->getLSB(selvoicebank, selvoice) != snapLSB
            || midiInstruments->getFont(selvoicebank, selvoice) != snapFont;
    }

    /** Same red “pending” style as panel Save and Effects tab To Upper. */
    void updateVoicesRouteButtonDirtyStyle()
    {
        if (tbroute == nullptr)
            return;

        if (!tbroute->isEnabled())
        {
            tbroute->setColour(TextButton::textColourOffId, Colours::black);
            tbroute->setColour(TextButton::textColourOnId, Colours::black);
            tbroute->setColour(TextButton::buttonColourId, Colours::lightgrey);
            tbroute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            return;
        }

        if (voicesSelectionDiffersFromSnapshot())
        {
            tbroute->setColour(TextButton::textColourOffId, Colours::white);
            tbroute->setColour(TextButton::textColourOnId, Colours::white);
            tbroute->setColour(TextButton::buttonColourId, Colours::darkred);
            tbroute->setColour(TextButton::buttonOnColourId, Colours::darkred.brighter());
        }
        else
        {
            tbroute->setColour(TextButton::textColourOffId, Colours::black);
            tbroute->setColour(TextButton::textColourOnId, Colours::black);
            tbroute->setColour(TextButton::buttonColourId, Colours::lightgrey);
            tbroute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
        }
    }

    juce::Label lblskeyboard{ "Keyboard" };
    juce::Label lblskeyboardtxt{ "No Keyboard" };
    juce::Label lblsgroup{ "Sound Group" };
    juce::Label lblsgrouptxt{ "No Group" };
    juce::Label lblsvoice{ "Button Voice" };
    juce::Label lblsvoicetxt{ "No Voice" };
    juce::Label lblsvendornametxt{ "Sound File" };
    juce::GroupComponent voiceBrowserGroup{ "voiceBrowser", "Voice Categories" };
    juce::TextButton browserBackButton{ "Back" };
    juce::TextButton browserPrevButton{ "Prev" };
    juce::TextButton browserNextButton{ "Next" };
    juce::Label browserPageLabel{ "browserPageLabel", "" };
    juce::OwnedArray<juce::TextButton> browserButtons;

    enum class BrowserLevel
    {
        categories,
        voices
    };
    BrowserLevel browserLevel = BrowserLevel::categories;
    int selectedCategoryIdx = -1;
    int browserPage = 0;
    bool browserInteractive = false;

    void resized() override
    {
        renderVoiceBrowser();
    }

    void setVoiceBrowserInteractive(bool enabled)
    {
        browserInteractive = enabled;
        voiceBrowserGroup.setEnabled(enabled);
        browserBackButton.setEnabled(enabled);
        browserPrevButton.setEnabled(enabled);
        browserNextButton.setEnabled(enabled);
        if (!enabled)
            browserPageLabel.setText("", dontSendNotification);

        renderVoiceBrowser();
    }

    int getBrowserItemCount() const
    {
        if (midiInstruments == nullptr)
            return 0;

        if (browserLevel == BrowserLevel::categories)
            return midiInstruments->getCategoryCount();

        if (selectedCategoryIdx < 0)
            return 0;

        return midiInstruments->getCategoryVoiceCount(selectedCategoryIdx);
    }

    int getButtonsPerPage() const
    {
        constexpr int cols = 6;
        constexpr int rows = 4;
        return cols * rows;
    }

    int getBrowserPageCount() const
    {
        const int count = getBrowserItemCount();
        const int perPage = juce::jmax(1, getButtonsPerPage());
        return juce::jmax(1, (count + perPage - 1) / perPage);
    }

    void clearBrowserButtons()
    {
        for (auto* b : browserButtons)
            removeChildComponent(b);
        browserButtons.clear(true);
    }

    void sendPreviewMidiForSelection(int categoryIdx, int voiceIdx)
    {
        int controllerNumber = CCMSB; // MSB
        juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
            buttongroupmidiout,
            controllerNumber,
            midiInstruments->getMSB(categoryIdx, voiceIdx)
        );
        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
        mididevices->sendToOutputs(ccMessage);

        controllerNumber = CCLSB;    // LSB
        ccMessage = juce::MidiMessage::controllerEvent(
            buttongroupmidiout,
            controllerNumber,
            midiInstruments->getLSB(categoryIdx, voiceIdx)
        );
        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
        mididevices->sendToOutputs(ccMessage);

        // Send PC
        ccMessage = juce::MidiMessage::programChange(
            buttongroupmidiout,
            midiInstruments->getFont(categoryIdx, voiceIdx)
        );
        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
        mididevices->sendToOutputs(ccMessage);
    }

    void renderVoiceBrowser()
    {
        clearBrowserButtons();

        if (!browserInteractive || midiInstruments == nullptr)
        {
            browserBackButton.setVisible(false);
            browserPrevButton.setVisible(false);
            browserNextButton.setVisible(false);
            browserPageLabel.setText("", dontSendNotification);
            return;
        }

        auto inner = voiceBrowserGroup.getBounds().reduced(10);
        inner.removeFromTop(20);
        constexpr int cols = 6;
        const int gapX = 8;
        const int gapY = 8;
        const int btnW = 143; // keep current visual button width
        const int btnH = 29;  // keep current visual button height
        const int perPage = juce::jmax(1, getButtonsPerPage());
        const int itemCount = getBrowserItemCount();
        const int pageCount = getBrowserPageCount();

        browserPage = juce::jlimit(0, juce::jmax(0, pageCount - 1), browserPage);
        const int pageStart = browserPage * perPage;
        const int pageEnd = juce::jmin(itemCount, pageStart + perPage);

        for (int item = pageStart; item < pageEnd; ++item)
        {
            const int local = item - pageStart;
            const int col = local % cols;
            const int row = local / cols;

            auto* b = browserButtons.add(new juce::TextButton());
            addAndMakeVisible(b);
            b->setColour(TextButton::textColourOffId, Colours::black);
            b->setColour(TextButton::textColourOnId, Colours::black);
            b->setColour(TextButton::buttonColourId, Colours::lightgrey);
            b->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            b->setBounds(inner.getX() + col * (btnW + gapX), inner.getY() + row * (btnH + gapY), btnW, btnH);

            if (browserLevel == BrowserLevel::categories)
            {
                const int categoryIdx = item;
                b->setButtonText(midiInstruments->getCategory(categoryIdx));
                b->onClick = [this, categoryIdx]()
                {
                    selectedCategoryIdx = categoryIdx;
                    browserLevel = BrowserLevel::voices;
                    browserPage = 0;
                    renderVoiceBrowser();
                };
            }
            else
            {
                const int voiceIdx = item + 1;
                b->setButtonText(midiInstruments->getVoice(selectedCategoryIdx, voiceIdx));
                b->onClick = [this, voiceIdx]()
                {
                    selvoicebank = selectedCategoryIdx;
                    selvoice = voiceIdx;

                    lblsvoicetxt.setText(midiInstruments->getVoice(selvoicebank, selvoice), {});
                    sendPreviewMidiForSelection(selvoicebank, selvoice);

                    if ((currenttabidx == PTUpper) || (currenttabidx == PTLower) || (currenttabidx == PTBass))
                        tbroute->setEnabled(true);
                    updateVoicesRouteButtonDirtyStyle();
                };
            }
        }

        browserBackButton.setVisible(browserLevel == BrowserLevel::voices);
        if (browserLevel == BrowserLevel::voices && selectedCategoryIdx >= 0)
            voiceBrowserGroup.setText("Category Voices: " + midiInstruments->getCategory(selectedCategoryIdx));
        else
            voiceBrowserGroup.setText("Voice Categories");
        browserPrevButton.setVisible(pageCount > 1);
        browserNextButton.setVisible(pageCount > 1);
        browserPrevButton.setEnabled(browserPage > 0);
        browserNextButton.setEnabled(browserPage + 1 < pageCount);
        browserPageLabel.setText(pageCount > 1
            ? ("Page " + juce::String(browserPage + 1) + "/" + juce::String(pageCount))
            : juce::String(), dontSendNotification);
    }

    void addLabelAndSetStyle(Label& label, bool heading)
    {
        if (heading == false)
            label.setFont(Font(FontOptions(16.00f, Font::plain)));
        else
            label.setFont(Font(FontOptions(16.00f, Font::bold)));

        label.setJustificationType(Justification::centredLeft);
        label.setEditable(false, false, false);
        label.setColour(TextEditor::textColourId, Colours::white);
        label.setColour(TextEditor::backgroundColourId, Colour(0x00000000));

        addAndMakeVisible(&label);
    }

    //-------------------------------------------------------------------------
    // This little function avoids a bit of code-duplication by adding a component to
    // our list as well as calling addAndMakeVisible on it..
    template <typename ComponentType>
    ComponentType* addToList(ComponentType* newComp)
    {
        components.add(newComp);
        addAndMakeVisible(newComp);
        return newComp;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoicesPage)
};


//=============================================================================
// Class: Instrument Effects Page
//=============================================================================
static int zinstcnteffects = 0;
class EffectsPage final : public Component
{
public:
    EffectsPage(TabbedComponent& tabs) :
        startTime(juce::Time::getMillisecondCounterHiRes() * 0.001),
        mididevices(MidiDevices::getInstance()), // Creates the singleton if there isn't already one
        instrumentpanel(InstrumentPanel::getInstance())
    {
        juce::Logger::writeToLog("== EffectsPage(): Constructor " + std::to_string(zinstcnteffects++));

        //int bwidth = 150, bheight = 30;

        getLookAndFeel().setColour(juce::Slider::thumbColourId, juce::Colours::darkred.brighter());

        int xgroup = 220, ygroup = 10, gwidth = 1030, gheight = 260;
        group = addToList(new GroupComponent("group", "Effects for Voice Button"));
        group->setColour(GroupComponent::outlineColourId, Colours::grey.darker());
        group->setBounds(xgroup, ygroup, gwidth, gheight);

        auto sleffectwidth = 200;
        auto sleffectheight = 120;
        auto soundwidth = 210;
        auto margin = 20;
        auto gmargin = 25;

        lblskeyboard.setText("Keyboard:", {});
        addLabelAndSetStyle(lblskeyboard, true);
        lblskeyboard.setBounds(margin, margin, 150, 20);

        lblskeyboardtxt.setText("No Keyboard", {});
        addLabelAndSetStyle(lblskeyboardtxt, false);
        lblskeyboardtxt.setBounds(margin, margin + 10, 150, 50);

        lblsgroup.setText("Sound Group:", {});
        addLabelAndSetStyle(lblsgroup, true);
        lblsgroup.setBounds(margin, margin + 50, 150, 20);

        lblsgrouptxt.setText("No Group", {});
        addLabelAndSetStyle(lblsgrouptxt, false);
        lblsgrouptxt.setBounds(margin, margin + 60, 160, 50);

        lblsvoice.setText("Button Voice:", {});
        addLabelAndSetStyle(lblsvoice, true);
        lblsvoice.setBounds(margin, margin + 100, 150, 20);

        lblsvoicetxt.setText("No Voice", {});
        addLabelAndSetStyle(lblsvoicetxt, false);
        lblsvoicetxt.setBounds(margin, margin + 110, 160, 50);

        slvol = addToList(createSlider(false));
        slvol->setSliderStyle(Slider::Rotary);
        slvol->setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, false);
        slvol->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
        slvol->setColour(Slider::ColourIds::rotarySliderFillColourId, juce::Colours::antiquewhite);
        slvol->setColour(Slider::ColourIds::rotarySliderOutlineColourId, juce::Colours::dimgrey);
        slvol->setBounds(soundwidth + gmargin, gmargin, sleffectwidth, sleffectheight);
        slvol->setTextValueSuffix(" Vol");
        slvol->onValueChange = [=]()
            {
                int sval = (int)slvol->getValue();
                changeEffect(panelbuttonidx, CCVol, sval);

                const int panelgroup = lookupPanelGroup(panelbuttonidx);
                auto* selectedButtonGroup = instrumentpanel->getButtonGroup(panelgroup);
                const int masterCc7 = sliderStepToMasterCc7(selectedButtonGroup->getMasterVolStep());
                const int effectiveCc7 = selectedButtonGroup->getMuteButtonStatus()
                    ? 0
                    : computeEffectiveVolumeCc7(masterCc7, sval, getAppState().defaultEffectsVol);

                // Compare against last sent group-level CC7, not raw per-voice Effects Vol.
                if (!selectedButtonGroup->isEffectDirty(0, effectiveCc7))
                    return;

                auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCVol, effectiveCc7);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            };

        slexp = addToList(createSlider(false));
        slexp->setSliderStyle(Slider::Rotary);
        slexp->setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, false);
        slexp->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
        slexp->setColour(Slider::ColourIds::rotarySliderFillColourId, juce::Colours::antiquewhite);
        slexp->setColour(Slider::ColourIds::rotarySliderOutlineColourId, juce::Colours::dimgrey);
        slexp->setBounds(soundwidth + gmargin, gmargin + sleffectheight, sleffectwidth, sleffectheight);
        slexp->setTextValueSuffix(" Exp");
        slexp->onValueChange = [=]()
            {
                int sval = (int)slexp->getValue();
                changeEffect(panelbuttonidx, CCExp, sval);

                auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCExp, sval);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            };

        slrev = addToList(createSlider(false));
        slrev->setSliderStyle(Slider::Rotary);
        slrev->setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, false);
        slrev->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
        slrev->setColour(Slider::ColourIds::rotarySliderFillColourId, juce::Colours::antiquewhite);
        slrev->setColour(Slider::ColourIds::rotarySliderOutlineColourId, juce::Colours::dimgrey);
        slrev->setBounds(soundwidth + gmargin + sleffectwidth, gmargin, sleffectwidth, sleffectheight);
        slrev->setTextValueSuffix(" Rev");
        slrev->onValueChange = [=]()
            {
                int sval = (int)slrev->getValue();
                changeEffect(panelbuttonidx, CCRev, sval);

                auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCRev, sval);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            };

        slcho = addToList(createSlider(false));
        slcho->setSliderStyle(Slider::Rotary);
        slcho->setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, false);
        slcho->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
        slcho->setColour(Slider::ColourIds::rotarySliderFillColourId, juce::Colours::antiquewhite);
        slcho->setColour(Slider::ColourIds::rotarySliderOutlineColourId, juce::Colours::dimgrey);
        slcho->setBounds(soundwidth + gmargin + sleffectwidth, gmargin + sleffectheight, sleffectwidth, sleffectheight);
        slcho->setTextValueSuffix(" Cho");
        slcho->onValueChange = [=]()
            {
                int sval = (int)slcho->getValue();
                changeEffect(panelbuttonidx, CCCho, sval);

                auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCCho, sval);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            };

        slmod = addToList(createSlider(false));
        slmod->setSliderStyle(Slider::Rotary);
        slmod->setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, false);
        slmod->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
        slmod->setColour(Slider::ColourIds::rotarySliderFillColourId, juce::Colours::antiquewhite);
        slmod->setColour(Slider::ColourIds::rotarySliderOutlineColourId, juce::Colours::dimgrey);
        slmod->setBounds(soundwidth + gmargin + 2 * sleffectwidth, gmargin, sleffectwidth, sleffectheight);
        slmod->setTextValueSuffix(" Mod");
        slmod->onValueChange = [=]()
            {
                int sval = (int)slmod->getValue();
                changeEffect(panelbuttonidx, CCMod, sval);

                auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCMod, sval);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            };

        sltim = addToList(createSlider(false));
        sltim->setSliderStyle(Slider::Rotary);
        sltim->setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, false);
        sltim->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
        sltim->setColour(Slider::ColourIds::rotarySliderFillColourId, juce::Colours::antiquewhite);
        sltim->setColour(Slider::ColourIds::rotarySliderOutlineColourId, juce::Colours::dimgrey);
        sltim->setBounds(soundwidth + gmargin + 2 * sleffectwidth, gmargin + sleffectheight, sleffectwidth, sleffectheight);
        sltim->setTextValueSuffix(" Tim");
        sltim->onValueChange = [=]()
            {
                int sval = (int)sltim->getValue();
                changeEffect(panelbuttonidx, CCTim, sval);

                auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCTim, sval);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            };

        slatk = addToList(createSlider(false));
        slatk->setSliderStyle(Slider::Rotary);
        slatk->setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, false);
        slatk->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
        slatk->setColour(Slider::ColourIds::rotarySliderFillColourId, juce::Colours::antiquewhite);
        slatk->setColour(Slider::ColourIds::rotarySliderOutlineColourId, juce::Colours::dimgrey);
        slatk->setBounds(soundwidth + gmargin + 3 * sleffectwidth, gmargin, sleffectwidth, sleffectheight);
        slatk->setTextValueSuffix(" Atk");
        slatk->onValueChange = [=]()
            {
                int sval = (int)slatk->getValue();
                changeEffect(panelbuttonidx, CCAtk, sval);

                auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCAtk, sval);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            };

        slrel = addToList(createSlider(false));
        slrel->setSliderStyle(Slider::Rotary);
        slrel->setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, false);
        slrel->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
        slrel->setColour(Slider::ColourIds::rotarySliderFillColourId, juce::Colours::antiquewhite);
        slrel->setColour(Slider::ColourIds::rotarySliderOutlineColourId, juce::Colours::dimgrey);
        slrel->setBounds(soundwidth + gmargin + 3 * sleffectwidth, gmargin + sleffectheight, sleffectwidth, sleffectheight);
        slrel->setTextValueSuffix(" Rel");
        slrel->onValueChange = [=]()
            {
                int sval = (int)slrel->getValue();
                changeEffect(panelbuttonidx, CCRel, sval);

                auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCRel, sval);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            };

        slbri = addToList(createSlider(false));
        slbri->setSliderStyle(Slider::Rotary);
        slbri->setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, false);
        slbri->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
        slbri->setColour(Slider::ColourIds::rotarySliderFillColourId, juce::Colours::antiquewhite);
        slbri->setColour(Slider::ColourIds::rotarySliderOutlineColourId, juce::Colours::dimgrey);
        slbri->setBounds(soundwidth + gmargin + 4 * sleffectwidth, gmargin, sleffectwidth, sleffectheight);
        slbri->setTextValueSuffix(" Bri");
        slbri->onValueChange = [=]()
            {
                int sval = (int)slbri->getValue();
                changeEffect(panelbuttonidx, CCBri, sval);

                auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCBri, sval);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            };

        slpan = addToList(createSlider(false));
        slpan->setSliderStyle(Slider::Rotary);
        slpan->setRotaryParameters(MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, false);
        slpan->setTextBoxStyle(Slider::TextBoxRight, false, 70, 20);
        slpan->setColour(Slider::ColourIds::rotarySliderFillColourId, juce::Colours::antiquewhite);
        slpan->setColour(Slider::ColourIds::rotarySliderOutlineColourId, juce::Colours::dimgrey);
        slpan->setBounds(soundwidth + gmargin + 4 * sleffectwidth, gmargin + sleffectheight, sleffectwidth, sleffectheight);
        slpan->setTextValueSuffix(" Pan");
        slpan->onValueChange = [=]()
            {
                int sval = (int)slpan->getValue();
                changeEffect(panelbuttonidx, CCPan, sval);

                auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCPan, sval);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            };

        tbroute = addToList(new TextButton("To Upper"));
        tbroute->setColour(TextButton::textColourOffId, Colours::black);
        tbroute->setColour(TextButton::textColourOnId, Colours::black);
        tbroute->setColour(TextButton::buttonColourId, Colours::lightgrey);
        tbroute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
        tbroute->setClickingTogglesState(false);
        tbroute->setBounds(margin, 2 * margin + 140, 80, 30);
        tbroute->onClick = [=, &tabs]()
            {
                // Treat current instrument as the new baseline (same as after setEffects).
                syncEffectsSnapshotFromInstrument();
                updateToRouteButtonDirtyStyle();

                // Used by Listener to trigger and update Button Text
                beffectsupdated = true;

                tabs.setCurrentTabIndex(currenttabidx, true);
                //String sname = tabs.getCurrentTabName();
            };
        // Disable the routing button until Voice or Effect has been selected
        tbroute->setEnabled(false);

        tbcancel = addToList(new TextButton("Cancel"));
        tbcancel->setColour(TextButton::textColourOffId, Colours::black);
        tbcancel->setColour(TextButton::textColourOnId, Colours::black);
        tbcancel->setColour(TextButton::buttonColourId, Colours::lightgrey);
        tbcancel->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);

        tbcancel->setClickingTogglesState(false);
        tbcancel->setBounds(margin + 100, 2 * margin + 140, 80, 30);
        tbcancel->onClick = [=, &tabs]()
            {
                // Populate the Effects Page with last Button Instrument pressed and switch to tab
                tabs.setCurrentTabIndex(currenttabidx, true);

                // Reset all effects back to original for Cancel
                cancelEffects(panelbuttonidx);

                // Tab redraw: https://forum.juce.com/t/how-to-capture-tabbedcomponents-tab-active/29203/9
                VoiceButton* ptrvoicebutton = instrumentpanel->getVoiceButton(panelbuttonidx);
                ptrvoicebutton->triggerClick();
            };
    }

    ~EffectsPage() {
        DBG("=== EffectsPage(): Destructor " + std::to_string(--zinstcnteffects));
    }

    //-----------------------------------------------------------------------------
    // Initialize effect controllers to current values on instrument
    bool setEffects(Instrument midinstr)
    {
        effvol = midinstr.getVol();
        slvol->setValue(effvol, dontSendNotification);

        effexp = midinstr.getExp();
        slexp->setValue(effexp, dontSendNotification);

        effrev = midinstr.getRev();
        slrev->setValue(effrev, dontSendNotification);

        effcho = midinstr.getCho();
        slcho->setValue(effcho, dontSendNotification);

        effmod= midinstr.getMod();
        slmod->setValue(effmod, dontSendNotification);

        efftim = midinstr.getTim();
        sltim->setValue(efftim, dontSendNotification);

        effatk = midinstr.getAtk();
        slatk->setValue(effatk, dontSendNotification);

        effrel = midinstr.getRel();
        slrel->setValue(effrel, dontSendNotification);

        effbri = midinstr.getBri();
        slbri->setValue(effbri, dontSendNotification);

        effpan = midinstr.getPan();
        slpan->setValue(effpan, dontSendNotification);

        return true;
    }

    bool changeEffect(int panelbtnidx, MidiCCType cctype, int val) {

        switch (cctype) {
            case CCVol:
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setVol(val);
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setDirty(MAPVOL);
                break;
            case CCExp:
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setExp(val);
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setDirty(MAPEXP);
                break;
            case CCRev:
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setRev(val);
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setDirty(MAPREV);
                break;
            case CCCho:
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setCho(val);
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setDirty(MAPCH0);
                break;
            case CCMod:
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setMod(val);
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setDirty(MAPMOD);
                break;
            case CCTim:
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setTim(val);
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setDirty(MAPTIM);
                break;
            case CCAtk:
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setAtk(val);
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setDirty(MAPATK);
                break;
            case CCRel:
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setRel(val);
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setDirty(MAPREL);
                break;
            case CCBri:
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setBri(val);
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setDirty(MAPBRI);
                break;
            case CCPan:
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setPan(val);
                instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setDirty(MAPPAN);
                break;
            default:
                break;
        }

        updateToRouteButtonDirtyStyle();
        return true;
    }

    //-----------------------------------------------------------------------------
    // Reset all Effects on this Voice Button to Original
    bool cancelEffects(int panelbtnidx) {

        int dirty = instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->getDirty();

        if (dirty & MAPVOL) {
            instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setVol(effvol);

            auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCVol, effvol);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }

        if (dirty & MAPEXP) {
            instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setExp(effexp);

            auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCExp, effexp);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }

        if (dirty & MAPREV) {
            instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setRev(effrev);

            auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCRev, effrev);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }

        if (dirty & MAPCH0) {
            instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setCho(effcho);

            auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCCho, effcho);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }

        if (dirty & MAPMOD) {
            instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setMod(effmod);

            auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCMod, effmod);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }

        if (dirty & MAPTIM) {
            instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setTim(efftim);

            auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCTim, efftim);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }

        if (dirty & MAPATK) {
            instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setAtk(effatk);

            auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCAtk, effatk);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }

        if (dirty & MAPREL) {
            instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setRel(effrel);

            auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCRel, effrel);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }

        if (dirty & MAPBRI) {
            instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setBri(effbri);

            auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCBri, effbri);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }

        if (dirty & MAPPAN) {
            instrumentpanel->getVoiceButton(panelbtnidx)->getInstrumentPtr()->setPan(effpan);

            auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, CCPan, effpan);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }

        updateToRouteButtonDirtyStyle();
        return true;
    }

    //-----------------------------------------------------------------------------
    // Entry into Effects Panel: Show effects parameters for last panel button pressed
    bool setPanelButton(int panelbtnidx, String sgroup, int midiout, int tabindex) {

        currenttabidx = tabindex;
        buttongroupmidiout = midiout;
        panelbuttonidx = panelbtnidx;

        {
            if (currenttabidx == PTLower)
                group->setColour(GroupComponent::outlineColourId, Colours::palegreen.darker());
            else if (currenttabidx == PTBass)
                group->setColour(GroupComponent::outlineColourId, Colours::antiquewhite.darker());
            else
                group->setColour(GroupComponent::outlineColourId, Colours::palevioletred.darker());
        }

        lblsgrouptxt.setText(sgroup, {});

        String totabtext;
        if (currenttabidx == PTLower) {
            totabtext = "To Lower";
            lblskeyboardtxt.setText("Lower", {});
        }
        else if (currenttabidx == PTBass) {
            totabtext = "To Bass";
            lblskeyboardtxt.setText("Bass&Drums", {});
        }
        else {
            totabtext = "To Upper";
            lblskeyboardtxt.setText("Upper", {});
        }
        tbroute->setButtonText(totabtext);

        ptrvoicebutton = instrumentpanel->getVoiceButton(panelbtnidx);
        String svoice1 = ptrvoicebutton->getInstrument().getVoice();
        lblsvoicetxt.setText(svoice1, {});

        setEffects(instrumentpanel->getVoiceButton(panelbtnidx)->getInstrument());

        // Enable the routing button now that Voice or Effect has been selected
        tbroute->setEnabled(true);
        updateToRouteButtonDirtyStyle();
        hasPanelContextFlag = true;

        return true;
    }

    bool hasPanelContext() const { return hasPanelContextFlag; }

private:
    OwnedArray<Component> components;

    double startTime;

    MidiDevices* mididevices = nullptr;
    InstrumentPanel* instrumentpanel = nullptr;

    juce::GroupComponent* group;

    int currenttabidx = 0;
    int buttongroupmidiout = 1;

    TextButton* tbroute;
    TextButton* tbcancel;
    VoiceButton* ptrvoicebutton;
    int panelbuttonidx = 0;
    bool hasPanelContextFlag = false;

    int mchannel = 1;

    juce::Label lblskeyboard{ "Keyboard" };
    juce::Label lblskeyboardtxt{ "No Keyboard" };
    juce::Label lblsgroup{ "Sound Group" };
    juce::Label lblsgrouptxt{ "No Group" };
    juce::Label lblsvoice{ "Button Voice" };
    juce::Label lblsvoicetxt{ "No Voice" };

    Slider* slvol;
    Slider* slexp;
    Slider* slrev;
    Slider* slcho;
    Slider* slmod;
    Slider* sltim;
    Slider* slatk;
    Slider* slrel;
    Slider* slbri;
    Slider* slpan;

    int effvol;
    int effexp;
    int effrev;
    int effcho;
    int effmod;
    int efftim;
    int effatk;
    int effrel;
    int effbri;
    int effpan;

    void syncEffectsSnapshotFromInstrument()
    {
        if (instrumentpanel == nullptr || panelbuttonidx < 0)
            return;

        auto* inst = instrumentpanel->getVoiceButton(panelbuttonidx)->getInstrumentPtr();
        if (inst == nullptr)
            return;

        effvol = inst->getVol();
        effexp = inst->getExp();
        effrev = inst->getRev();
        effcho = inst->getCho();
        effmod = inst->getMod();
        efftim = inst->getTim();
        effatk = inst->getAtk();
        effrel = inst->getRel();
        effbri = inst->getBri();
        effpan = inst->getPan();
    }

    bool effectsValuesDifferFromSnapshot() const
    {
        if (instrumentpanel == nullptr || panelbuttonidx < 0)
            return false;

        auto* inst = instrumentpanel->getVoiceButton(panelbuttonidx)->getInstrumentPtr();
        if (inst == nullptr)
            return false;

        return inst->getVol() != effvol || inst->getExp() != effexp || inst->getRev() != effrev
            || inst->getCho() != effcho || inst->getMod() != effmod || inst->getTim() != efftim
            || inst->getAtk() != effatk || inst->getRel() != effrel || inst->getBri() != effbri
            || inst->getPan() != effpan;
    }

    /** Same red “pending save” style as panel Save/Load (see updatePanelSaveButtonsPendingStyle). */
    void updateToRouteButtonDirtyStyle()
    {
        if (tbroute == nullptr)
            return;

        if (!tbroute->isEnabled())
        {
            tbroute->setColour(TextButton::textColourOffId, Colours::black);
            tbroute->setColour(TextButton::textColourOnId, Colours::black);
            tbroute->setColour(TextButton::buttonColourId, Colours::lightgrey);
            tbroute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            return;
        }

        if (effectsValuesDifferFromSnapshot())
        {
            tbroute->setColour(TextButton::textColourOffId, Colours::white);
            tbroute->setColour(TextButton::textColourOnId, Colours::white);
            tbroute->setColour(TextButton::buttonColourId, Colours::darkred);
            tbroute->setColour(TextButton::buttonOnColourId, Colours::darkred.brighter());
        }
        else
        {
            tbroute->setColour(TextButton::textColourOffId, Colours::black);
            tbroute->setColour(TextButton::textColourOnId, Colours::black);
            tbroute->setColour(TextButton::buttonColourId, Colours::lightgrey);
            tbroute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
        }
    }

    Slider* createSlider(bool isSnapping)
    {
        auto* s = isSnapping ? new SnappingSlider()
            : new Slider();

        s->setRange(0.0, 127.0, 1.0);
        s->setColour(juce::Slider::ColourIds::trackColourId, juce::Colours::green.darker());
        s->setPopupMenuEnabled(true);
        return s;
    }

    void addLabelAndSetStyle(Label& label, bool heading)
    {
        if (heading == false)
            label.setFont(Font(FontOptions(16.00f, Font::plain)));
        else
            label.setFont(Font(FontOptions(16.00f, Font::bold)));

        label.setJustificationType(Justification::centredLeft);
        label.setEditable(false, false, false);
        label.setColour(TextEditor::textColourId, Colours::white);
        label.setColour(TextEditor::backgroundColourId, Colour(0x00000000));

        addAndMakeVisible(&label);
    }

    //-----------------------------------------------------------------------------
    // This little function avoids a bit of code-duplication by adding a component to
    // our list as well as calling addAndMakeVisible on it..
    template <typename ComponentType>
    ComponentType* addToList(ComponentType* newComp)
    {
        components.add(newComp);
        addAndMakeVisible(newComp);
        return newComp;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPage)
};


//==============================================================================
// Class: KeyboardManualPage - Construct and load
//==============================================================================
static int zinstcntmanualpage = 0;
struct KeyboardPanelPage final : public Component,
    public ComponentListener
{
    KeyboardPanelPage(TabbedComponent& tabs, PanelTabsType tabidx,
        EffectsPage& effectspage,
        VoicesPage& voicespage) :
        startTime(juce::Time::getMillisecondCounterHiRes() * 0.001),
        instrumentmodules(InstrumentModules::getInstance()),
        midiInstruments(MidiInstruments::getInstance()), // Singletons if there isn't already one
        mididevices(MidiDevices::getInstance()),
        instrumentpanel(InstrumentPanel::getInstance()),
        appState(getAppState())
    {
        juce::Logger::writeToLog("== KeyboardManualPage(): Constructor " + std::to_string(zinstcntmanualpage++));

        int bwidth = 75, bheight = 50, swidth = 20;
        int sbwidth = (int)(bwidth / 1.2), sbheight = (int)(bheight / 1.5);

        int mgroup = 10;
        int xgroup = 10, ygroup = 10;

        auto configureArrowButton = [](ArrowCommandButton* button)
            {
                button->setClickingTogglesState(true);
                button->setColour(TextButton::textColourOffId, Colours::black);
                button->setColour(TextButton::textColourOnId, Colours::black);
                button->setColour(TextButton::buttonColourId, Colours::darkgrey.darker(0.25f));
                button->setColour(TextButton::buttonOnColourId, Colours::darkgrey.brighter(0.20f));
            };

        auto setArrowButtonsMutedState = [configureArrowButton](ArrowCommandButton* upButton, ArrowCommandButton* downButton, bool isMuted)
            {
                auto applyState = [&](ArrowCommandButton* button)
                    {
                        if (button == nullptr)
                            return;

                        if (isMuted)
                        {
                            button->setColour(TextButton::textColourOffId, Colours::grey);
                            button->setColour(TextButton::textColourOnId, Colours::grey);
                            button->setColour(TextButton::buttonColourId, Colours::darkred.darker(0.45f));
                            button->setColour(TextButton::buttonOnColourId, Colours::darkred.darker(0.30f));
                        }
                        else
                        {
                            configureArrowButton(button);
                        }

                        button->setEnabled(!isMuted);
                        button->repaint();
                    };

                applyState(upButton);
                applyState(downButton);
            };

        auto getDefaultGroupOutlineColour = [tabidx]()
            {
                if (tabidx == PTLower)
                    return Colours::palegreen.darker();
                if (tabidx == PTBass)
                    return Colours::antiquewhite.darker();
                return Colours::palevioletred.darker();
            };

        auto getGroupOutlineColour = [getDefaultGroupOutlineColour](ButtonGroup* buttonGroup)
            {
                if (buttonGroup != nullptr)
                {
                    const auto name = buttonGroup->groupname.toLowerCase();
                    const bool isDrumByName =
                        name.contains("drum 1")
                        || name.contains("drum1")
                        || name.contains("drums 1")
                        || name.contains("drums1")
                        || name.contains("drum 2")
                        || name.contains("drum2")
                        || name.contains("drums 2")
                        || name.contains("drums2");

                    // Safety fallback for Bass drum groups if names are edited in config.
                    const bool isDrumByBassMidiOut =
                        (buttonGroup->midikeyboard == KBDBASS)
                        && (buttonGroup->midiout == 10 || buttonGroup->midiout == 11);

                    if (isDrumByName || isDrumByBassMidiOut)
                    {
                        return Colours::cornflowerblue;
                    }
                }
                return getDefaultGroupOutlineColour();
            };

        auto applyMutedGroupVisualCue = [tabidx, getDefaultGroupOutlineColour, getGroupOutlineColour](ButtonGroup* buttonGroup, bool isMuted)
            {
                if (buttonGroup == nullptr)
                    return;

                if (auto* groupComponent = buttonGroup->getGroupComponentPtr())
                {
                    // Keep Lower/Bass group outlines consistent with their tab palette.
                    // Use muted-outline tint only on Upper where contrast remains clear.
                    const bool useMutedOutlineTint = isMuted && (tabidx == PTUpper);
                    groupComponent->setColour(GroupComponent::outlineColourId,
                        useMutedOutlineTint ? Colours::darkred.brighter(0.35f)
                                            : getGroupOutlineColour(buttonGroup));
                }
            };

        // Remember current Tab Index so we can redirect Voice and Effects pages back to it
        currenttabidx = tabidx;

        // Keyboard Manual Status Bar
        auto* statusLabel = addToList(new Label("Status: "));
        //statusLabel->setColour(juce::Label::backgroundColourId, juce::Colours::black);
        statusLabel->setColour(juce::Label::textColourId, juce::Colours::grey);
        statusLabel->setJustificationType(juce::Justification::left);
        statusLabel->setBounds(mgroup + 1260, 185, 200, 20);

        int g1width, g2width, g3width, g4width;
        int g1widthfull;
        int g1xoffset, g2xoffset, g3xoffset, g4xoffset;
        // --------------------------------------------------------------------
        // Upper Organ Button Group 1
        // --------------------------------------------------------------------
        {

            // Full number of buttons for Group 1 is 12, but for Bass show only 8 
            // Note, we create the max amount of buttons for a group, but display 
            // smaller number for e.g. Bass as fewer options available. This keeps
            // the logic and panel layout consistent.
            int cbuttons = 12;
            int cbuttonsdisplayed = 12;

            int rbuttons = 2;
            int bgroupid = 101;

            // Start with 1st Instrument on the selected Panel (Upper, Lower or Bass) and the first ButtonGroup
            int buttongroupidx = 0;
            if (tabidx == PTLower) {
                panelbuttonidx = 32;    // On Lower Panel
                buttongroupidx = 4;
            }
            else if (tabidx == PTBass) {
                panelbuttonidx = 64;   // On Bassa & Drums Panel
                buttongroupidx = 8;
                cbuttonsdisplayed = 8;  // Display only 8 buttons on Bass
            }
            else {
                panelbuttonidx = 0;     // On Upper Panel
                buttongroupidx = 0;
            }

            // Load Button Group Details
            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
            String buttongroupname = ptrbuttongroup->groupname;
            int buttongroupmidiout = ptrbuttongroup->midiout;
            String buttongrouptitle = formatButtonGroupTitle(*ptrbuttongroup, false);

            // Create G1 volume slider so if can be referenced by the buttons.
            auto* g1svol = addToList(createVolSlider(false));

            g1xoffset = xgroup;

            g1width = xgroup + mgroup + bwidth * (cbuttonsdisplayed / 2) + mgroup + swidth;
            g1widthfull = xgroup + mgroup + bwidth * (cbuttons / 2) + mgroup + swidth;
            int g1height = ygroup + mgroup * rbuttons + bheight * rbuttons + mgroup + sbheight;
            {
                auto* group = addToList(new GroupComponent("group", buttongrouptitle));

                group->setColour(GroupComponent::outlineColourId, getGroupOutlineColour(ptrbuttongroup));

                group->setBounds(g1xoffset, ygroup, g1width, g1height);

                ptrbuttongroup->setGroupComponentPtr(group);
            }

            // Create the Panel Buttons for Group 1
            int row = 0, col = 0;
            for (int i = 0; i < cbuttons; ++i)
            {
                // First or second row of button group
                if (i == cbuttonsdisplayed / rbuttons)
                {
                    row++;
                    col = 0;
                }

                Instrument instrument = instrumentpanel->getVoiceButton(panelbuttonidx)->getInstrument();
                instrument.setChannel(buttongroupmidiout);

                //auto* sb = addToList(new VoiceButton(instrument.getVoice()));
                VoiceButton* sb = new VoiceButton(instrument.getVoice());
                if (i < cbuttonsdisplayed)
                    addToList(sb);

                instrumentpanel->getVoiceButton(panelbuttonidx)->setDisplayButtonPtr(sb);

                sb->setInstrument(instrument);
                sb->setButtonText(instrument.getVoice());

                sb->setButtonGroupId(bgroupid);
                sb->setButtonId(i);
                sb->setPanelButtonIdx(panelbuttonidx);
                sb->setExplicitUserClickHandler([this](int idx) { registerExplicitVoiceSelection(idx); });

                sb->setClickingTogglesState(true);

                sb->setRadioGroupId(bgroupid);
                sb->setColour(TextButton::textColourOffId, Colours::black);
                sb->setColour(TextButton::textColourOnId, Colours::black);
                sb->setColour(TextButton::buttonColourId, Colours::lightgrey);
                sb->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
                sb->setBounds(g1xoffset + mgroup + col * bwidth, ygroup + mgroup * 2 + row * bheight, bwidth, bheight);
                sb->setConnectedEdges(((col != 0) ? Button::ConnectedOnLeft : 0)
                    | ((col != (cbuttons / rbuttons - 1)) ? Button::ConnectedOnRight : 0));
                sb->onClick = [=]()
                    {
                        // If the button is already on, do not send another change in addition to original down
                        // New button pressed is now toggling this one off. 
                        // To do: In future may actually use repeat toggle to turn all buttons in group off == mute group
                        if (!sb->getToggleState())
                            return;

                        DBG("VoiceButton.onClick(): Clicked " + std::to_string(sb->getPanelButtonIdx()));

                        // Remember last button we pressed in case we need to edit sound or effects
                        panelbuttonidx = sb->getPanelButtonIdx();

                        // Reload the instrument since the Voice may have been updated since original load
                        // and update the Button text if not done yet
                        Instrument instrument = instrumentpanel->getVoiceButton(panelbuttonidx)->getInstrument();

                        sb->setButtonText(instrument.getVoice());

                        // Reverse lookup Button Group ID for the selected Panel Button to preset Effects Midi Channel
                        int panelgroup = lookupPanelGroup(panelbuttonidx);
                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(panelgroup);
                        int buttongroupmidiout = ptrbuttongroup->midiout;

                        // Remember active Voice Button in Group
                        ptrbuttongroup->setActiveVoiceButton(panelbuttonidx);

                        int controllerNumber = CCMSB;    // MSB
                        juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
                            buttongroupmidiout,
                            controllerNumber,
                            instrument.getMSB()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        // Send the MIDI CC message to MIDI output(s)
                        //mididevices->sendMessageNow(ccMessage);
                        mididevices->sendToOutputs(ccMessage);

                        controllerNumber = CCLSB;    // LSB
                        ccMessage = juce::MidiMessage::controllerEvent(
                            buttongroupmidiout,
                            controllerNumber,
                            instrument.getLSB()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        // Send PC
                        ccMessage = juce::MidiMessage::programChange(
                            buttongroupmidiout,
                            instrument.getFont()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        //---------------------------------------------------------
                        // Only send Effects if they have been modified in this app
                        const int channelvolume = computeEffectiveCc7ForButton(ptrbuttongroup, g1svol, panelbuttonidx);
                        updateVolumeSliderDebugText(g1svol, channelvolume);
                        sendCombinedGroupVolume(ptrbuttongroup, buttongroupmidiout, channelvolume);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g1svol->getValue(), channelvolume);

                        // Send Exp
                        if (ptrbuttongroup->isEffectDirty(1, instrument.getExp())) {
                            //if (isdirty & MAPEXP) {
                            controllerNumber = CCExp;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getExp()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Rev
                        if (ptrbuttongroup->isEffectDirty(2, instrument.getRev())) {
                            //if (isdirty & MAPREV) {
                            controllerNumber = CCRev;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getRev()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Cho
                        if (ptrbuttongroup->isEffectDirty(3, instrument.getCho())) {
                            //if (isdirty & MAPCHO) {
                            controllerNumber = CCCho;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getCho()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Mod
                        if (ptrbuttongroup->isEffectDirty(4, instrument.getMod())) {
                            //if (isdirty & MAPMOD) {
                            controllerNumber = CCMod;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getMod()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Tim
                        if (ptrbuttongroup->isEffectDirty(5, instrument.getTim())) {
                            //if (isdirty & MAPTIM) {
                            controllerNumber = CCTim;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getTim()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Atk
                        if (ptrbuttongroup->isEffectDirty(6, instrument.getAtk())) {
                            //if (isdirty & MAPATK) {
                            controllerNumber = CCAtk;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getAtk()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Rel
                        if (ptrbuttongroup->isEffectDirty(7, instrument.getRel())) {
                            //if (isdirty & MAPREL) {
                            controllerNumber = CCRel;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getRel()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Bri
                        if (ptrbuttongroup->isEffectDirty(8, instrument.getBri())) {
                            //if (isdirty & MAPBRI) {
                            controllerNumber = CCBri;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getBri()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Pan
                        if (ptrbuttongroup->isEffectDirty(9, instrument.getPan())) {
                            //if (isdirty & MAPPAN) {
                            controllerNumber = CCPan;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getPan()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }
                    };

                    sb->addMouseListener(this, true);

                // Default first button to send MIDI program change
                if (i == 0)
                    sb->triggerClick();

                panelbuttonidx++;
                col++;
            }

            // Create Volume Slider
            g1svol->setSliderStyle(Slider::LinearBarVertical);
            g1svol->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            g1svol->setBounds(g1xoffset + mgroup + (cbuttonsdisplayed / rbuttons) * bwidth + mgroup, ygroup + mgroup * 2, 20, mgroup + bheight * rbuttons + sbheight);
            g1svol->setPopupDisplayEnabled(true, true, this);
            g1svol->setValue(ptrbuttongroup->getMasterVolStep());
            g1svol->onValueChange = [=]()
                {
                    ButtonGroup* selectedButtonGroup = instrumentpanel->getButtonGroup(buttongroupidx);
                    selectedButtonGroup->setMasterVolStep((int)g1svol->getValue());

                    const int activeButtonIdx = selectedButtonGroup->getActiveVoiceButton();
                    const int effectiveCc7 = computeEffectiveCc7ForButton(selectedButtonGroup, g1svol, activeButtonIdx);
                    updateVolumeSliderDebugText(g1svol, effectiveCc7);
                    sendCombinedGroupVolume(selectedButtonGroup, selectedButtonGroup->midiout, effectiveCc7);
                    updateVolumeStatusLine(statusLabel, buttongroupname, g1svol->getValue(), effectiveCc7);
                };

            // Slider Volume up and down buttons
            auto* g1bvup = addToList(new ArrowCommandButton(ArrowCommandButton::Direction::up));
            configureArrowButton(g1bvup);
            g1bvup->setBounds(g1xoffset + mgroup + (bwidth - sbwidth) / 2 + (cbuttonsdisplayed / rbuttons - 2) * bwidth, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g1bvup->onClick = [=]()
                {
                    //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Status", strtest /*"The button was clicked!"*/);

                    vbuttonupClicked(g1svol);
                    g1bvup->setToggleState(false, dontSendNotification);

                    //String strstatus = "Vol Up: " + juce::String(g1svol->getValue());
                    //statusLabel->setText(strstatus, juce::dontSendNotification);
                };

            auto* g1bvdwn = addToList(new ArrowCommandButton(ArrowCommandButton::Direction::down));
            configureArrowButton(g1bvdwn);
            g1bvdwn->setBounds(g1xoffset + mgroup + (bwidth - sbwidth) / 2 + (cbuttonsdisplayed / rbuttons - 1) * bwidth, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g1bvdwn->onClick = [=]()
                {
                    //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Status", strtest /*"The button was clicked!"*/);

                    vbuttondownClicked(g1svol);
                    g1bvdwn->setToggleState(false, dontSendNotification);

                    //String strstatus = "Vol Down: " + juce::String(g1svol->getValue());
                    //statusLabel->setText(strstatus, juce::dontSendNotification);
                };

            // Add Sound Mute Button
            auto* g1tbmute = addToList(new MuteButton("Mute"));
            g1tbmute->setClickingTogglesState(true);
            g1tbmute->setColour(TextButton::textColourOffId, Colours::white);
            g1tbmute->setColour(TextButton::textColourOnId, Colours::black);
            g1tbmute->setColour(TextButton::buttonColourId, Colours::black.darker());
            g1tbmute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            g1tbmute->setBounds(g1xoffset + mgroup + (bwidth - sbwidth) / 2 + ((cbuttonsdisplayed / rbuttons - 2) -1) * bwidth, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g1tbmute->setToggleState(false, dontSendNotification);
            g1tbmute->onClick = [=]()
                {
                    if (g1tbmute->getToggleState()) {

                        g1tbmute->setButtonText("Muted");

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
                        auto ccMessage = juce::MidiMessage::controllerEvent(ptrbuttongroup->midiout, 7, 0);
                        // To do: Which one to use
                        //ccMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime);
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        g1svol->setEnabled(false);
                        setArrowButtonsMutedState(g1bvup, g1bvdwn, true);

                        ptrbuttongroup->setMuteButtonStatus(true);
                        ptrbuttongroup->isEffectDirty(0, 0);
                        updateVolumeSliderDebugText(g1svol, 0);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g1svol->getValue(), 0);
                        applyMutedGroupVisualCue(ptrbuttongroup, true);
                        g1uppermute = true;
                    }
                    else {

                        g1tbmute->setButtonText("Mute");

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
                        ptrbuttongroup->setMuteButtonStatus(false);
                        applyMutedGroupVisualCue(ptrbuttongroup, false);
                        g1svol->setEnabled(true);
                        setArrowButtonsMutedState(g1bvup, g1bvdwn, false);

                        const int activeButtonIdx = ptrbuttongroup->getActiveVoiceButton();
                        const int effectiveCc7 = computeEffectiveCc7ForButton(ptrbuttongroup, g1svol, activeButtonIdx);
                        updateVolumeSliderDebugText(g1svol, effectiveCc7);
                        sendCombinedGroupVolume(ptrbuttongroup, ptrbuttongroup->midiout, effectiveCc7);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g1svol->getValue(), effectiveCc7);
                        g1uppermute = false;
                    }

                };

            // Mute Button: Toggle Initial Button Mute Status keeping in mind Button Group and Tab Index
            int bmgroupidx = 0;
            if (tabidx == PTLower) { bmgroupidx = 4; }
            else if (tabidx == PTBass) { bmgroupidx = 8; }
            else { bmgroupidx = 0; }

            ptrbuttongroup = instrumentpanel->getButtonGroup(bmgroupidx);
            ptrbuttongroup->setMuteButtonPtr(g1tbmute);

            ptrbuttongroup = instrumentpanel->getButtonGroup(bmgroupidx);
            if (ptrbuttongroup->muted)
                g1tbmute->triggerClick();
        }   // End of Button Group 1

        // --------------------------------------------------------------------
        // Upper Orchestral Button Group 2
        // --------------------------------------------------------------------
        {
            int buttongroupidx = 1;
            if (tabidx == PTLower) {
                panelbuttonidx = 32 + 12;    // On Lower Panel
                buttongroupidx = 5;
            }
            else if (tabidx == PTBass) {
                panelbuttonidx = 64 + 12;   // On Bassa & Drums Panel
                buttongroupidx = 9;
            }
            else {
                panelbuttonidx = 0 + 12;     // On Upper Panel
                buttongroupidx = 1;
            }

            // Load Button Group Details
            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
            String buttongroupname = ptrbuttongroup->groupname;
            int buttongroupmidiout = ptrbuttongroup->midiout;
            String buttongrouptitle = formatButtonGroupTitle(*ptrbuttongroup, false);

            // Create the Group Volume slider as it is referenced by the Voice Buttons
            auto* g2svol = addToList(createVolSlider(false));

            g2xoffset = g1widthfull + xgroup * 2;

            int cbuttons = 8;
            int rbuttons = 2;
            int bgroupid = 102;

            g2width = xgroup + mgroup + bwidth * (cbuttons / 2) + mgroup + swidth;
            int g2height = ygroup + mgroup * rbuttons + bheight * rbuttons + mgroup + sbheight;
            {
                auto* group = addToList(new GroupComponent("group", buttongrouptitle));

                group->setColour(GroupComponent::outlineColourId, getGroupOutlineColour(ptrbuttongroup));

                group->setBounds(g2xoffset, ygroup, g2width, g2height);

                ptrbuttongroup->setGroupComponentPtr(group);
            }

            // Create the Panel Buttons for Group 2
            int row = 0, col = 0;
            for (int i = 0; i < cbuttons; ++i)
            {
                // First or second row of button group
                if (i == cbuttons / rbuttons)
                {
                    row++;
                    col = 0;
                }

                Instrument instrument = instrumentpanel->getVoiceButton(panelbuttonidx)->getInstrument();
                instrument.setChannel(buttongroupmidiout);

                auto* sb = addToList(new VoiceButton(instrument.getVoice()));
                instrumentpanel->getVoiceButton(panelbuttonidx)->setDisplayButtonPtr(sb);

                sb->setInstrument(instrument);
                sb->setButtonText(instrument.getVoice());

                sb->setButtonGroupId(bgroupid);
                sb->setButtonId(i);
                sb->setPanelButtonIdx(panelbuttonidx);
                sb->setExplicitUserClickHandler([this](int idx) { registerExplicitVoiceSelection(idx); });

                sb->setClickingTogglesState(true);

                sb->setRadioGroupId(bgroupid);
                sb->setColour(TextButton::textColourOffId, Colours::black);
                sb->setColour(TextButton::textColourOnId, Colours::black);
                sb->setColour(TextButton::buttonColourId, Colours::lightgrey);
                sb->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
                sb->setBounds(g2xoffset + mgroup + col * bwidth, ygroup + mgroup * 2 + row * bheight, bwidth, bheight);
                sb->setConnectedEdges(((col != 0) ? Button::ConnectedOnLeft : 0)
                    | ((col != (cbuttons / rbuttons - 1)) ? Button::ConnectedOnRight : 0));
                sb->onClick = [=]()
                    {
                        // If the button is already on, do not send another change in addition to original down
                        // New button pressed is now toggling this one off. 
                        // To do: In future may actually use repeat toggle to turn all buttons in group off == mute group
                        if (!sb->getToggleState())
                            return;

                        DBG("VoiceButton.onClick(): Clicked " + std::to_string(sb->getPanelButtonIdx()));

                        // Remember last button we pressed in case we need to edit sound or effects
                        panelbuttonidx = sb->getPanelButtonIdx();

                        // Reload the instrument since the Voice may have been updated since original load
                        // and update the Button text if not done yet
                        Instrument instrument = instrumentpanel->getVoiceButton(panelbuttonidx)->getInstrument();

                        sb->setButtonText(instrument.getVoice());

                        int panelgroup = lookupPanelGroup(panelbuttonidx);
                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(panelgroup);
                        int buttongroupmidiout = ptrbuttongroup->midiout;

                        // Remember active Voice Button in Group
                        ptrbuttongroup->setActiveVoiceButton(panelbuttonidx);

                        int controllerNumber = CCMSB;    // MSB
                        juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
                            buttongroupmidiout,
                            controllerNumber,
                            instrument.getMSB()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        // Send the MIDI CC message to MIDI output(s)
                        //mididevices->sendMessageNow(ccMessage);
                        mididevices->sendToOutputs(ccMessage);

                        controllerNumber = CCLSB;    // LSB
                        ccMessage = juce::MidiMessage::controllerEvent(
                            buttongroupmidiout,
                            controllerNumber,
                            instrument.getLSB()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        // Send PC
                        ccMessage = juce::MidiMessage::programChange(
                            buttongroupmidiout,
                            instrument.getFont()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        //---------------------------------------------------------
                        // Only send Effects if they have been modified in this app
                        const int channelvolume = computeEffectiveCc7ForButton(ptrbuttongroup, g2svol, panelbuttonidx);
                        updateVolumeSliderDebugText(g2svol, channelvolume);
                        sendCombinedGroupVolume(ptrbuttongroup, buttongroupmidiout, channelvolume);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g2svol->getValue(), channelvolume);

                        // Send Exp
                        if (ptrbuttongroup->isEffectDirty(1, instrument.getExp())) {
                            //if (isdirty & MAPEXP) {
                            controllerNumber = CCExp;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getExp()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Rev
                        if (ptrbuttongroup->isEffectDirty(2, instrument.getRev()) == true) {
                            //if (isdirty & MAPREV) {
                            controllerNumber = CCRev;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getRev()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        if (ptrbuttongroup->isEffectDirty(3, instrument.getCho()) == true) {
                            //if (isdirty & MAPCHO) {
                            controllerNumber = CCCho;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getCho()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Mod
                        if (ptrbuttongroup->isEffectDirty(4, instrument.getMod()) == true) {
                            //if (isdirty & MAPMOD) {
                            controllerNumber = CCMod;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getMod()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Tim
                        if (ptrbuttongroup->isEffectDirty(5, instrument.getTim()) == true) {
                            //if (isdirty & MAPTIM) {
                            controllerNumber = CCTim;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getTim()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                            //}
                        }

                        // Send Atk
                        if (ptrbuttongroup->isEffectDirty(6, instrument.getAtk()) == true) {
                            //if (isdirty & MAPATK) {
                            controllerNumber = CCAtk;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getAtk()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Rel
                        if (ptrbuttongroup->isEffectDirty(7, instrument.getRel()) == true) {
                            //if (isdirty & MAPREL) {
                            controllerNumber = CCRel;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getRel()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Bri
                        if (ptrbuttongroup->isEffectDirty(8, instrument.getBri()) == true) {
                            //if (isdirty & MAPBRI) {
                            controllerNumber = CCBri;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getBri()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Pan
                        if (ptrbuttongroup->isEffectDirty(9, instrument.getPan()) == true) {
                            //if (isdirty & MAPPAN) {
                            controllerNumber = CCPan;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                instrument.getChannel(),
                                buttongroupmidiout,
                                instrument.getPan()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }
                    };

                    sb->addMouseListener(this, true);

                // Default first button to send MIDI program change
                if (i == 0)
                    sb->triggerClick();

                panelbuttonidx++;
                col++;
            }

            // Create Volume Slider
            g2svol->setSliderStyle(Slider::LinearBarVertical);
            g2svol->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            g2svol->setBounds(g2xoffset + mgroup + col * bwidth + mgroup, ygroup + mgroup * 2, 20, mgroup + bheight * rbuttons + sbheight);
            g2svol->setPopupDisplayEnabled(true, true, this);
            g2svol->setValue(ptrbuttongroup->getMasterVolStep());
            g2svol->onValueChange = [=]()
                {
                    ButtonGroup* selectedButtonGroup = instrumentpanel->getButtonGroup(buttongroupidx);
                    selectedButtonGroup->setMasterVolStep((int)g2svol->getValue());

                    const int activeButtonIdx = selectedButtonGroup->getActiveVoiceButton();
                    const int effectiveCc7 = computeEffectiveCc7ForButton(selectedButtonGroup, g2svol, activeButtonIdx);
                    updateVolumeSliderDebugText(g2svol, effectiveCc7);
                    sendCombinedGroupVolume(selectedButtonGroup, selectedButtonGroup->midiout, effectiveCc7);
                    updateVolumeStatusLine(statusLabel, buttongroupname, g2svol->getValue(), effectiveCc7);
                };

            // Slider Volume up and down buttons
            auto* g2bvup = addToList(new ArrowCommandButton(ArrowCommandButton::Direction::up));
            configureArrowButton(g2bvup);
            g2bvup->setBounds(g2xoffset + mgroup + (bwidth - sbwidth) / 2 + (cbuttons / rbuttons - 2) * bwidth, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g2bvup->onClick = [=]()
                {
                    //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Status", strtest /*"The button was clicked!"*/);

                    vbuttonupClicked(g2svol);
                    g2bvup->setToggleState(false, dontSendNotification);
                };

            auto* g2bvdwn = addToList(new ArrowCommandButton(ArrowCommandButton::Direction::down));
            configureArrowButton(g2bvdwn);
            g2bvdwn->setBounds(g2xoffset + mgroup + (bwidth - sbwidth) / 2 + (cbuttons / rbuttons - 1) * bwidth, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g2bvdwn->onClick = [=]()
                {
                    //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Status", strtest /*"The button was clicked!"*/);

                    vbuttondownClicked(g2svol);
                    g2bvdwn->setToggleState(false, dontSendNotification);
                };

            // Add Sound Mute Button
            auto* g2tbmute = addToList(new MuteButton("Mute"));
            g2tbmute->setClickingTogglesState(true);
            g2tbmute->setColour(TextButton::textColourOffId, Colours::white);
            g2tbmute->setColour(TextButton::textColourOnId, Colours::black);
            g2tbmute->setColour(TextButton::buttonColourId, Colours::black.darker());
            g2tbmute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            g2tbmute->setBounds(g2xoffset + mgroup + (bwidth - sbwidth) / 2 + (cbuttons / rbuttons - 3) * bwidth, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g2tbmute->setToggleState(false, dontSendNotification);
            g2tbmute->onClick = [=]()
                {
                    if (g2tbmute->getToggleState()) {

                        g2tbmute->setButtonText("Muted");

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
                        auto ccMessage = juce::MidiMessage::controllerEvent(ptrbuttongroup->midiout, 7, 0);
                        // To do: Which one to use
                        //ccMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime);
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        g2svol->setEnabled(false);
                        setArrowButtonsMutedState(g2bvup, g2bvdwn, true);

                        ptrbuttongroup->setMuteButtonStatus(true);
                        ptrbuttongroup->isEffectDirty(0, 0);
                        updateVolumeSliderDebugText(g2svol, 0);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g2svol->getValue(), 0);
                        applyMutedGroupVisualCue(ptrbuttongroup, true);
                        g2uppermute = true;
                    }
                    else {

                        g2tbmute->setButtonText("Mute");

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
                        ptrbuttongroup->setMuteButtonStatus(false);
                        applyMutedGroupVisualCue(ptrbuttongroup, false);
                        g2svol->setEnabled(true);
                        setArrowButtonsMutedState(g2bvup, g2bvdwn, false);

                        const int activeButtonIdx = ptrbuttongroup->getActiveVoiceButton();
                        const int effectiveCc7 = computeEffectiveCc7ForButton(ptrbuttongroup, g2svol, activeButtonIdx);
                        updateVolumeSliderDebugText(g2svol, effectiveCc7);
                        sendCombinedGroupVolume(ptrbuttongroup, ptrbuttongroup->midiout, effectiveCc7);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g2svol->getValue(), effectiveCc7);
                        g2uppermute = false;
                    }

                };

            // Mute Button: Toggle Initial Button Mute Status keeping in mind Button Group and Tab Index
            int bmgroupidx = 1;
            if (tabidx == PTLower) { bmgroupidx = 5; }
            else if (tabidx == PTBass) { bmgroupidx = 9; }
            else { bmgroupidx = 1; }

            ptrbuttongroup = instrumentpanel->getButtonGroup(bmgroupidx);
            ptrbuttongroup->setMuteButtonPtr(g2tbmute);

            ptrbuttongroup = instrumentpanel->getButtonGroup(bmgroupidx);
            if (ptrbuttongroup->muted)
                g2tbmute->triggerClick();
        }   // End of Button Group 2

        // --------------------------------------------------------------------
        // Upper Symphonic Button Group 3
        // --------------------------------------------------------------------
        {
            int buttongroupidx = 2;
            if (tabidx == PTLower) {
                panelbuttonidx = 32 + 20;    // On Lower Panel
                buttongroupidx = 6;
            }
            else if (tabidx == PTBass) {
                panelbuttonidx = 64 + 20;   // On Bassa & Drums Panel
                buttongroupidx = 10;
            }
            else {
                panelbuttonidx = 0 + 20;     // On Upper Panel
                buttongroupidx = 2;
            }

            // Load Button Group Details
            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
            String buttongroupname = ptrbuttongroup->groupname;
            int buttongroupmidiout = ptrbuttongroup->midiout;
            String buttongrouptitle = formatButtonGroupTitle(*ptrbuttongroup, false);

            // Create the Group Volume slider as it is referenced by the Voice Buttons
            auto* g3svol = addToList(createVolSlider(false));

            g3xoffset = g2xoffset + g2width + xgroup;

            int cbuttons = 6;
            int rbuttons = 2;
            int bgroupid = 103;

            g3width = xgroup + mgroup + bwidth * (cbuttons / 2) + mgroup + swidth;
            int g3height = ygroup + mgroup * rbuttons + bheight * rbuttons + mgroup + sbheight;
            {
                auto* group = addToList(new GroupComponent("group", buttongrouptitle));

                group->setColour(GroupComponent::outlineColourId, getGroupOutlineColour(ptrbuttongroup));

                group->setBounds(g3xoffset, ygroup, g3width, g3height);

                ptrbuttongroup->setGroupComponentPtr(group);
            }

            // Create the Panel Buttons for Group 3
            int row = 0, col = 0;
            for (int i = 0; i < cbuttons; ++i)
            {
                // First or second row of button group
                if (i == cbuttons / rbuttons)
                {
                    row++;
                    col = 0;
                }

                ////Instrument instrument = midiInstruments.getInstrument(2, i + 1);
                Instrument instrument = instrumentpanel->getVoiceButton(panelbuttonidx)->getInstrument();
                instrument.setChannel(buttongroupmidiout);

                ////auto* sb = addToList(new VoiceButton("Sound " + String(i + 1)));
                auto* sb = addToList(new VoiceButton(instrument.getVoice()));
                instrumentpanel->getVoiceButton(panelbuttonidx)->setDisplayButtonPtr(sb);

                sb->setInstrument(instrument);
                sb->setButtonText(instrument.getVoice());

                sb->setButtonGroupId(bgroupid);
                sb->setButtonId(i);
                sb->setPanelButtonIdx(panelbuttonidx);
                sb->setExplicitUserClickHandler([this](int idx) { registerExplicitVoiceSelection(idx); });

                sb->setClickingTogglesState(true);

                sb->setRadioGroupId(bgroupid);
                sb->setColour(TextButton::textColourOffId, Colours::black);
                sb->setColour(TextButton::textColourOnId, Colours::black);
                sb->setColour(TextButton::buttonColourId, Colours::lightgrey);
                sb->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
                sb->setBounds(g3xoffset + mgroup + col * bwidth, ygroup + mgroup * 2 + row * bheight, bwidth, bheight);
                sb->setConnectedEdges(((col != 0) ? Button::ConnectedOnLeft : 0)
                    | ((col != (cbuttons / rbuttons - 1)) ? Button::ConnectedOnRight : 0));
                sb->onClick = [=]()
                    {
                        // If the button is already on, do not send another change in addition to original down
                        // New button pressed is now toggling this one off. 
                        // To do: In future may actually use repeat toggle to turn all buttons in group off == mute group
                        if (!sb->getToggleState())
                            return;

                        DBG("VoiceButton.onClick(): Clicked " + std::to_string(sb->getPanelButtonIdx()));

                        // Remember last button we pressed in case we need to edit sound or effects
                        panelbuttonidx = sb->getPanelButtonIdx();

                        // Reload the instrument since the Voice may have been updated since original load
                        // and update the Button text if not done yet
                        Instrument instrument = instrumentpanel->getVoiceButton(panelbuttonidx)->getInstrument();

                        sb->setButtonText(instrument.getVoice());

                        int panelgroup = lookupPanelGroup(panelbuttonidx);
                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(panelgroup);
                        int buttongroupmidiout = ptrbuttongroup->midiout;

                        // Remember active Voice Button in Group
                        ptrbuttongroup->setActiveVoiceButton(panelbuttonidx);

                        int controllerNumber = CCMSB;    // MSB
                        juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
                            buttongroupmidiout,
                            controllerNumber,
                            instrument.getMSB()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        // Send the MIDI CC message to MIDI output(s)
                        //mididevices->sendMessageNow(ccMessage);
                        mididevices->sendToOutputs(ccMessage);

                        controllerNumber = CCLSB;    // LSB
                        ccMessage = juce::MidiMessage::controllerEvent(
                            buttongroupmidiout,
                            controllerNumber,
                            instrument.getLSB()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        // Send PC
                        ccMessage = juce::MidiMessage::programChange(
                            buttongroupmidiout,
                            instrument.getFont()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        //---------------------------------------------------------
                        // Only send Effects if they have been modified in this app
                        const int channelvolume = computeEffectiveCc7ForButton(ptrbuttongroup, g3svol, panelbuttonidx);
                        updateVolumeSliderDebugText(g3svol, channelvolume);
                        sendCombinedGroupVolume(ptrbuttongroup, buttongroupmidiout, channelvolume);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g3svol->getValue(), channelvolume);

                        // Send Exp
                        if (ptrbuttongroup->isEffectDirty(1, instrument.getExp()) == true) {
                            //if (isdirty & MAPEXP) {
                            controllerNumber = CCExp;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getExp()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Rev
                        if (ptrbuttongroup->isEffectDirty(2, instrument.getRev()) == true) {
                            //if (isdirty & MAPREV) {
                            controllerNumber = CCRev;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getRev()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Cho
                        if (ptrbuttongroup->isEffectDirty(3, instrument.getCho()) == true) {
                            //if (isdirty & MAPCHO) {
                            controllerNumber = CCCho;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getCho()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Mod
                        if (ptrbuttongroup->isEffectDirty(4, instrument.getMod()) == true) {
                            //if (isdirty & MAPMOD) {
                            controllerNumber = CCMod;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getMod()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Tim
                        if (ptrbuttongroup->isEffectDirty(5, instrument.getTim()) == true) {
                            //if (isdirty & MAPTIM) {
                            controllerNumber = CCTim;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getTim()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Atk
                        if (ptrbuttongroup->isEffectDirty(6, instrument.getAtk()) == true) {
                            //if (isdirty & MAPATK) {
                            controllerNumber = CCAtk;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getAtk()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Rel
                        if (ptrbuttongroup->isEffectDirty(7, instrument.getRel()) == true) {
                            //if (isdirty & MAPREL) {
                            controllerNumber = CCRel;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getRel()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Bri
                        if (ptrbuttongroup->isEffectDirty(8, instrument.getBri()) == true) {
                            //if (isdirty & MAPBRI) {
                            controllerNumber = CCBri;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getBri()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Pan
                        if (ptrbuttongroup->isEffectDirty(9, instrument.getPan()) == true) {
                            //if (isdirty & MAPPAN) {
                            controllerNumber = CCPan;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                buttongroupmidiout,
                                controllerNumber,
                                instrument.getPan()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }
                    };

                    sb->addMouseListener(this, true);

                // Default first button to send MIDI program change
                if (i == 0)
                    sb->triggerClick();

                panelbuttonidx++;
                col++;
            }

            // Create Volume Slider
            g3svol->setSliderStyle(Slider::LinearBarVertical);
            g3svol->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            g3svol->setBounds(g3xoffset + mgroup + col * bwidth + mgroup, ygroup + mgroup * 2, 20, mgroup + bheight * rbuttons + sbheight);
            g3svol->setPopupDisplayEnabled(true, true, this);
            g3svol->setValue(ptrbuttongroup->getMasterVolStep());
            g3svol->onValueChange = [=]()
                {
                    ButtonGroup* selectedButtonGroup = instrumentpanel->getButtonGroup(buttongroupidx);
                    selectedButtonGroup->setMasterVolStep((int)g3svol->getValue());

                    const int activeButtonIdx = selectedButtonGroup->getActiveVoiceButton();
                    const int effectiveCc7 = computeEffectiveCc7ForButton(selectedButtonGroup, g3svol, activeButtonIdx);
                    updateVolumeSliderDebugText(g3svol, effectiveCc7);
                    sendCombinedGroupVolume(selectedButtonGroup, selectedButtonGroup->midiout, effectiveCc7);
                    updateVolumeStatusLine(statusLabel, buttongroupname, g3svol->getValue(), effectiveCc7);
                };

            // Slider Volume up and down buttons
            auto* g3bvup = addToList(new ArrowCommandButton(ArrowCommandButton::Direction::up));
            configureArrowButton(g3bvup);
            g3bvup->setBounds(g3xoffset + mgroup + (bwidth - sbwidth) / 2 + (cbuttons / rbuttons - 2) * bwidth, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g3bvup->onClick = [=]()
                {
                    //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Status", strtest /*"The button was clicked!"*/);

                    vbuttonupClicked(g3svol);
                    g3bvup->setToggleState(false, dontSendNotification);
                };

            auto* g3bvdwn = addToList(new ArrowCommandButton(ArrowCommandButton::Direction::down));
            configureArrowButton(g3bvdwn);
            g3bvdwn->setBounds(g3xoffset + mgroup + (bwidth - sbwidth) / 2 + (cbuttons / rbuttons - 1) * bwidth, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g3bvdwn->onClick = [=]()
                {
                    //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Status", strtest /*"The button was clicked!"*/);

                    vbuttondownClicked(g3svol);
                    g3bvdwn->setToggleState(false, dontSendNotification);
                };

            // Add Sound Mute Button
            auto* g3tbmute = addToList(new MuteButton("Mute"));
            g3tbmute->setClickingTogglesState(true);
            g3tbmute->setColour(TextButton::textColourOffId, Colours::white);
            g3tbmute->setColour(TextButton::textColourOnId, Colours::black);
            g3tbmute->setColour(TextButton::buttonColourId, Colours::black.darker());
            g3tbmute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            g3tbmute->setBounds(g3xoffset + mgroup + (bwidth - sbwidth) / 2, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g3tbmute->setToggleState(false, dontSendNotification);
            g3tbmute->onClick = [=]()
                {
                    if (g3tbmute->getToggleState()) {

                        g3tbmute->setButtonText("Muted");

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
                        auto ccMessage = juce::MidiMessage::controllerEvent(ptrbuttongroup->midiout, 7, 0);
                        // To do: Which one to use
                        //ccMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime);
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        g3svol->setEnabled(false);
                        setArrowButtonsMutedState(g3bvup, g3bvdwn, true);

                        ptrbuttongroup->setMuteButtonStatus(true);
                        ptrbuttongroup->isEffectDirty(0, 0);
                        updateVolumeSliderDebugText(g3svol, 0);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g3svol->getValue(), 0);
                        applyMutedGroupVisualCue(ptrbuttongroup, true);
                        g3uppermute = true;
                    }
                    else {

                        g3tbmute->setButtonText("Mute");

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
                        ptrbuttongroup->setMuteButtonStatus(false);
                        applyMutedGroupVisualCue(ptrbuttongroup, false);
                        g3svol->setEnabled(true);
                        setArrowButtonsMutedState(g3bvup, g3bvdwn, false);

                        const int activeButtonIdx = ptrbuttongroup->getActiveVoiceButton();
                        const int effectiveCc7 = computeEffectiveCc7ForButton(ptrbuttongroup, g3svol, activeButtonIdx);
                        updateVolumeSliderDebugText(g3svol, effectiveCc7);
                        sendCombinedGroupVolume(ptrbuttongroup, ptrbuttongroup->midiout, effectiveCc7);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g3svol->getValue(), effectiveCc7);
                        g3uppermute = false;
                    }

                };

            // Mute Button: Toggle Initial Button Mute Status keeping in mind Button Group and Tab Index
            int bmgroupidx = 2;
            if (tabidx == PTLower) { bmgroupidx = 6; }
            else if (tabidx == PTBass) { bmgroupidx = 10; }
            else { bmgroupidx = 2; }

            ptrbuttongroup = instrumentpanel->getButtonGroup(bmgroupidx);
            ptrbuttongroup->setMuteButtonPtr(g3tbmute);

            ptrbuttongroup = instrumentpanel->getButtonGroup(bmgroupidx);
            if (ptrbuttongroup->muted)
                g3tbmute->triggerClick();
        }   // End of Button Group 3


        // --------------------------------------------------------------------
        // Upper Solo Button Group 4
        // --------------------------------------------------------------------
        {
            int buttongroupidx = 3;
            if (tabidx == PTLower) {
                panelbuttonidx = 32 + 26;    // On Lower Panel
                buttongroupidx = 7;
            }
            else if (tabidx == PTBass) {
                panelbuttonidx = 64 + 26;   // On Bassa & Drums Panel
                buttongroupidx = 11;
            }
            else {
                panelbuttonidx = 0 + 26;     // On Upper Panel
                buttongroupidx = 3;
            }

            // Load Button Group Details
            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
            String buttongroupname = ptrbuttongroup->groupname;
            int buttongroupmidiout = ptrbuttongroup->midiout;
            String buttongrouptitle = formatButtonGroupTitle(*ptrbuttongroup, tabidx != PTBass);

            // Create the Group Volume slider as it is referenced by the Voice Buttons
            auto* g4svol = addToList(createVolSlider(false));

            g4xoffset = g3xoffset + g3width + xgroup;

            int cbuttons = 6;
            int rbuttons = 2;
            int bgroupid = 104;

            g4width = xgroup + mgroup + bwidth * (cbuttons / 2) + mgroup + swidth;
            int g4height = ygroup + mgroup * rbuttons + bheight * rbuttons + mgroup + sbheight;
            {
                auto* group = addToList(new GroupComponent("group", buttongrouptitle));

                group->setColour(GroupComponent::outlineColourId, getGroupOutlineColour(ptrbuttongroup));

                group->setBounds(g4xoffset, ygroup, g4width, g4height);

                ptrbuttongroup->setGroupComponentPtr(group);
            }

            // Create the Panel Buttons for Group 4
            int row = 0, col = 0;
            for (int i = 0; i < cbuttons; ++i)
            {
                // First or second row of button group
                if (i == cbuttons / rbuttons)
                {
                    row++;
                    col = 0;
                }

                Instrument instrument = instrumentpanel->getVoiceButton(panelbuttonidx)->getInstrument();
                instrument.setChannel(buttongroupmidiout);

                auto* sb = addToList(new VoiceButton(instrument.getVoice()));
                instrumentpanel->getVoiceButton(panelbuttonidx)->setDisplayButtonPtr(sb);

                sb->setInstrument(instrument);
                sb->setButtonText(instrument.getVoice());

                sb->setButtonGroupId(bgroupid);
                sb->setButtonId(i);
                sb->setPanelButtonIdx(panelbuttonidx);
                sb->setExplicitUserClickHandler([this](int idx) { registerExplicitVoiceSelection(idx); });

                sb->setClickingTogglesState(true);

                sb->setRadioGroupId(bgroupid);
                sb->setColour(TextButton::textColourOffId, Colours::black);
                sb->setColour(TextButton::textColourOnId, Colours::black);
                sb->setColour(TextButton::buttonColourId, Colours::lightgrey);
                sb->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
                sb->setBounds(g4xoffset + mgroup + col * bwidth, ygroup + mgroup * 2 + row * bheight, bwidth, bheight);
                sb->setConnectedEdges(((col != 0) ? Button::ConnectedOnLeft : 0)
                    | ((col != (cbuttons / rbuttons - 1)) ? Button::ConnectedOnRight : 0));
                sb->onClick = [=]()
                    {
                        // If the button is already on, do not send another change in addition to original down
                        // New button pressed is now toggling this one off. 
                        // To do: In future may actually use repeat toggle to turn all buttons in group off == mute group
                        if (!sb->getToggleState())
                            return;

                        DBG("VoiceButton.onClick(): Clicked " + std::to_string(sb->getPanelButtonIdx()));

                        // Remember last button we pressed in case we need to edit sound or effects
                        panelbuttonidx = sb->getPanelButtonIdx();

                        // Reload the instrument since the Voice may have been updated since original load
                        // and update the Button text if not done yet
                        Instrument instrument = instrumentpanel->getVoiceButton(panelbuttonidx)->getInstrument();

                        int panelgroup = lookupPanelGroup(panelbuttonidx);
                        ButtonGroup* selectedButtonGroup = instrumentpanel->getButtonGroup(panelgroup);
                        int buttongroupmidiout = selectedButtonGroup->midiout;

                        // Remember active Voice Button in Group
                        selectedButtonGroup->setActiveVoiceButton(panelbuttonidx);

                        sb->setButtonText(instrument.getVoice());

                        int controllerNumber = CCMSB;    // MSB
                        juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
                            buttongroupmidiout,
                            controllerNumber,
                            instrument.getMSB()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        // Send the MIDI CC message to MIDI output(s)
                        //mididevices->sendMessageNow(ccMessage);
                        mididevices->sendToOutputs(ccMessage);

                        controllerNumber = CCLSB;    // LSB
                        ccMessage = juce::MidiMessage::controllerEvent(
                            buttongroupmidiout,
                            controllerNumber,
                            instrument.getLSB()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        // Send PC
                        ccMessage = juce::MidiMessage::programChange(
                            buttongroupmidiout,
                            instrument.getFont()
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        //---------------------------------------------------------
                        // Only send Effects if they have been modified in this app
                        const int channelvolume = computeEffectiveCc7ForButton(selectedButtonGroup, g4svol, panelbuttonidx);
                        updateVolumeSliderDebugText(g4svol, channelvolume);
                        sendCombinedGroupVolume(selectedButtonGroup, buttongroupmidiout, channelvolume);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g4svol->getValue(), channelvolume);

                        // Send Exp
                        if (selectedButtonGroup->isEffectDirty(1, instrument.getExp()) == true) {
                            //if (isdirty & MAPEXP) {
                            controllerNumber = CCExp;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                instrument.getChannel(),
                                controllerNumber,
                                instrument.getExp()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Rev
                        if (selectedButtonGroup->isEffectDirty(2, instrument.getRev()) == true) {
                            //if (isdirty & MAPREV) {
                            controllerNumber = CCRev;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                instrument.getChannel(),
                                controllerNumber,
                                instrument.getRev()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Cho
                        if (selectedButtonGroup->isEffectDirty(3, instrument.getCho()) == true) {
                            //if (isdirty & MAPCHO) {
                            controllerNumber = CCMod;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                instrument.getChannel(),
                                controllerNumber,
                                instrument.getMod()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Mod
                        if (selectedButtonGroup->isEffectDirty(4, instrument.getMod()) == true) {
                            //if (isdirty & MAPMOD) {
                            controllerNumber = CCMod;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                instrument.getChannel(),
                                controllerNumber,
                                instrument.getMod()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Tim
                        if (selectedButtonGroup->isEffectDirty(5, instrument.getTim()) == true) {
                            //if (isdirty & MAPTIM) {
                            controllerNumber = CCTim;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                instrument.getChannel(),
                                controllerNumber,
                                instrument.getTim()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Atk
                        if (selectedButtonGroup->isEffectDirty(6, instrument.getMod()) == true) {
                            //if (isdirty & MAPATK) {
                            controllerNumber = CCAtk;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                instrument.getChannel(),
                                controllerNumber,
                                instrument.getAtk()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Rel
                        if (selectedButtonGroup->isEffectDirty(7, instrument.getRel()) == true) {
                            //if (isdirty & MAPREL) {
                            controllerNumber = CCRel;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                instrument.getChannel(),
                                controllerNumber,
                                instrument.getRel()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Bri
                        if (selectedButtonGroup->isEffectDirty(8, instrument.getBri()) == true) {
                            //if (isdirty & MAPBRI) {
                            controllerNumber = CCBri;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                instrument.getChannel(),
                                controllerNumber,
                                instrument.getBri()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }

                        // Send Pan
                        if (selectedButtonGroup->isEffectDirty(9, instrument.getPan()) == true) {
                            //if (isdirty & MAPPAN) {
                            controllerNumber = CCPan;
                            ccMessage = juce::MidiMessage::controllerEvent(
                                instrument.getChannel(),
                                controllerNumber,
                                instrument.getPan()
                            );
                            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                            mididevices->sendToOutputs(ccMessage);
                        }
                    };

                    sb->addMouseListener(this, true);

                // Default first button to send MIDI program change
                if (i == 0)
                    sb->triggerClick();

                panelbuttonidx++;
                col++;
            }

            // Create Volume Slider
            g4svol->setSliderStyle(Slider::LinearBarVertical);
            g4svol->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            g4svol->setBounds(g4xoffset + mgroup + col * bwidth + mgroup, ygroup + mgroup * 2, 20, mgroup + bheight * rbuttons + sbheight);
            g4svol->setPopupDisplayEnabled(true, true, this);
            g4svol->setValue(ptrbuttongroup->getMasterVolStep());
            g4svol->onValueChange = [=]()
                {
                    ButtonGroup* selectedButtonGroup = instrumentpanel->getButtonGroup(buttongroupidx);
                    selectedButtonGroup->setMasterVolStep((int)g4svol->getValue());

                    const int activeButtonIdx = selectedButtonGroup->getActiveVoiceButton();
                    const int effectiveCc7 = computeEffectiveCc7ForButton(selectedButtonGroup, g4svol, activeButtonIdx);
                    updateVolumeSliderDebugText(g4svol, effectiveCc7);
                    sendCombinedGroupVolume(selectedButtonGroup, selectedButtonGroup->midiout, effectiveCc7);
                    updateVolumeStatusLine(statusLabel, buttongroupname, g4svol->getValue(), effectiveCc7);
                };

            // Slider Volume up and down buttons
            auto* g4bvup = addToList(new ArrowCommandButton(ArrowCommandButton::Direction::up));
            configureArrowButton(g4bvup);
            g4bvup->setBounds(g4xoffset + mgroup + (bwidth - sbwidth) / 2 + (cbuttons / rbuttons - 2) * bwidth, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g4bvup->onClick = [=]()
                {
                    //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Status", strtest /*"The button was clicked!"*/);

                    vbuttonupClicked(g4svol);
                    g4bvup->setToggleState(false, dontSendNotification);
                };

            auto* g4bvdwn = addToList(new ArrowCommandButton(ArrowCommandButton::Direction::down));
            configureArrowButton(g4bvdwn);
            g4bvdwn->setBounds(g4xoffset + mgroup + (bwidth - sbwidth) / 2 + (cbuttons / rbuttons - 1) * bwidth, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g4bvdwn->onClick = [=]()
                {
                    //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Status", strtest /*"The button was clicked!"*/);

                    vbuttondownClicked(g4svol);
                    g4bvdwn->setToggleState(false, dontSendNotification);
                };

            // Add Sound Mute Button
            auto* g4tbmute = addToList(new MuteButton("Mute"));
            g4tbmute->setClickingTogglesState(true);
            g4tbmute->setColour(TextButton::textColourOffId, Colours::white);
            g4tbmute->setColour(TextButton::textColourOnId, Colours::black);
            g4tbmute->setColour(TextButton::buttonColourId, Colours::black.darker());
            g4tbmute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            g4tbmute->setBounds(g4xoffset + mgroup + (bwidth - sbwidth) / 2, mgroup * 4 + rbuttons * bheight, sbwidth, sbheight);
            g4tbmute->setToggleState(false, dontSendNotification);
            g4tbmute->onClick = [=]()
                {
                    if (g4tbmute->getToggleState()) {

                        g4tbmute->setButtonText("Muted");

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
                        auto ccMessage = juce::MidiMessage::controllerEvent(ptrbuttongroup->midiout, 7, 0);
                        // To do: Which one to use
                        //ccMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime);
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        g4svol->setEnabled(false);
                        setArrowButtonsMutedState(g4bvup, g4bvdwn, true);

                        ptrbuttongroup->setMuteButtonStatus(true);
                        ptrbuttongroup->isEffectDirty(0, 0);
                        updateVolumeSliderDebugText(g4svol, 0);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g4svol->getValue(), 0);
                        applyMutedGroupVisualCue(ptrbuttongroup, true);
                        g4uppermute = true;
                    }
                    else {

                        g4tbmute->setButtonText("Mute");

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
                        ptrbuttongroup->setMuteButtonStatus(false);
                        applyMutedGroupVisualCue(ptrbuttongroup, false);
                        g4svol->setEnabled(true);
                        setArrowButtonsMutedState(g4bvup, g4bvdwn, false);

                        const int activeButtonIdx = ptrbuttongroup->getActiveVoiceButton();
                        const int effectiveCc7 = computeEffectiveCc7ForButton(ptrbuttongroup, g4svol, activeButtonIdx);
                        updateVolumeSliderDebugText(g4svol, effectiveCc7);
                        sendCombinedGroupVolume(ptrbuttongroup, ptrbuttongroup->midiout, effectiveCc7);
                        updateVolumeStatusLine(statusLabel, buttongroupname, g4svol->getValue(), effectiveCc7);
                        g4uppermute = false;
                    }

                };

            // Mute Button: Toggle Initial Button Mute Status keeping in mind Button Group and Tab Index
            int bmgroupidx = 3;
            if (tabidx == PTLower) { bmgroupidx = 7; }
            else if (tabidx == PTBass) { bmgroupidx = 11; }
            else { bmgroupidx = 3; }

            ptrbuttongroup = instrumentpanel->getButtonGroup(bmgroupidx);
            ptrbuttongroup->setMuteButtonPtr(g4tbmute);

            ptrbuttongroup = instrumentpanel->getButtonGroup(bmgroupidx);
            if (ptrbuttongroup->muted)
                g4tbmute->triggerClick();
        }   // End of Button Group 4

        // --------------------------------------------------------------------
        // Button Voices and Effects Changes Group
        // Establish if on Upper or Lower Tabs to enable correct out channel
        if ((tabidx != PTUpper) || (tabidx != PTLower) || (tabidx != PTBass))
        {
            String buttongrouptitle = "Voice Edits";

            int g6xoffset = mgroup + 180;
            ygroup = 190;

            int g6width = mgroup + bwidth * 2 + mgroup;
            int g6height = mgroup * 3 + bheight;
            {
                auto* group = addToList(new GroupComponent("group", buttongrouptitle));
                group->setBounds(g6xoffset, ygroup, g6width, g6height);
            }

            // Add Sound Change Button
            auto* gxtbchange = addToList(new TextButton("Sounds"));
            gxtbchange->setClickingTogglesState(false);
            gxtbchange->setColour(TextButton::textColourOffId, Colours::black);
            gxtbchange->setColour(TextButton::textColourOnId, Colours::black);
            gxtbchange->setColour(TextButton::buttonColourId, Colours::lightgrey);
            gxtbchange->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            gxtbchange->setConnectedEdges(Button::ConnectedOnRight);
            gxtbchange->setBounds(g6xoffset + mgroup, ygroup + mgroup * 2, bwidth, bheight);
            gxtbchange->setToggleState(false, dontSendNotification);
            gxtbchange->onClick = [=, &tabs, &voicespage]()
                {
                    bpendingSoundEdit = true;
                    refreshPanelSaveAvailability();

                    //String strstatus = "Change Sound Button";
                    //statusLabel->setText(strstatus, juce::dontSendNotification);

                    // Lookup the Group ID for the selected Pane Button to preset Effects
                    int panelgroup = lookupPanelGroup(panelbuttonidx);

                    ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(panelgroup);
                    String panelgroupname = ptrbuttongroup->groupname;
                    int panelgroupmidiin = ptrbuttongroup->midiin;
                    int panelgroupmidiout = ptrbuttongroup->midiout;
                    String panelgrouptitle = panelgroupname + "   [In:" + std::to_string(panelgroupmidiin) + " | Out:" + std::to_string(panelgroupmidiout) + "]";

                    String sfilename = instrumentmodules->getFileName(ptrbuttongroup->moduleidx);
                    voicespage.setPanelButton(panelbuttonidx, panelgrouptitle, sfilename, panelgroupmidiout, currenttabidx);

                    // Populate the Effects Page with last Button Instrument pressed and switch to tab
                    tabs.setCurrentTabIndex(PTVoices, true);
                    //String sname = tabs->getCurrentTabName();
                };

            // Add Sound Effects Edit Button
            auto* gxtbedit = addToList(new TextButton("Effects"));
            gxtbedit->setClickingTogglesState(false);
            gxtbedit->setColour(TextButton::textColourOffId, Colours::black);
            gxtbedit->setColour(TextButton::textColourOnId, Colours::black);
            gxtbedit->setColour(TextButton::buttonColourId, Colours::lightgrey);
            gxtbedit->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            gxtbedit->setConnectedEdges(Button::ConnectedOnLeft);
            gxtbedit->setBounds(g6xoffset + mgroup + bwidth, ygroup + mgroup * 2, bwidth, bheight);
            gxtbedit->setToggleState(false, dontSendNotification);
            gxtbedit->onClick = [=, &tabs, &effectspage]()
                {
                    bpendingEffectsEdit = true;
                    refreshPanelSaveAvailability();

                    //String strstatus = "Edit Sound Button";
                    //statusLabel->setText(strstatus, juce::dontSendNotification);

                    // Lookup the Group ID for the selected Pane Button to preset Effects
                    int panelgroup = lookupPanelGroup(panelbuttonidx);

                    ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(panelgroup);
                    String panelgroupname = ptrbuttongroup->groupname;
                    int panelgroupmidiin = ptrbuttongroup->midiin;
                    int panelgroupmidiout = ptrbuttongroup->midiout;
                    String panelgrouptitle = panelgroupname + "   [In:" + std::to_string(panelgroupmidiin) + " | Out:" + std::to_string(panelgroupmidiout) + "]";

                    // Populate the Effects Page with last Button Instrument pressed and switch to tab
                    effectspage.setPanelButton(panelbuttonidx, panelgrouptitle, panelgroupmidiout, currenttabidx);

                    tabs.setCurrentTabIndex(PTEffects, true);
                    //String sname = tabs->getCurrentTabName();
                };

            voiceEditSoundsButton = gxtbchange;
            voiceEditEffectsButton = gxtbedit;
            gxtbchange->setEnabled(false);
            gxtbedit->setEnabled(false);
        }

        // --------------------------------------------------------------------
        // Preset Button Group 
        // Show Preset Button Group on Upper Keyboard only initially
        if ((tabidx == PTUpper) || (tabidx == PTLower) || (tabidx == PTBass))
        {
            int buttongroupidx = 0;
            if (tabidx == PTLower) {
                panelbuttonidx = 32;    // On Lower Panel
                buttongroupidx = 4;
            }
            else if (tabidx == PTBass) {
                panelbuttonidx = 64;   // On Bass & Drums Panel
                buttongroupidx = 8;
            }
            else {
                panelbuttonidx = 0;     // On Upper Panel
                buttongroupidx = 0;
            }

            // Load Button Group Details
            String presetgrouptitle = "Presets (All Keyboards)";

            int g10xoffset = 370; //xgroup;
            ygroup = 190;

            const int cbuttons = numberpresets + 2; // Set + Preset(0..6) + Next Preset
            int rbuttons = 1;
            int bgroupid = 110;

            int g10width = mgroup + bwidth * cbuttons + mgroup;
            int g10height = mgroup * 3 + bheight;
            {
                auto* group = addToList(new GroupComponent("group", presetgrouptitle));
                group->setBounds(g10xoffset, ygroup, g10width, g10height);
            }

            int col = 0;

            // Manual Preset Set/Unset
            const int presetControlButtonWidth = bwidth;

            auto* tbsetpreset = addToList(new PresetButton("Set"));
            tbsetpreset->setClickingTogglesState(true);
            tbsetpreset->setColour(TextButton::textColourOffId, Colours::white);
            tbsetpreset->setColour(TextButton::textColourOnId, Colours::black);
            tbsetpreset->setColour(TextButton::buttonColourId, Colours::black.darker());
            tbsetpreset->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            tbsetpreset->setTooltip("Toggle Set mode to store changes into the selected preset.");
            tbsetpreset->setBounds(g10xoffset + mgroup + col * bwidth, ygroup + mgroup * 2, presetControlButtonWidth, bheight);
            tbsetpreset->setConnectedEdges(Button::ConnectedOnRight);
            tbsetpreset->onClick = [=]()
                {
                    if (tbsetpreset->getToggleState()) {
                        bsetpreset = true;
                        bpendingPresetSet = true;
                        refreshPanelSaveAvailability();

                        DBG("PresetButton.onClick(): Set");
                    }
                    else {
                        bsetpreset = false;

                        DBG("PresetButton.onClick(): Unset");
                    }
                };

            // Create the Panel Buttons for Group 1
            col = 1;
            for (int i = 0; i < numberpresets; ++i)
            {
                auto* pstb = addToList(new PresetButton());

                presetbuttons.add(pstb);
                presetbuttons[i]->setPresetButtonPtr(pstb);

                pstb->setButtonId(i);
                pstb->setRadioGroupId(bgroupid);
                pstb->setClickingTogglesState(true);

                if (i == 0) {
                    pstb->setButtonText("Manual");
                    pstb->setColour(TextButton::textColourOffId, Colours::white);
                    pstb->setColour(TextButton::textColourOnId, Colours::white);
                    pstb->setColour(TextButton::buttonColourId, Colours::darkred.brighter());
                    pstb->setColour(TextButton::buttonOnColourId, Colours::darkred);
                }
                else {
                    pstb->setButtonText("Preset " + std::to_string(i));
                    pstb->setColour(TextButton::textColourOffId, Colours::black);
                    pstb->setColour(TextButton::textColourOnId, Colours::black);
                    pstb->setColour(TextButton::buttonColourId, Colours::lightgrey);
                    pstb->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
                }
                pstb->setBounds(g10xoffset + mgroup + col * bwidth, ygroup + mgroup * 2, bwidth, bheight);
                pstb->setConnectedEdges(((col != 0) ? Button::ConnectedOnLeft : 0)
                    | ((col != (cbuttons / rbuttons - 1)) ? Button::ConnectedOnRight : 0));
                pstb->onClick = [=]()
                    {
                        // If the button is already on, do not send another change in addition to original down
                        if (pstb->getToggleState()) 
                        {
                            // Soft Trigger after we changed tabs and decided up show (only!!!)
                            // last Preset button pressed
                            if (tabchanged == true) {
                                DBG("PresetButton.onClick(): Soft exit Preset " + std::to_string(i));

                                tabchanged = false;
                                return;
                            }

                            // Remember last preset so we can duplicate and show it 
                            // as pressed in Upper, Lower, and Bass&Drums keyboards.
                            instrumentpanel->panelpresets->setActivePresetIdx(pstb->getButtonId());

                            // Update the selected Preset if Set Button is true
                            if (bsetpreset == true) {
                                savePreset(pstb->getButtonId());

                                tbsetpreset->triggerClick();
                                bsetpreset = false;

                                DBG("PresetButton.onClick(): Saved " + std::to_string(i));
                            }
                            // Recall an existig Preset setting and update the Panel
                            else {
                                loadPreset(pstb->getButtonId());

                                DBG("PresetButton.onClick(): Recall " + std::to_string(i));
                            }
                        }
                    };

                // Default first Preset Button (Manual)
                if (i == 0)
                  pstb->triggerClick();

                col++;
            }

            auto* tbnextpreset = addToList(new PresetButton("Next"));
            tbnextpreset->setClickingTogglesState(false);
            tbnextpreset->setColour(TextButton::textColourOffId, Colours::white);
            tbnextpreset->setColour(TextButton::textColourOnId, Colours::white);
            tbnextpreset->setColour(TextButton::buttonColourId, Colours::black.darker());
            tbnextpreset->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            tbnextpreset->setTooltip("Advance to the next preset (Manual and Preset 6 both go to Preset 1).");
            tbnextpreset->setBounds(g10xoffset + mgroup + col * bwidth, ygroup + mgroup * 2, presetControlButtonWidth, bheight);
            tbnextpreset->setConnectedEdges(Button::ConnectedOnLeft);
            tbnextpreset->onClick = [=]()
                {
                    triggerNextPresetFromCurrentSelection();
                };
        }   //End - Preset Button Group

        // --------------------------------------------------------------------
        // Rotary Button Group
        // Only use Rotary for Upper and Lower Channels, 
        // Do not show on Bass Keyboard
        if  (tabidx != PTBass)
        {
            int buttongroupidx = 0;
            if (tabidx == PTLower) {
                buttongroupidx = 4;
            }
            else if (tabidx == PTUpper) {
                buttongroupidx = 0;
            }
            else {
                //// To do: Add back after testing and remove the line below! Replace with return to 
                //// not show Rotary on Bass&Drums tab;
                buttongroupidx = 8;
            }

            // Load Button Group Details for In and Out Channels
            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
            int buttongroupmidiin = ptrbuttongroup->midiin;
            int buttongroupmidiout = ptrbuttongroup->midiout;
            String buttongrouptitle = "Rotary   [In:" + std::to_string(buttongroupmidiin) + " | Out:" + std::to_string(buttongroupmidiout) + "]";

            int g5xoffset = mgroup;
            ygroup = 190;

            int g5width = mgroup + bwidth * 2 + mgroup;
            int g5height = mgroup * 3 + bheight;
            {
                auto* group = addToList(new GroupComponent("group", buttongrouptitle));
                group->setBounds(g5xoffset, ygroup, g5width, g5height);
            }

            tbrotslow = addToList(new CommandButton());
            tbrotslow->setButtonText("Fast");
            tbrotslow->setClickingTogglesState(true);
            tbrotslow->setColour(TextButton::textColourOffId, Colours::black);
            tbrotslow->setColour(TextButton::textColourOnId, Colours::black);
            tbrotslow->setColour(TextButton::buttonOnColourId, Colours::lightgrey);
            tbrotslow->setColour(TextButton::buttonColourId, Colours::antiquewhite);
            tbrotslow->setBounds(g5xoffset + mgroup + bwidth, ygroup + mgroup * 2, bwidth, bheight);
            tbrotslow->setConnectedEdges(Button::ConnectedOnLeft);
            tbrotslow->setToggleState(false, dontSendNotification);
            tbrotslow->onClick = [=]()
                {
                    int buttongroupidx = 0;
                    if (tabidx == PTLower) {
                        buttongroupidx = 4;
                    }
                    else if (tabidx == PTUpper) {
                        buttongroupidx = 0;
                    }

                    ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
                    int buttongroupmoduleidx = ptrbuttongroup->moduleidx;
                    int buttongroupmidiout = ptrbuttongroup->midiout;

                    if (!tbrotslow->getToggleState())
                    {
                        // Ignore Fast/Slow command if Rotor brake on
                        if (rotarybrake) return;

                        if (instrumentmodules->getRotorType(buttongroupmoduleidx) == 1) {
                            // Preset Rotor change control value to fast
                            RotaryFastSlow(buttongroupmidiout,
                                instrumentmodules->getRotorCC(buttongroupmoduleidx),        //74
                                instrumentmodules->getRotorFast(buttongroupmoduleidx)
                            );
                        }
                        else if (instrumentmodules->getRotorType(buttongroupmoduleidx) == 2) {
                            rotorsarray.add(new RotaryFastThread(*this, buttongroupmidiout));
                        }

                        tbrotslow->setButtonText("Fast");
                        isFastUpper = true;
                    }
                    else
                    {
                        if (rotarybrake) return;

                        if (instrumentmodules->getRotorType(buttongroupmoduleidx) == 1) {
                            // Preset Rotor change control value to slow
                            RotaryFastSlow(buttongroupmidiout,
                                instrumentmodules->getRotorCC(buttongroupmoduleidx),        //74
                                instrumentmodules->getRotorSlow(buttongroupmoduleidx)
                            );
                        }
                        else if (instrumentmodules->getRotorType(buttongroupmoduleidx) == 2) {
                            rotorsarray.add(new RotarySlowThread(*this, buttongroupmidiout));
                        }

                        tbrotslow->setButtonText("Slow");
                        isFastUpper = false;
                    }

                    commitManualRotaryToAppState();
                    {
                        const int enc = encodePresetRotaryFromManual(isFastUpper, rotarybrake);
                        const int pst = juce::jlimit(0, numberpresets - 1, instrumentpanel->panelpresets->getActivePresetIdx());
                        if (tabidx == PTUpper) {
                            for (int gg = 0; gg <= 3; ++gg) {
                                instrumentpanel->getButtonGroup(gg)->rotary = enc;
                                instrumentpanel->panelpresets->setRotaryStatus(pst, gg, enc);
                            }
                        }
                        else if (tabidx == PTLower) {
                            for (int gg = 4; gg <= 7; ++gg) {
                                instrumentpanel->getButtonGroup(gg)->rotary = enc;
                                instrumentpanel->panelpresets->setRotaryStatus(pst, gg, enc);
                            }
                        }
                    }
                };

            tbrotbrake = addToList(new CommandButton());
            tbrotbrake->setButtonText("Brake Off");
            tbrotbrake->setClickingTogglesState(true);
            tbrotbrake->setColour(TextButton::textColourOffId, Colours::black);
            tbrotbrake->setColour(TextButton::textColourOnId, Colours::black);
            tbrotbrake->setColour(TextButton::buttonColourId, Colours::lightgrey);
            tbrotbrake->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            tbrotbrake->setBounds(g5xoffset + mgroup, ygroup + mgroup * 2, bwidth, bheight);
            tbrotbrake->setConnectedEdges(Button::ConnectedOnRight);
            tbrotbrake->setToggleState(false, dontSendNotification);
            tbrotbrake->onClick = [=]()
                {
                    int buttongroupidx = 0;
                    if (tabidx == PTLower) {
                        buttongroupidx = 4;
                    }
                    else if (tabidx == PTUpper) {
                        buttongroupidx = 0;
                    }

                    ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
                    int buttongroupmoduleidx = ptrbuttongroup->moduleidx;
                    int buttongroupmidiout = ptrbuttongroup->midiout;

                    if (tbrotbrake->getToggleState())
                    {
                        // Set Rotor Brake on
                        RotaryFastSlow(buttongroupmidiout,
                            instrumentmodules->getRotorCC(buttongroupmoduleidx),        //74
                            instrumentmodules->getRotorOff(buttongroupmoduleidx)
                        );

                        tbrotbrake->setButtonText("Brake On");
                        rotarybrake = true;

                        tbrotslow->setEnabled(false);     // Disable rotor fast/slow changes
                    }
                    else
                    {
                        // Set Rotor Brake off and restart Rotor at previous Fast or Slow
                        if (isFastUpper) {
                            RotaryFastSlow(buttongroupmidiout,
                                instrumentmodules->getRotorCC(buttongroupmoduleidx),
                                instrumentmodules->getRotorFast(buttongroupmoduleidx)
                            );
                        }
                        else {
                            RotaryFastSlow(buttongroupmidiout,
                                instrumentmodules->getRotorCC(buttongroupmoduleidx),
                                instrumentmodules->getRotorSlow(buttongroupmoduleidx)
                            );
                        }

                        tbrotbrake->setButtonText("Brake Off");
                        rotarybrake = false;

                        tbrotslow->setEnabled(isrotary);     // Enable rotor fast/slow changes when module supports it
                    }

                    commitManualRotaryToAppState();
                    {
                        const int enc = encodePresetRotaryFromManual(isFastUpper, rotarybrake);
                        const int pst = juce::jlimit(0, numberpresets - 1, instrumentpanel->panelpresets->getActivePresetIdx());
                        if (tabidx == PTUpper) {
                            for (int gg = 0; gg <= 3; ++gg) {
                                instrumentpanel->getButtonGroup(gg)->rotary = enc;
                                instrumentpanel->panelpresets->setRotaryStatus(pst, gg, enc);
                            }
                        }
                        else if (tabidx == PTLower) {
                            for (int gg = 4; gg <= 7; ++gg) {
                                instrumentpanel->getButtonGroup(gg)->rotary = enc;
                                instrumentpanel->panelpresets->setRotaryStatus(pst, gg, enc);
                            }
                        }
                    }
                };

            applyManualRotaryFromAppState();
        }   // End - Rotary Group

        // Quick Access Keyboard Buttons
        int xaccess = 1070;
        {
            if ((tabidx == PTLower) || (tabidx == PTBass)) {
                auto* toUpperKBD = addToList(new CommandButton());
                toUpperKBD->setButtonText("Upper");
                toUpperKBD->setClickingTogglesState(false);
                toUpperKBD->setColour(TextButton::textColourOffId, Colours::black);
                toUpperKBD->setColour(TextButton::textColourOnId, Colours::white);
                toUpperKBD->setColour(TextButton::buttonColourId, Colours::white);
                toUpperKBD->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
                toUpperKBD->setTooltip("Switch to the Upper tab.");
                toUpperKBD->setBounds(mgroup + xaccess, ygroup + mgroup * 2, 80, 50);
                toUpperKBD->setToggleState(true, dontSendNotification);
                toUpperKBD->onClick = [=, &tabs]()
                    {
                        tabs.setCurrentTabIndex(PTUpper, false);
                    };
                xaccess = xaccess + 90;
            }

            if ((tabidx == PTUpper) || (tabidx == PTBass)) {
                auto* toLowerKBD = addToList(new CommandButton());
                toLowerKBD->setButtonText("Lower");
                toLowerKBD->setClickingTogglesState(false);
                toLowerKBD->setColour(TextButton::textColourOffId, Colours::black);
                toLowerKBD->setColour(TextButton::textColourOnId, Colours::white);
                toLowerKBD->setColour(TextButton::buttonColourId, Colours::white);
                toLowerKBD->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
                toLowerKBD->setTooltip("Switch to the Lower tab.");
                toLowerKBD->setBounds(mgroup + xaccess, ygroup + mgroup * 2, 80, 50);
                toLowerKBD->setToggleState(true, dontSendNotification);
                toLowerKBD->onClick = [=, &tabs]()
                    {
                        tabs.setCurrentTabIndex(PTLower, false);
                    };
                xaccess = xaccess + 90;
            }

            if ((tabidx == PTUpper) || (tabidx == PTLower)) {
                auto* toBassKBD = addToList(new CommandButton());
                toBassKBD->setButtonText("Bass");
                toBassKBD->setClickingTogglesState(false);
                toBassKBD->setColour(TextButton::textColourOffId, Colours::black);
                toBassKBD->setColour(TextButton::textColourOnId, Colours::white);
                toBassKBD->setColour(TextButton::buttonColourId, Colours::white);
                toBassKBD->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
                toBassKBD->setTooltip("Switch to the Bass&Drums tab.");
                toBassKBD->setBounds(mgroup + xaccess, ygroup + mgroup * 2, 80, 50);
                toBassKBD->setToggleState(true, dontSendNotification);
                toBassKBD->onClick = [=, &tabs]()
                    {
                        tabs.setCurrentTabIndex(PTBass, false);
                    };
            }

        }

        // --------------------------------------------------------------------
        // Instrument Panel Save and Save As Buttons
        if ((tabidx != PTUpper) || (tabidx != PTLower) || (tabidx != PTBass))
        {
            auto* tbSave = addToList(new CommandButton());
            cbSave = tbSave;
            tbSave->setButtonText("Save");
            tbSave->setColour(TextButton::textColourOffId, Colours::white);
            tbSave->setColour(TextButton::textColourOnId, Colours::white);
            tbSave->setColour(TextButton::buttonColourId, Colours::black.darker());
            tbSave->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            tbSave->setTooltip("Save the current panel to the existing file.");
            tbSave->setBounds(mgroup + 1260, 235, 80, 30);
            tbSave->setToggleState(false, dontSendNotification);
            tbSave->onClick = [=]()
                {
                    DBG("*** KeyboardManualPage(): Saving Panel");

                    //bool bsaved = instrumentpanel->saveInstrumentPanel(appState.instrumentdname, appState.panelfname);
                    bool bsaved = instrumentpanel->saveInstrumentPanel(appState.panelfullpathname);

                    String msgloaded;
                    if (!bsaved) {
                        msgloaded = "Save failed: " + appState.panelfname;
                        juce::Logger::writeToLog("*** KeyboardManualPage(): Save failed! " + appState.panelfname);
                    }
                    else {
                        msgloaded = "Saved: " + appState.panelfname;
                        juce::Logger::writeToLog("*** KeyboardManualPage(): Saved " + appState.panelfname);
                    }

                    if (TextButton* focused = tbSave)
                        BubbleMessage(*focused, msgloaded, this->bubbleMessage);

                    if (bsaved)
                        clearPendingExitSavePrompt();
                    refreshPanelSaveAvailability();
                };
            tbSave->setEnabled(false);

            auto* tbSaveAs = addToList(new CommandButton());
            cbSaveAs = tbSaveAs;
            tbSaveAs->setButtonText("Save As");
            tbSaveAs->setColour(TextButton::textColourOffId, Colours::white);
            tbSaveAs->setColour(TextButton::textColourOnId, Colours::white);
            tbSaveAs->setColour(TextButton::buttonColourId, Colours::black.darker());
            tbSaveAs->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            tbSaveAs->setTooltip("Save the current panel to a new file name.");
            tbSaveAs->setBounds(mgroup + 1360, 235, 80, 30);
            tbSaveAs->setToggleState(false, dontSendNotification);
            tbSaveAs->onClick = [=]()
                {
                    bool useNativeVersion = false;
                    const juce::File savePanelStart = appState.panelfullpathname.isNotEmpty()
                        ? juce::File(appState.panelfullpathname)
                        : getDefaultPanelsDirectory().getChildFile(
                            appState.panelfname.isNotEmpty() ? appState.panelfname : defpanelfname);

                    filechooser.reset(new FileChooser("Save Panel As: Select existing or create new *.pnl",
                        savePanelStart,
                        "*.pnl", useNativeVersion));

                    filechooser->launchAsync(
                        FileBrowserComponent::saveMode
                            | FileBrowserComponent::canSelectFiles
                            | FileBrowserComponent::warnAboutOverwriting,
                        [=](const FileChooser& chooser)
                        {
                            String selectedfname;

                            auto results = chooser.getURLResults();

                            for (auto result : results)
                                selectedfname << (result.isLocalFile() ? result.getLocalFile().getFileName()
                                    : result.toString(false));

                            if (selectedfname.length() != 0) {
                                int extpos = selectedfname.indexOf(0, ".");
                                if (extpos == -1) {
                                    DBG("*** Save Panel: Extension not defined as .pnl. Adding extension");

                                    selectedfname = selectedfname + ".pnl";
                                }

                                auto finishSaveAs = [this, tbSaveAs, selectedfname](bool allowMismatch)
                                    {
                                        const bool ok = instrumentpanel->saveInstrumentPanel(
                                            appState.instrumentdname, selectedfname, allowMismatch);

                                        String msgloaded;
                                        if (!ok) {
                                            msgloaded = "Save as failed: " + selectedfname + "\nConfig: " + appState.configfname;
                                            juce::Logger::writeToLog("*** KeyboardManualPage(): Save as failed! " + selectedfname);
                                        }
                                        else {
                                            msgloaded = "Saved as: " + selectedfname + "\nConfig: " + appState.configfname;
                                            juce::Logger::writeToLog("*** KeyboardManualPage(): Saved as " + selectedfname
                                                + " (embedded config: " + appState.configfname + ")");
                                            updatePanelFileStatusLabel();
                                        }

                                        if (TextButton* focused = tbSaveAs)
                                            BubbleMessage(*focused, msgloaded, this->bubbleMessage);

                                        if (ok)
                                            clearPendingExitSavePrompt();
                                        refreshPanelSaveAvailability();
                                    };

                                if (appState.configPanelPairingMismatchAcknowledged)
                                {
                                    auto safeKb = juce::Component::SafePointer<KeyboardPanelPage>(this);
                                    juce::AlertWindow::showOkCancelBox(
                                        juce::AlertWindow::WarningIcon,
                                        "Config / panel mismatch",
                                        getPanelSavePairingMismatchMessage(),
                                        "Save anyway",
                                        "Cancel",
                                        this,
                                        juce::ModalCallbackFunction::create([safeKb, finishSaveAs](int r)
                                            {
                                                if (safeKb == nullptr || r != 1)
                                                    return;
                                                finishSaveAs(true);
                                            }));
                                }
                                else
                                {
                                    finishSaveAs(false);
                                }
                            }
                            else {
                                statusLabel->setText("Empty file name save! " + selectedfname, juce::dontSendNotification);
                            }                            
                        });

                    tbSave->setEnabled(false);
                };
            updatePanelSaveButtonsPendingStyle();
        }   // End - Save and Save As Buttons

        lblpanelfile = addToList(new Label("Panel File", appState.panelfname));
        lblpanelfile->setColour(juce::Label::textColourId, juce::Colours::grey);
        lblpanelfile->setJustificationType(juce::Justification::left);
        lblpanelfile->setMinimumHorizontalScale(0.8f);
        lblpanelfile->setBounds(mgroup + 1260, 205, 200, 30);
        updatePanelFileStatusLabel();

    }   // End KeyboardManual Page Constructor

    // Button Group Voice Volume Up or Down Button clicked
    void vbuttonupClicked(Slider* svol)
    {
        if (svol->getValue() >= 9.0)
            svol->setValue(10.0);
        else
            svol->setValue(svol->getValue() + 1.0);    // add async call
    }

    void vbuttondownClicked(Slider* svol)
    {
        if (svol->getValue() <= 1.0)
            svol->setValue(0.0);
        else
            svol->setValue(svol->getValue() - 1.0);    // add async call
    }

    // Read active Voice Buttons on Keyboard Panel and save to selected Preset
    void savePreset(int pstidx) {

        // Read Active VoiceButtons for every Button Group (all 3 keyboards)
        for (int i = 0; i < numberbuttongroups; i++) {
            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(i);
            int pbtnidx = ptrbuttongroup->getActiveVoiceButton();

            instrumentpanel->panelpresets->setPanelButtonIdx(pstidx, i, pbtnidx);

            DBG("*** savePreset(): Button " + std::to_string(pbtnidx));
        }

        // Read Mute Status for every Button Group 
        for (int i = 0; i < numberbuttongroups; i++) {
            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(i);
            bool bmuted = ptrbuttongroup->muted;

            instrumentpanel->panelpresets->setMuteStatus(pstidx, i, bmuted);
        }

        // Read Rotary Status for every Button Group (0=slow, 1=fast, 2=brake — matches .pnl preset field)
        for (int i = 0; i < numberbuttongroups; i++) {
            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(i);
            instrumentpanel->panelpresets->setRotaryStatus(pstidx, i, ptrbuttongroup->rotary);
        }

        refreshPanelSaveAvailability();
    }

    // Preset Keyboard Panel Voice Buttons with selected Preset (all 3 keyboards)
    void loadPreset(int pstidx) {

        // Activate VoiceButtons for every Button Group and all 3 keyboards
        for (int i = 0; i < numberbuttongroups; i++) {

            int pbtnidx = instrumentpanel->panelpresets->getPanelButtonIdx(pstidx, i);

            VoiceButton* ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
            auto ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
            ptrdisplayvbutton->triggerClick();

            DBG("*** loadPreset(): Triggered Button Group " + std::to_string(i) + ", Sound Button " + std::to_string(pbtnidx));
        }

        // Activate Mute Status for every Button Group
        for (int i = 0; i < numberbuttongroups; i++) {
            bool bmuted = instrumentpanel->panelpresets->getMuteStatus(pstidx, i);

            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(i);
            auto ptrmutebutton = ptrbuttongroup->getMuteButtonPtr();

            if (ptrbuttongroup->getMuteButtonStatus() != bmuted) {
                ptrmutebutton->triggerClick();

                // Set new status
                ptrbuttongroup->setMuteButtonStatus(bmuted);

                DBG("*** loadPreset(): Mute Button " + std::to_string(i));
            }
            else if (bmuted) {
                // Voice triggerClick above may have sent non-zero volume; model/UI already say
                // muted so the inequality branch skipped. Re-assert CC7=0 (same as Mute onClick).
                auto ccMessage = juce::MidiMessage::controllerEvent(ptrbuttongroup->midiout, 7, 0);
                ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                mididevices->sendToOutputs(ccMessage);
            }
        }

        // Activate Rotary for every Button Group (snapshot + MIDI + Upper/Lower UI)
        for (int i = 0; i < numberbuttongroups; i++) {
            const int irotary = instrumentpanel->panelpresets->getRotaryStatus(pstidx, i);
            instrumentpanel->getButtonGroup(i)->rotary = juce::jlimit(0, 2, irotary);
        }

        instrumentpanel->sendMidiForStoredButtonGroupRotaryState();

        if (gNotifyPresetRotarySyncFromButtonGroups)
            gNotifyPresetRotarySyncFromButtonGroups();
    }

    // After Panel Load, update all the Buttons by triggering a click on every first button in a Group
    void updatePanelButtons() {

        DBG("*** updatePanelButtons(): Retriggering all Voice Buttons");

        // Upper Group 4
        int pbtnidx = 26;
        VoiceButton* ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        auto ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();
        // Upper Group 3
        pbtnidx = 20;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();
        // Upper Group 2
        pbtnidx = 12;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();
        // Upper Group 1
        pbtnidx = 0;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();

        // Lower Group 4
        pbtnidx = 58;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();
        // Lower Group 3
        pbtnidx = 52;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();
        // Lower Group 2
        pbtnidx = 44;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();
        // Lower Group 1
        pbtnidx = 32;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();

        // Bass Group 2
        pbtnidx = 76;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();
        // Bass Group 1
        pbtnidx = 64;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();

        // Drums Group 2
        pbtnidx = 90;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();
        // Drums Group 1
        pbtnidx = 84;
        ptrvbutton = instrumentpanel->getVoiceButton(pbtnidx);
        ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
        ptrdisplayvbutton->triggerClick();
    }

    //-----------------------------------------------------------------------------
    // Use Mouse Entry Listener to update Panel Voices after individual Voice update
    // or complete Panel Instrument voice updates 
    // https://melatonin.dev/blog/juce-component-mouse-and-keyboard/
    void addMouseListener(const juce::MouseEvent& event);

    void mouseEnter([[maybe_unused]] const juce::MouseEvent& event) override {
        DBG("*** KeyboardManualPage(): Mouse Event Enter " + event.eventComponent->getDescription());
    }

    // Trigger Button updates when voice updated or panel reloaded - not working!!!
    void focusGained([[maybe_unused]] const juce::MouseEvent& event) {
        DBG("*** KeyboardManualPage(): Focus Received " + event.eventComponent->getDescription());
    }

    void tabSelected([[maybe_unused]] const juce::MouseEvent& event) {
        DBG("*** KeyboardManualPage(): Tab Selected " + event.eventComponent->getDescription());
    }

    // --------------------------------------------------------------------
    // broughtToFront() - Triggers everytime we switch the keyboard tab
    // Proceeds to update the newly selected tab based on 1) single Voice/Sound
    // change, or 2) new Instrument Panel loaded, and/or 3) synchronize
    // the active Presets amongst tabs  
    // --------------------------------------------------------------------
    void broughtToFront() {

        DBG("*** KeyboardManualPage(): Brought To Front Tab " + std::to_string(currenttabidx));

        // If Rotary not available for the current Instrument Module, disable buttons
        if ((currenttabidx == PTUpper) || (currenttabidx == PTLower)) {
            if (isrotary == true) {
                tbrotslow->setEnabled(true);
                tbrotbrake->setEnabled(true);
            }
            else {
                tbrotslow->setEnabled(false);
                tbrotbrake->setEnabled(false);
            }
        }

        // Single Voice Update via VoicesPage
        if (bvoiceupdated) {
            VoiceButton* ptrvbutton = instrumentpanel->getVoiceButton(panelbuttonidx);
            auto ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
            ptrdisplayvbutton->triggerClick();

            bvoiceupdated = false;

            refreshPanelSaveAvailability();

            return;
        }

        // Effects cheanged via effects updatd
        else if (beffectsupdated) {
            beffectsupdated = false;

            refreshPanelSaveAvailability();

            return;
        }

        // All Voices Panel Update following Instrument Panel reload from disk
        else if (instrumentpanel->getPanelUpdated() == true) {

            DBG("*** KeyboardManualPage(): Brought To Front Updating Panel Button text " + std::to_string(currenttabidx));

            // Update Panel Voice Buttons with newly loaded Voices
            for (int pbidx = 0; pbidx < numbervoicebuttons; pbidx++) {
                VoiceButton* ptrvbutton = instrumentpanel->getVoiceButton(pbidx);
                auto ptrdisplayvbutton = ptrvbutton->getDisplayButtonPtr();
                ptrdisplayvbutton->setButtonText(instrumentpanel->getVoiceButton(pbidx)->getInstrument().getVoice());
            }

            // Apply Preset 0 = Manual to default Instrument Panel Voice Buttons
            loadPreset(0);
            appState.lastSelectedPanelButtonIdx = -1;
            appState.hasExplicitVoiceSelection = false;

            instrumentpanel->setPanelUpdated(false);

            return;
        }

        // Otherwise and in case we switched Tabs, ensure that the correct Preset is
        // highlighted. 
        // Note: Need to review if we should avoid a complete retrigger of Preset
        else if (currenttabidx != lasttabidx) {

            tabchanged = true;

            // Ensure right/active button is displayed, but do not trigger MIDI outputs
            int pstidx = instrumentpanel->panelpresets->getActivePresetIdx();
            PresetButton* presetbuttonptr = presetbuttons[pstidx]->getPresetButtonPtr();
            presetbuttonptr->triggerClick();

            lasttabidx = currenttabidx;
        }

        // Update every Voice Button Group Title if Config changes
        else if (appState.configchanged) {

            for (int bgidx = 0; bgidx < numberbuttongroups; bgidx++) {
                ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(bgidx);

                const bool includeSplit = ((bgidx == 3) || (bgidx == 7));
                String buttongrouptitle = formatButtonGroupTitle(*ptrbuttongroup, includeSplit);

                GroupComponent* ptrgroupcomponent = ptrbuttongroup->getGroupComponentPtr();
                ptrgroupcomponent->setText(buttongrouptitle);
            }

            appState.configchanged = false;
        }

        updatePanelFileStatusLabel();
        refreshPanelSaveAvailability();
    }

    //-----------------------------------------------------------------------------
    ~KeyboardPanelPage()
    {
        DBG("== KeyboardManualPage(): Destructor " + std::to_string(--zinstcntmanualpage));
    }

    /** True after the user has clicked a voice on this manual (Voice Edits buttons are enabled). */
    bool areVoiceEditShortcutButtonsEnabled() const
    {
        return voiceEditSoundsButton != nullptr && voiceEditSoundsButton->isEnabled();
    }

    /** Sounds/Effects tab hotkeys use the same rule as the Voice Edits row: allowed if any manual has enabled those buttons. */
    static bool anyVoiceEditShortcutsEnabledInTabs(juce::TabbedComponent& tabbed)
    {
        for (const int tabIdx : { PTUpper, PTLower, PTBass })
        {
            if (auto* k = dynamic_cast<KeyboardPanelPage*>(tabbed.getTabContentComponent(tabIdx)))
                if (k->areVoiceEditShortcutButtonsEnabled())
                    return true;
        }
        return false;
    }

    void refreshPanelSaveAvailability()
    {
        if (cbSave != nullptr)
        {
            const bool pending = hasPendingExitSavePrompt();
            const bool pairingOk = !appState.configPanelPairingMismatchAcknowledged;
            const bool canSave = pending && pairingOk;
            cbSave->setEnabled(canSave);

            if (canSave)
            {
                cbSave->setTooltip("Save the instrument panel to the current .pnl file.");
            }
            else if (!pairingOk)
            {
                cbSave->setTooltip(
                    "Config/panel mismatch (you chose Load anyway). Save is disabled here; use Save As, "
                    "or load a config and panel that match.");
            }
            else if (!pending)
            {
                cbSave->setTooltip("Save appears when you change sounds, effects, or preset set on the panel.");
            }
            else
            {
                cbSave->setTooltip({});
            }
        }

        if (cbSaveAs != nullptr)
        {
            if (appState.configPanelPairingMismatchAcknowledged)
            {
                cbSaveAs->setTooltip(
                    "A dialog will ask you to confirm; the saved panel will embed the current config file name.");
            }
            else
            {
                cbSaveAs->setTooltip("Save the panel under a new or existing .pnl file name.");
            }
        }

        updatePanelSaveButtonsPendingStyle();
    }

    /** After preset recall: update this tab's rotary controls from ButtonGroup::rotary (Upper=0, Lower=4). */
    void applyPresetRotaryUiFromStoredGroups()
    {
        if (currenttabidx == PTBass || tbrotslow == nullptr || tbrotbrake == nullptr)
            return;

        const int g = (currenttabidx == PTLower) ? 4 : 0;
        bool wantFast = false;
        bool wantBrake = false;
        decodePresetRotary(instrumentpanel->getButtonGroup(g)->rotary, wantFast, wantBrake);
        applyRotaryUiSnapshot(wantFast, wantBrake, true);
    }

    /** When sendMidi is false, only syncs widget state from AppState (e.g. main tab Upper/Lower switch). */
    void applyManualRotaryFromAppState(bool sendMidi = true)
    {
        if (currenttabidx == PTBass || tbrotslow == nullptr || tbrotbrake == nullptr)
            return;

        const bool wantFast = (currenttabidx == PTUpper) ? appState.upperManualRotaryFast : appState.lowerManualRotaryFast;
        const bool wantBrake = (currenttabidx == PTUpper) ? appState.upperManualRotaryBrake : appState.lowerManualRotaryBrake;

        applyRotaryUiSnapshot(wantFast, wantBrake, sendMidi);

        // Align ButtonGroup::rotary with manual Leslie (panel root) for Save + per-preset rotary in .pnl
        const int eU = encodePresetRotaryFromManual(appState.upperManualRotaryFast, appState.upperManualRotaryBrake);
        for (int g = 0; g <= 3; ++g)
            instrumentpanel->getButtonGroup(g)->rotary = eU;
        const int eL = encodePresetRotaryFromManual(appState.lowerManualRotaryFast, appState.lowerManualRotaryBrake);
        for (int g = 4; g <= 7; ++g)
            instrumentpanel->getButtonGroup(g)->rotary = eL;
    }

    /** Phase 1 hotkeys: sync preset radio UI only (no MIDI). */
    void setPresetRadioSelection(int pstidx)
    {
        if (pstidx < 0 || pstidx >= presetbuttons.size()) return;
        if (auto* p = presetbuttons[pstidx])
            p->setToggleState(true, dontSendNotification);
    }

    /** Upper/Lower manual rotary from global hotkeys (works regardless of selected tab). */
    void triggerNextPresetHotkey()
    {
        triggerNextPresetFromCurrentSelection();
    }

    void triggerRotaryFastSlowHotkey()
    {
        if (currenttabidx == PTBass || tbrotslow == nullptr) return;
        if (!getAppState().isrotary) return;
        if (!tbrotslow->isEnabled()) return;
        tbrotslow->triggerClick();
    }

    void triggerRotaryBrakeHotkey()
    {
        if (currenttabidx == PTBass || tbrotbrake == nullptr) return;
        if (!getAppState().isrotary) return;
        if (!tbrotbrake->isEnabled()) return;
        tbrotbrake->triggerClick();
    }

private:
    static String formatButtonGroupTitle(const ButtonGroup& buttonGroup, bool includeSplit)
    {
        String title = buttonGroup.groupname;
        const String alias = buttonGroup.modulealias.trim().toUpperCase();
        if (alias.isNotEmpty())
            title += " " + alias;

        title += " [In:" + juce::String(buttonGroup.midiin)
            + " | Out:" + juce::String(buttonGroup.midiout);

        if (includeSplit)
            title += " | Spl:" + buttonGroup.splitoutname;

        title += "]";
        return title;
    }

    void updatePanelFileStatusLabel()
    {
        if (lblpanelfile == nullptr)
            return;

        lblpanelfile->setText("Panel: " + appState.panelfname, juce::dontSendNotification);
        lblpanelfile->setTooltip(appState.panelfullpathname);
    }

    void triggerNextPresetFromCurrentSelection()
    {
        const int activePresetIdx = juce::jlimit(0, numberpresets - 1, instrumentpanel->panelpresets->getActivePresetIdx());
        int nextPresetIdx = activePresetIdx + 1;

        // Manual (0) jumps to Preset 1. Preset 6 wraps to Preset 1.
        if (nextPresetIdx >= numberpresets || activePresetIdx == 0)
            nextPresetIdx = 1;

        if (nextPresetIdx < 0 || nextPresetIdx >= presetbuttons.size())
            return;

        auto* nextPresetButton = presetbuttons[nextPresetIdx]->getPresetButtonPtr();
        if (nextPresetButton != nullptr)
            nextPresetButton->triggerClick();
    }

    /** Called from VoiceButton::mouseUp only for explicit user clicks on a voice button. */
    void registerExplicitVoiceSelection(int selectedPanelButtonIdx)
    {
        if (selectedPanelButtonIdx < 0 || selectedPanelButtonIdx >= numbervoicebuttons)
            return;

        panelbuttonidx = selectedPanelButtonIdx;
        appState.lastSelectedPanelButtonIdx = selectedPanelButtonIdx;
        appState.hasExplicitVoiceSelection = true;
        enableVoiceEditShortcutButtons();
    }

    /** Sounds / Effects shortcut buttons in "Voice Edits"; enabled after user selects a voice in any group. */
    void enableVoiceEditShortcutButtons()
    {
        if (voiceEditSoundsButton != nullptr)
            voiceEditSoundsButton->setEnabled(true);
        if (voiceEditEffectsButton != nullptr)
            voiceEditEffectsButton->setEnabled(true);
        if (gNotifyVoiceEditTabAccessChanged)
            gNotifyVoiceEditTabAccessChanged();
    }

    juce::TextButton* voiceEditSoundsButton = nullptr;
    juce::TextButton* voiceEditEffectsButton = nullptr;

    void applyRotaryUiSnapshot(bool wantFast, bool wantBrake, bool sendMidi)
    {
        if (currenttabidx == PTBass || tbrotslow == nullptr || tbrotbrake == nullptr)
            return;

        const int buttongroupidx = (currenttabidx == PTLower) ? 4 : 0;
        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
        const int buttongroupmoduleidx = ptrbuttongroup->moduleidx;
        const int buttongroupmidiout = ptrbuttongroup->midiout;
        const int rotorType = instrumentmodules->getRotorType(buttongroupmoduleidx);

        if (wantBrake)
        {
            if (sendMidi)
            {
                RotaryFastSlow(buttongroupmidiout,
                    instrumentmodules->getRotorCC(buttongroupmoduleidx),
                    instrumentmodules->getRotorOff(buttongroupmoduleidx)
                );
            }
            tbrotbrake->setToggleState(true, dontSendNotification);
            tbrotbrake->setButtonText("Brake On");
            rotarybrake = true;
            tbrotslow->setEnabled(false);
            isFastUpper = wantFast;
        }
        else
        {
            tbrotbrake->setToggleState(false, dontSendNotification);
            tbrotbrake->setButtonText("Brake Off");
            rotarybrake = false;
            const bool rotaryUiEnabled = (isrotary == true) && ((currenttabidx == PTUpper) || (currenttabidx == PTLower));
            tbrotslow->setEnabled(rotaryUiEnabled);

            if (wantFast)
            {
                if (sendMidi)
                {
                    if (rotorType == 1) {
                        RotaryFastSlow(buttongroupmidiout,
                            instrumentmodules->getRotorCC(buttongroupmoduleidx),
                            instrumentmodules->getRotorFast(buttongroupmoduleidx)
                        );
                    }
                    else if (rotorType == 2) {
                        rotorsarray.add(new RotaryFastThread(*this, buttongroupmidiout));
                    }
                }
                tbrotslow->setToggleState(false, dontSendNotification);
                tbrotslow->setButtonText("Fast");
                isFastUpper = true;
            }
            else
            {
                if (sendMidi)
                {
                    if (rotorType == 1) {
                        RotaryFastSlow(buttongroupmidiout,
                            instrumentmodules->getRotorCC(buttongroupmoduleidx),
                            instrumentmodules->getRotorSlow(buttongroupmoduleidx)
                        );
                    }
                    else if (rotorType == 2) {
                        rotorsarray.add(new RotarySlowThread(*this, buttongroupmidiout));
                    }
                }
                tbrotslow->setToggleState(true, dontSendNotification);
                tbrotslow->setButtonText("Slow");
                isFastUpper = false;
            }
        }
    }

    OwnedArray<Component> components;

    double startTime;

    MidiInstruments* midiInstruments = nullptr;
    MidiDevices* mididevices = nullptr;
    InstrumentPanel* instrumentpanel = nullptr;
    InstrumentModules* instrumentmodules = nullptr;
    AppState& appState;

    std::unique_ptr<FileChooser> filechooser;

    std::unique_ptr<BubbleMessageComponent> bubbleMessage;

    TooltipWindow tooltipWindow;

    TextButton* tbrotslow;
    TextButton* tbrotbrake;

    Label* lblpanelfile;
    CommandButton* cbSave = nullptr;
    CommandButton* cbSaveAs = nullptr;

    bool isFastUpper = false;
    bool rotarybrake = false;
    OwnedArray<Rotors> rotorsarray;

    int currenttabidx{ 0 };
    int panelbuttonidx{ 0 };

    int gmidichan = 1;

    bool g1uppermute = false;
    bool g2uppermute = false;
    bool g3uppermute = false;
    bool g4uppermute = false;

    Array<PresetButton*> presetbuttons;
    bool bsetpreset = false;

    bool isPanelSavePending() const
    {
        return hasPendingExitSavePrompt();
    }

    void updatePanelSaveButtonsPendingStyle()
    {
        auto applyStyle = [&](CommandButton* button)
            {
                if (button == nullptr)
                    return;

                button->setColour(TextButton::textColourOffId, Colours::white);
                button->setColour(TextButton::textColourOnId, Colours::white);

                if (isPanelSavePending())
                {
                    button->setColour(TextButton::buttonColourId, Colours::darkred);
                    button->setColour(TextButton::buttonOnColourId, Colours::darkred.brighter());
                }
                else
                {
                    button->setColour(TextButton::buttonColourId, Colours::black.darker());
                    button->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
                }
            };

        applyStyle(cbSave);
        applyStyle(cbSaveAs);
    }

    // Volume bar with fixed stacked gradient (bottom stays green/orange).
    class VolumeGradientSlider final : public Slider
    {
    public:
        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            auto inner = bounds.reduced(2.0f);

            g.setColour(juce::Colours::black.withAlpha(0.25f));
            g.fillRoundedRectangle(bounds, 2.5f);

            const auto min = (float)getMinimum();
            const auto max = (float)getMaximum();
            const auto value = (float)getValue();
            const float value01 = (max > min) ? juce::jlimit(0.0f, 1.0f, (value - min) / (max - min)) : 0.0f;

            const float filledHeight = inner.getHeight() * value01;
            auto fillArea = juce::Rectangle<float>(
                inner.getX(),
                inner.getBottom() - filledHeight,
                inner.getWidth(),
                filledHeight);

            juce::ColourGradient gradient(
                juce::Colours::green.darker(), inner.getCentreX(), inner.getBottom(),
                juce::Colours::red.darker(), inner.getCentreX(), inner.getY(),
                false);
            gradient.addColour(0.65, juce::Colours::orange.darker());

            g.setGradientFill(gradient);
            g.fillRect(fillArea);

            g.setColour(juce::Colours::black.withAlpha(0.55f));
            g.drawRect(inner, 1.0f);

            if (!isEnabled())
            {
                g.setColour(juce::Colours::black.withAlpha(0.35f));
                g.fillRect(inner);
            }
        }
    };

    Slider* createSlider(bool isSnapping)
    {
        auto* s = isSnapping ? new SnappingSlider()
            : new Slider();

        s->setRange(0.0, 100.0, 1.0);
        s->setColour(juce::Slider::ColourIds::trackColourId, juce::Colours::green.darker());
        s->setPopupMenuEnabled(true);
        s->setValue(Random::getSystemRandom().nextDouble() * 100.0, dontSendNotification);
        return s;
    }

    Slider* createVolSlider(bool isSnapping)
    {
        auto* s = isSnapping ? new SnappingSlider()
            : static_cast<Slider*>(new VolumeGradientSlider());

        s->setRange(0.0, 10.0, 1.0);
        s->setPopupMenuEnabled(true);
        s->setValue(8.0, dontSendNotification);
        updateVolumeSliderDebugText(s, sliderStepToMasterCc7(8));
        return s;
    }

    int computeEffectiveCc7ForButton(ButtonGroup* buttonGroup, Slider* groupSlider, int panelbtnidx) const
    {
        if (buttonGroup == nullptr || groupSlider == nullptr)
            return 0;

        auto* voiceButton = instrumentpanel->getVoiceButton(panelbtnidx);
        if (voiceButton == nullptr)
            return 0;

        const int masterCc7 = sliderStepToMasterCc7((int)groupSlider->getValue());
        const int effectVol = voiceButton->getInstrument().getVol();
        const int effectiveCc7 = computeEffectiveVolumeCc7(masterCc7, effectVol, appState.defaultEffectsVol);
        return buttonGroup->getMuteButtonStatus() ? 0 : effectiveCc7;
    }

    void sendCombinedGroupVolume(ButtonGroup* buttonGroup, int midiOut, int effectiveCc7)
    {
        if (buttonGroup == nullptr)
            return;

        const int clampedCc7 = juce::jlimit(0, 127, effectiveCc7);
        if (!buttonGroup->isEffectDirty(0, clampedCc7))
            return;

        auto ccMessage = juce::MidiMessage::controllerEvent(midiOut, CCVol, clampedCc7);
        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
        mididevices->sendToOutputs(ccMessage);
    }

    void updateVolumeSliderDebugText(Slider* slider, int effectiveCc7) const
    {
        if (slider == nullptr)
            return;

        slider->setTextValueSuffix(" / CC7 " + juce::String(juce::jlimit(0, 127, effectiveCc7)));
    }

    void updateVolumeStatusLine(Label* label, const String& groupName, double sliderVal, int effectiveCc7) const
    {
        if (label == nullptr)
            return;
        label->setText(groupName + " " + String((int)sliderVal) + "→" + String(effectiveCc7), dontSendNotification);
    }

    static juce::String getMidiMessageDescription(const juce::MidiMessage& m)
    {
        if (m.isNoteOn())           return "Note on " + juce::MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3);
        if (m.isNoteOff())          return "Note off " + juce::MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3);
        if (m.isProgramChange())    return "Program change " + juce::String(m.getProgramChangeNumber());
        if (m.isPitchWheel())       return "Pitch wheel " + juce::String(m.getPitchWheelValue());
        if (m.isAftertouch())       return "After touch " + juce::MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3) + ": " + juce::String(m.getAfterTouchValue());
        if (m.isChannelPressure())  return "Channel pressure " + juce::String(m.getChannelPressureValue());
        if (m.isAllNotesOff())      return "All notes off";
        if (m.isAllSoundOff())      return "All sound off";
        if (m.isMetaEvent())        return "Meta event";

        if (m.isController())
        {
            juce::String name(juce::MidiMessage::getControllerName(m.getControllerNumber()));

            if (name.isEmpty())
                name = "[" + juce::String(m.getControllerNumber()) + "]";

            return "Controller " + name + ": " + juce::String(m.getControllerValue());
        }

        return juce::String::toHexString(m.getRawData(), m.getRawDataSize());
    }

    //-----------------------------------------------------------------------------
    // Simple on Rotary Fast/Slow and On/Off toggles with Midi Devices doing the ramp up/down
    void RotaryFastSlow(int midiout, int controllerNumber, int controllerValue) {
        // Preset rotary change control value to slow
        juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
            midiout,
            controllerNumber,
            controllerValue
        );
        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
        mididevices->sendToOutputs(ccMessage);
    }

    void RotaryOnOff(int midiout, int controllerNumber, int controllerValue) {
        // Preset rotary change control value to fast
        juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
            midiout,
            controllerNumber,
            controllerValue
        );
        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
        mididevices->sendToOutputs(ccMessage);
    }

    void commitManualRotaryToAppState()
    {
        if (currenttabidx == PTBass || tbrotslow == nullptr)
            return;

        if (currenttabidx == PTUpper)
        {
            appState.upperManualRotaryFast = isFastUpper;
            appState.upperManualRotaryBrake = rotarybrake;
        }
        else if (currenttabidx == PTLower)
        {
            appState.lowerManualRotaryFast = isFastUpper;
            appState.lowerManualRotaryBrake = rotarybrake;
        }
    }


    //-----------------------------------------------------------------------------
    // Function adds a component to a list and calls addAndMakeVisible on it
    template <typename ComponentType>
    ComponentType* addToList(ComponentType* newComp)
    {
        components.add(newComp);
        addAndMakeVisible(newComp);
        return newComp;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyboardPanelPage)
};


//==============================================================================
// Class: MidiStartPage
//==============================================================================
static int zinstcntMidiStartPage = 0;
class MidiStartPage final : public Component,
    private MidiKeyboardState::Listener,
    private MidiInputCallback,
    private AsyncUpdater,
    public ComponentListener
{
public:
    MidiStartPage(TabbedComponent& tabs,
                  std::function<bool(const juce::String&)> loadConfigFromBasenameFn,
                  std::function<void()> refreshKeyboardPanelSaveAvailabilityFn = {},
                  std::shared_ptr<std::atomic<bool>> virtualKeyboardInputEnabledState = {}) :
        midiKeyboard(keyboardState, MidiKeyboardComponent::horizontalKeyboard),
        midiInputSelector(new MidiDeviceListBox("Midi Input Selector", *this, true)),
        midiOutputSelector(new MidiDeviceListBox("Midi Output Selector", *this, false)),
        instrumentmodules(InstrumentModules::getInstance()),
        midiInstruments(MidiInstruments::getInstance()),
        mididevices(MidiDevices::getInstance()),
        instrumentpanel(InstrumentPanel::getInstance()),
        appState(getAppState()),
        loadConfigFromBasename(std::move(loadConfigFromBasenameFn)),
        refreshKeyboardPanelSaveAvailability(std::move(refreshKeyboardPanelSaveAvailabilityFn)),
        virtualKeyboardInputEnabled(std::move(virtualKeyboardInputEnabledState))
    {
        juce::Logger::writeToLog("== MidiStartPage(): Constructor " + std::to_string(zinstcntMidiStartPage++));

        addLabelAndSetStyle(midiInputLabel);
        addLabelAndSetStyle(midiOutputLabel);
        addLabelAndSetStyle(incomingMidiLabel);

        if (!BluetoothMidiDevicePairingDialogue::isAvailable())
            pairButton.setEnabled(false);

        addAndMakeVisible(pairButton);
        pairButton.onClick = []
            {
                RuntimePermissions::request(RuntimePermissions::bluetoothMidi,
                    [](bool wasGranted)
                    {
                        if (wasGranted)
                            BluetoothMidiDevicePairingDialogue::open();
                    });
            };
        keyboardState.addListener(this);

        addAndMakeVisible(midiInputSelector.get());
        addAndMakeVisible(midiOutputSelector.get());

        addAndMakeVisible(loadConfigButton);
        loadConfigButton.setButtonText("Load Config");
        loadConfigButton.setColour(TextButton::textColourOffId, Colours::white);
        loadConfigButton.setColour(TextButton::textColourOnId, Colours::white);
        loadConfigButton.setColour(TextButton::buttonColourId, Colours::black);
        loadConfigButton.setColour(TextButton::buttonOnColourId, Colours::black);
        loadConfigButton.setToggleState(true, dontSendNotification);
        loadConfigButton.onClick = [this]()
            {
                // 6-arg ctor: explicit useOSNativeDialogBox=false => JUCE FileBrowser (not OS shell dialog).
                filechooser.reset(new FileChooser(
                    "Select Config file to load",
                    File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                        .getChildFile(organdir)
                        .getChildFile(appState.configdir),
                    "*.cfg",
                    false,
                    false,
                    nullptr));

                filechooser->launchAsync(
                    FileBrowserComponent::openMode
                    | FileBrowserComponent::canSelectFiles
                    | FileBrowserComponent::filenameBoxIsReadOnly,
                    [this](const FileChooser& chooser)
                    {
                        String selectedfname;
                        String selectedfullpathfname;
                        auto results = chooser.getURLResults();

                        for (auto result : results) {
                            if (result.isLocalFile()) {
                                selectedfname = result.getLocalFile().getFileName();
                                selectedfullpathfname = result.getLocalFile().getFullPathName();
                            }
                            else
                                result.toString(false);
                        }

                        if (selectedfname == "") return;

                        juce::Logger::writeToLog("*** MidiStartPage(): Loading Config: " + selectedfname);

                        auto refreshConfigLabel = [this]()
                            {
                                appState.configreload = (appState.configfname.compare(appState.pnlconfigfname) != 0);
                                configfileLabel.setText(appState.configfname, {});
                                if (appState.configreload == true)
                                    configfileLabel.setColour(juce::Label::textColourId, juce::Colours::red.darker());
                                else
                                    configfileLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
                            };

                        auto performLoad = [this, selectedfname, refreshConfigLabel]()
                            {
                                if (!loadConfigFromBasename)
                                {
                                    juce::Logger::writeToLog("*** MidiStartPage(): Load Config: loader not available");
                                    return;
                                }
                                const bool bloaded = loadConfigFromBasename(selectedfname);
                                refreshConfigLabel();
                                const String msgloaded = bloaded ? ("Loaded: " + appState.configfname)
                                    : ("Load failed: " + selectedfname);
                                if (TextButton* focused = &loadConfigButton)
                                    BubbleMessage(*focused, msgloaded, this->bubbleMessage);
                                if (refreshKeyboardPanelSaveAvailability)
                                    refreshKeyboardPanelSaveAvailability();
                            };

                        if (selectedfname == appState.pnlconfigfname)
                        {
                            appState.configPanelPairingMismatchAcknowledged = false;
                            performLoad();
                            return;
                        }

                        const String msg = juce::String("The current instrument panel was saved with \"") + appState.pnlconfigfname
                            + "\".\nYou selected \"" + selectedfname + "\".\n\n"
                            "Routing may not match this song/style until configs align.\n\n"
                            "Load anyway?";

                        auto safeStart = juce::Component::SafePointer<MidiStartPage>(this);
                        juce::AlertWindow::showOkCancelBox(
                            juce::AlertWindow::WarningIcon,
                            "Config / panel mismatch",
                            msg,
                            "Load anyway",
                            "Abort",
                            this,
                            juce::ModalCallbackFunction::create([safeStart, selectedfname](int result)
                                {
                                    if (safeStart == nullptr || result != 1)
                                        return;
                                    safeStart->appState.configPanelPairingMismatchAcknowledged = true;
                                    if (!safeStart->loadConfigFromBasename)
                                        return;
                                    const bool bloaded = safeStart->loadConfigFromBasename(selectedfname);
                                    safeStart->appState.configreload = (safeStart->appState.configfname.compare(safeStart->appState.pnlconfigfname) != 0);
                                    safeStart->configfileLabel.setText(safeStart->appState.configfname, {});
                                    if (safeStart->appState.configreload == true)
                                        safeStart->configfileLabel.setColour(juce::Label::textColourId, juce::Colours::red.darker());
                                    else
                                        safeStart->configfileLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
                                    const juce::String msgloaded = bloaded ? ("Loaded: " + safeStart->appState.configfname)
                                        : ("Load failed: " + selectedfname);
                                    if (juce::TextButton* focused = &safeStart->loadConfigButton)
                                        BubbleMessage(*focused, msgloaded, safeStart->bubbleMessage);
                                    if (safeStart->refreshKeyboardPanelSaveAvailability)
                                        safeStart->refreshKeyboardPanelSaveAvailability();
                                }));
                    });
            };

        addAndMakeVisible(loadPanelButton);
        loadPanelButton.setButtonText("Load Panel");
        loadPanelButton.setColour(TextButton::textColourOffId, Colours::white);
        loadPanelButton.setColour(TextButton::textColourOnId, Colours::white);
        loadPanelButton.setColour(TextButton::buttonColourId, Colours::black);
        loadPanelButton.setColour(TextButton::buttonOnColourId, Colours::black);
        loadPanelButton.setToggleState(true, dontSendNotification);
        loadPanelButton.onClick = [this]()
            {
                // Match Load Config: JUCE FileChooserDialogBox + FileBrowserComponent (useOSNativeDialogBox=false).
                filechooser.reset(new FileChooser(
                    "Select Button Panel file to load",
                    getDefaultPanelsDirectory(),
                    "*.pnl",
                    false,
                    false,
                    nullptr));

                filechooser->launchAsync(
                    FileBrowserComponent::openMode
                    | FileBrowserComponent::canSelectFiles
                    | FileBrowserComponent::filenameBoxIsReadOnly,
                    [this](const FileChooser& chooser)
                    {
                        String selectedfname;
                        String selectedfullpathfname;
                        auto results = chooser.getURLResults();

                        for (auto result : results)
                        {
                            if (result.isLocalFile())
                            {
                                selectedfname = result.getLocalFile().getFileName();
                                selectedfullpathfname = result.getLocalFile().getFullPathName();
                            }
                            else
                                result.toString(false);
                        }

                        if (selectedfname.isEmpty())
                            return;

                        proceedWithPickedPanelFile(juce::File(selectedfullpathfname));
                    });
            };

        addAndMakeVisible(exitButton);
        exitButton.setButtonText("Exit");
        exitButton.setColour(TextButton::textColourOffId, Colours::white);
        exitButton.setColour(TextButton::textColourOnId, Colours::white);
        exitButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        exitButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        exitButton.onClick = [=]() {

            ApplicationQuit();
            };
        // Header-level Exit button is provided globally in AMidiControl.
        exitButton.setVisible(false);

        int mgroup = 10;
        addAndMakeVisible(toModule);
        toModule.setButtonText(appState.vendorname);
        toModule.setClickingTogglesState(false);
        toModule.setColour(TextButton::textColourOffId, Colours::white);
        toModule.setColour(TextButton::textColourOnId, Colours::white);
        toModule.setColour(TextButton::buttonColourId, Colours::black);
        toModule.setColour(TextButton::buttonOnColourId, Colours::black);
        toModule.setBounds(mgroup, mgroup + 205, 160, 50);
        toModule.setToggleState(true, dontSendNotification);
        toModule.onClick = [=]()
            {
                try
                {
                    PopupMenu menu;
                    int icount = instrumentmodules->getNumModules();
                    // Do not use 0 as the index count as 0 is used to indicate no selection
                    for (int i = 1; i <= icount; ++i)
                    {
                        menu.addItem(i, instrumentmodules->getDisplayName(i - 1), true, true);
                    }
                    menu.showMenuAsync(PopupMenu::Options{}.withTargetComponent(toModule),
                        [=](int result) {
                            // If incomplete selection, abort
                            if (result == 0) return;

                            // Default Midi Instrument Module to Deebach until selected otherwise
                            appState.moduleidx = result - 1;
                            instrumentmodules->setInstrumentModule(appState.moduleidx);

                            // Save Device Modules includig index so we start with the same
                            instrumentmodules->saveModules();

                            // Instantiate Midi Instruments and load sound file into JSON object
                            // and Update Vendor Button text with new name
                            midiInstruments->loadMidiInstruments(appState.instrumentfname);
                            String currentVendorName = midiInstruments->getVendor();
                            toModule.setButtonText(currentVendorName);

                            // Instantiate Instrument Panel from Master Panel on Disk when true
                            bool bloaded = instrumentpanel->loadInstrumentPanel(appState.instrumentdname, defpanelfname, false);

                            String msgloaded;
                            if (bloaded == true) {
                                msgloaded = "Loading module:\n " + currentVendorName;
                            }
                            else {
                                msgloaded = "Module load failed!\n " + currentVendorName;
                            }

                            if (TextButton* focused = &toModule)
                                BubbleMessage(*focused, msgloaded, this->bubbleMessage);

                            DBG("=== MidiStartPage(): Module loaded: " + std::to_string(result));
                        });
                }
                catch (...) {
                    DBG("=== MidiStartPage(): Aborted Modules select");
                }
            };

        addAndMakeVisible(toConfig);
        toConfig.setButtonText("Config");
        toConfig.setClickingTogglesState(false);
        toConfig.setColour(TextButton::textColourOffId, Colours::black);
        toConfig.setColour(TextButton::textColourOnId, Colours::white);
        toConfig.setColour(TextButton::buttonColourId, Colours::white);
        toConfig.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        toConfig.setBounds(mgroup + 810, mgroup + 200, 90, 50);
        toConfig.setToggleState(true, dontSendNotification);
        toConfig.onClick = [=, &tabs]()
            {
                tabs.setCurrentTabIndex(PTConfig, false);
            };

        addAndMakeVisible(panelPrefixLabel);
        panelPrefixLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        panelPrefixLabel.setJustificationType(juce::Justification::left);

        addAndMakeVisible(panelfileLabel);
        panelfileLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        panelfileLabel.setJustificationType(juce::Justification::left);

        addAndMakeVisible(configPrefixLabel);
        configPrefixLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        configPrefixLabel.setJustificationType(juce::Justification::left);

        addAndMakeVisible(configfileLabel);
        configfileLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        configfileLabel.setJustificationType(juce::Justification::left);

        // Quick Access Keyboard Buttons
        {
            int quickAccessMargin = 10;
            int xaccess = 720;

            xaccess = xaccess + 200;
            addAndMakeVisible(toUpperKBD);
            toUpperKBD.setButtonText("Upper");
            toUpperKBD.setClickingTogglesState(false);
            toUpperKBD.setColour(TextButton::textColourOffId, Colours::black);
            toUpperKBD.setColour(TextButton::textColourOnId, Colours::white);
            toUpperKBD.setColour(TextButton::buttonColourId, Colours::white);
            toUpperKBD.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            toUpperKBD.setBounds(quickAccessMargin + xaccess, quickAccessMargin + 200, 80, 50);
            toUpperKBD.setToggleState(true, dontSendNotification);
            toUpperKBD.onClick = [=, &tabs]()
                {
                    tabs.setCurrentTabIndex(PTUpper, false);
                };

            xaccess = xaccess + 100;
            addAndMakeVisible(toLowerKBD);
            toLowerKBD.setButtonText("Lower");
            toLowerKBD.setClickingTogglesState(false);
            toLowerKBD.setColour(TextButton::textColourOffId, Colours::black);
            toLowerKBD.setColour(TextButton::textColourOnId, Colours::white);
            toLowerKBD.setColour(TextButton::buttonColourId, Colours::white);
            toLowerKBD.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            toLowerKBD.setBounds(quickAccessMargin + xaccess, quickAccessMargin + 200, 80, 50);
            toLowerKBD.setToggleState(true, dontSendNotification);
            toLowerKBD.onClick = [=, &tabs]()
                {
                    tabs.setCurrentTabIndex(PTLower, false);
                };

            xaccess = xaccess + 100;
            addAndMakeVisible(toBassKBD);
            toBassKBD.setButtonText("Bass");
            toBassKBD.setClickingTogglesState(false);
            toBassKBD.setColour(TextButton::textColourOffId, Colours::black);
            toBassKBD.setColour(TextButton::textColourOnId, Colours::white);
            toBassKBD.setColour(TextButton::buttonColourId, Colours::white);
            toBassKBD.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            toBassKBD.setBounds(quickAccessMargin + xaccess, quickAccessMargin + 200, 80, 50);
            toBassKBD.setToggleState(true, dontSendNotification);
            toBassKBD.onClick = [=, &tabs]()
                {
                    tabs.setCurrentTabIndex(PTBass, false);
                };
        }

        // MidiStartPage Status Bar
        addAndMakeVisible(statusLabel);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        statusLabel.setJustificationType(juce::Justification::right);

        addAndMakeVisible(buildInfoLabel);
        buildInfoLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        buildInfoLabel.setJustificationType(juce::Justification::right);
        buildInfoLabel.setText("Build " + juce::String(AMIDIORGAN_PROJECT_VERSION)
            + " (" + juce::String(AMIDIORGAN_BUILD_NUMBER) + ")", juce::dontSendNotification);

        // Preset Device Modules includig index so we start with the same one as last saved
        instrumentmodules->loadModules();
        instrumentmodules->setInstrumentModule(appState.moduleidx);
        ////vendorname = instrumentmodules->getDisplayName(moduleidx);

        // Instantiate Midi Instruments and load sound file into JSON object
        // and Update Vendor Button text with new name
        midiInstruments->loadMidiInstruments(appState.instrumentfname);
        String currentVendorName = midiInstruments->getVendor();
        toModule.setButtonText(currentVendorName);

        // Instantiate Instrument Panel from Master Panel on Disk when true
        bool bloaded = instrumentpanel->initInstrumentPanel(appState.instrumentdname, appState.panelfname, true);

        String msgloaded;
        if (bloaded == true) {
            //statusLabel.setText("Loaded: " + panelfname, {});
            msgloaded = "Loaded: " + appState.panelfname;
        }
        else {
            //statusLabel.setText("Load failed: " + panelfname, {});
            msgloaded = "Load failed: " + appState.panelfname;
        }

        if (Label* focused = &statusLabel)
            BubbleMessage(*focused, msgloaded, this->bubbleMessage);

        loadStickyMidiPortsFromDisk();

        // Update with active devices and Channel Output based on Button Groups
        updateDeviceLists();

        // Register for device-list changes after page initialization is complete.
        auto safeThis = juce::Component::SafePointer<MidiStartPage>(this);
        connection = MidiDeviceListConnection::make([safeThis]()
            {
                if (safeThis != nullptr)
                    safeThis->updateDeviceLists();
            });

        // Dsplay current Panel File and Config Files in use. 
        // Flag mismatch between panel file and incorrect config file in red
        configfileLabel.setText(appState.configfname, {});
        panelfileLabel.setText(appState.panelfname, {});

        appState.configreload = (appState.configfname.compare(appState.pnlconfigfname) != 0);

        if  (appState.configreload == true)
            configfileLabel.setColour(juce::Label::textColourId, juce::Colours::red.darker());
        else
            configfileLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    }

    //-----------------------------------------------------------------------------
    ~MidiStartPage() override
    {
        DBG("=== MidiStartPage(): Destructor " + std::to_string(--zinstcntMidiStartPage));

        saveStickyMidiPortsToDisk();

        // Disconnect device-change callback before tearing down controls/state.
        connection.reset();

        keyboardState.removeListener(this);

        midiInputSelector.reset();
        midiOutputSelector.reset();
    }

    //-----------------------------------------------------------------------------
    void paint(Graphics&) override {}

    void resized() override
    {
        auto margin = 10;

        midiInputLabel.setBounds(margin, margin,
            (getWidth() / 2) - (2 * margin), 20);

        midiOutputLabel.setBounds((getWidth() / 2) + margin, margin,
            (getWidth() / 2) - (2 * margin), 20);

        midiInputSelector->setBounds(margin, 2 * margin + 20,
            (getWidth() / 2) - (2 * margin), 100);

        midiOutputSelector->setBounds((getWidth() / 2) + margin, 2 * margin + 20,
            (getWidth() / 2) - (2 * margin), 100);

        pairButton.setBounds(margin, 2 * margin + 140,
            getWidth() - (2 * margin), 36);

        loadConfigButton.setBounds(180, margin + 205, 90, 50);

        // Keep Load Panel aligned with Instruments/Config actions.
        loadPanelButton.setBounds(280, margin + 205, 90, 50);
        toConfig.setBounds(820, margin + 200, 90, 50);

        // Place Exit in the top-right header area for faster access.
        exitButton.setBounds(getWidth() - 90, 4, 80, 24);

        panelPrefixLabel.setBounds(480, margin + 210, 70, 20);
        panelfileLabel.setBounds(550, margin + 210, 280, 20);

        configPrefixLabel.setBounds(480, margin + 230, 70, 20);
        configfileLabel.setBounds(550, margin + 230, 280, 20);

        statusLabel.setBounds(1180, margin + 200, 250, 20);
        buildInfoLabel.setBounds(getWidth() - 290, getHeight() - 24, 280, 20);
    }

    //-----------------------------------------------------------------------------
    // Auto refresh functionalities as we tab back in
    void broughtToFront() {

        DBG("*** MiDiStartPage(): Brought To Front Tab " + std::to_string(0));

        // Dsplay current Panel File and Config Files in use. 
        // Flag mismatch between panel file and incorrect config file in red
        configfileLabel.setText(appState.configfname, {});
        panelfileLabel.setText(appState.panelfname, {});

        appState.configreload = (appState.configfname.compare(appState.pnlconfigfname) != 0);

        if (appState.configreload == true)
            configfileLabel.setColour(juce::Label::textColourId, juce::Colours::red.darker());
        else
            configfileLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    }

private:
    static constexpr const char* kVirtualKeyboardInputId = "virtual-keyboard-internal";
    static constexpr const char* kVirtualKeyboardInputName = "Virtual Keyboard (Internal)";

    juce::StringArray stickyMidiInputIds;
    juce::StringArray stickyMidiOutputIds;

    bool hasVirtualKeyboardInput() const
    {
        return (virtualKeyboardInputEnabled != nullptr);
    }

    int getVirtualKeyboardRowIndex() const
    {
        return mididevices->getNumMidiInputs();
    }

    bool isVirtualKeyboardRow(int rowNumber) const
    {
        return hasVirtualKeyboardInput() && rowNumber == getVirtualKeyboardRowIndex();
    }

    bool isVirtualKeyboardInputEnabled() const
    {
        return hasVirtualKeyboardInput() && virtualKeyboardInputEnabled->load(std::memory_order_relaxed);
    }

    void setVirtualKeyboardInputEnabled(bool enabled)
    {
        if (!hasVirtualKeyboardInput())
            return;
        virtualKeyboardInputEnabled->store(enabled, std::memory_order_relaxed);
    }

    void loadStickyMidiPortsFromDisk()
    {
        loadMidiStickyDeviceIdentifiersFromFile(stickyMidiInputIds, stickyMidiOutputIds);
    }

    void saveStickyMidiPortsToDisk()
    {
        juce::StringArray inIds, outIds;
        for (int i = 0; i < mididevices->midiInputs.size(); ++i)
            if (mididevices->midiInputs[i]->inDevice != nullptr)
                inIds.add(mididevices->midiInputs[i]->deviceInfo.identifier);
        if (isVirtualKeyboardInputEnabled())
            inIds.add(kVirtualKeyboardInputId);
        for (int i = 0; i < mididevices->midiOutputs.size(); ++i)
            if (mididevices->midiOutputs[i]->outDevice != nullptr)
                outIds.add(mididevices->midiOutputs[i]->deviceInfo.identifier);

        if (!saveMidiStickyDeviceIdentifiersToFile(inIds, outIds))
            juce::Logger::writeToLog("*** MidiStartPage: could not save midi_sticky_devices.json");
    }

    void applyStickyMidiPortsForList(bool isInputDeviceList)
    {
        const juce::StringArray& sticky = isInputDeviceList ? stickyMidiInputIds : stickyMidiOutputIds;
        if (sticky.isEmpty())
            return;

        auto& devices = isInputDeviceList ? mididevices->midiInputs : mididevices->midiOutputs;

        for (const auto& stickyId : sticky)
        {
            if (stickyId.isEmpty())
                continue;

            if (isInputDeviceList && stickyId == kVirtualKeyboardInputId)
            {
                setVirtualKeyboardInputEnabled(true);
                continue;
            }

            for (int i = 0; i < devices.size(); ++i)
            {
                if (devices[i]->deviceInfo.identifier != stickyId)
                    continue;

                const bool alreadyOpen = isInputDeviceList
                    ? (devices[i]->inDevice != nullptr)
                    : (devices[i]->outDevice != nullptr);

                if (!alreadyOpen)
                    mididevices->openDevice(isInputDeviceList, i);

                break;
            }
        }
    }

    void proceedWithPickedPanelFile(const juce::File& pnlFile)
    {
        const String selectedfname = pnlFile.getFileName();
        const String selectedfullpathfname = pnlFile.getFullPathName();
        const juce::String embeddedCfg = readEmbeddedConfigFilenameFromPanelFile(pnlFile);

        auto finishPanelLoadUi = [this, selectedfname, selectedfullpathfname](bool bloaded)
            {
                appState.panelfname = selectedfname;
                appState.panelfullpathname = selectedfullpathfname;

                juce::String msgloaded;
                if (bloaded == true)
                    msgloaded = "Loaded panel:\n " + appState.panelfname;
                else
                    msgloaded = "Panel load failed: " + appState.panelfname;

                if (TextButton* focused = &loadPanelButton)
                    BubbleMessage(*focused, msgloaded, this->bubbleMessage);

                panelfileLabel.setText(appState.panelfname, {});

                clearConfigPanelPairingMismatchIfAligned(appState);

                appState.configreload = (appState.configfname.compare(appState.pnlconfigfname) != 0);

                if (appState.configreload == true)
                    configfileLabel.setColour(juce::Label::textColourId, juce::Colours::red.darker());
                else
                    configfileLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

                if (refreshKeyboardPanelSaveAvailability)
                    refreshKeyboardPanelSaveAvailability();
            };

        auto runPanelLoad = [this, selectedfname, selectedfullpathfname, finishPanelLoadUi]()
            {
                juce::Logger::writeToLog("*** MidiStartPage(): Loading Instrument Panel: " + selectedfname);
                const bool bloaded = instrumentpanel->loadInstrumentPanel(appState.instrumentdname, selectedfullpathfname, true);
                finishPanelLoadUi(bloaded);
            };

        if (embeddedCfg.isEmpty() || embeddedCfg == appState.configfname)
        {
            if (embeddedCfg == appState.configfname)
                appState.configPanelPairingMismatchAcknowledged = false;
            runPanelLoad();
            return;
        }

        const juce::String msg = juce::String("This panel file was saved with config \"") + embeddedCfg
            + "\".\nThe active config is \"" + appState.configfname + "\".\n\n"
            "Routing may not match this song/style until configs align.\n\n"
            "Load anyway?";

        auto safeStart = juce::Component::SafePointer<MidiStartPage>(this);
        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            "Config / panel mismatch",
            msg,
            "Load anyway",
            "Abort",
            this,
            juce::ModalCallbackFunction::create([safeStart, selectedfname, selectedfullpathfname](int result)
                {
                    if (safeStart == nullptr || result != 1)
                        return;
                    safeStart->appState.configPanelPairingMismatchAcknowledged = true;
                    juce::Logger::writeToLog("*** MidiStartPage(): Loading Instrument Panel: " + selectedfname);
                    const bool bloaded = safeStart->instrumentpanel->loadInstrumentPanel(
                        safeStart->appState.instrumentdname, selectedfullpathfname, true);
                    safeStart->appState.panelfname = selectedfname;
                    safeStart->appState.panelfullpathname = selectedfullpathfname;
                    juce::String msgloaded;
                    if (bloaded == true)
                        msgloaded = "Loaded panel:\n " + safeStart->appState.panelfname;
                    else
                        msgloaded = "Panel load failed: " + safeStart->appState.panelfname;
                    if (juce::TextButton* focused = &safeStart->loadPanelButton)
                        BubbleMessage(*focused, msgloaded, safeStart->bubbleMessage);
                    safeStart->panelfileLabel.setText(safeStart->appState.panelfname, {});
                    clearConfigPanelPairingMismatchIfAligned(safeStart->appState);
                    safeStart->appState.configreload = (safeStart->appState.configfname.compare(safeStart->appState.pnlconfigfname) != 0);
                    if (safeStart->appState.configreload == true)
                        safeStart->configfileLabel.setColour(juce::Label::textColourId, juce::Colours::red.darker());
                    else
                        safeStart->configfileLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
                    if (safeStart->refreshKeyboardPanelSaveAvailability)
                        safeStart->refreshKeyboardPanelSaveAvailability();
                }));
    }

    //-----------------------------------------------------------------------------
    //*** Start of MidiDeviceListBox Class
    struct MidiDeviceListBox final : private ListBoxModel,
        public ListBox
    {
        MidiDeviceListBox(const String& name,
            MidiStartPage& contentComponent,
            bool isInputDeviceList)
            : ListBox(name),
            parent(contentComponent),
            isInput(isInputDeviceList),
            lblmididevices(MidiDevices::getInstance()) // Creates the singleton if there isn't already one
        {
            setModel(this);
            setOutlineThickness(1);
            setMultipleSelectionEnabled(true);
            setClickingTogglesRowSelection(true);
        }

        int getNumRows() override
        {
            int rows = lblmididevices->getNumDevices(isInput);
            if (isInput && parent.hasVirtualKeyboardInput())
                rows += 1;
            return rows;
        }


        void paintListBoxItem(int rowNumber, Graphics& g,
            int width, int height, bool rowIsSelected) override
        {

            auto textColour = getLookAndFeel().findColour(ListBox::textColourId);

            if (rowIsSelected)
                g.fillAll(textColour.interpolatedWith(getLookAndFeel().findColour(ListBox::backgroundColourId), 0.5));

            g.setColour(textColour);
            g.setFont((float)height * 0.7f);
            if (isInput)
            {
                if (parent.isVirtualKeyboardRow(rowNumber))
                    g.drawText(MidiStartPage::kVirtualKeyboardInputName,
                        5, 0, width, height,
                        Justification::centredLeft, true);
                else if (rowNumber < lblmididevices->getNumMidiInputs())
                    g.drawText(lblmididevices->getMidiDevice(rowNumber, true)->deviceInfo.name,
                        5, 0, width, height,
                        Justification::centredLeft, true);
            }
            else
            {
                if (rowNumber < lblmididevices->getNumMidiOutputs())
                    g.drawText(lblmididevices->getMidiDevice(rowNumber, false)->deviceInfo.name,
                        5, 0, width, height,
                        Justification::centredLeft, true);
            }
        }


        //-----------------------------------------------------------------------------
        void selectedRowsChanged(int) override
        {
            auto newSelectedItems = getSelectedRows();
            if (newSelectedItems != lastSelectedItems)
            {
                for (auto i = 0; i < lastSelectedItems.size(); ++i)
                {
                    if (!newSelectedItems.contains(lastSelectedItems[i]))
                    {
                        if (isInput && parent.isVirtualKeyboardRow(lastSelectedItems[i]))
                            parent.setVirtualKeyboardInputEnabled(false);
                        else
                            lblmididevices->closeDevice(isInput, lastSelectedItems[i]);
                    }
                }

                for (auto i = 0; i < newSelectedItems.size(); ++i)
                {
                    if (!lastSelectedItems.contains(newSelectedItems[i]))
                    {
                        if (isInput && parent.isVirtualKeyboardRow(newSelectedItems[i]))
                            parent.setVirtualKeyboardInputEnabled(true);
                        else
                            lblmididevices->openDevice(isInput, newSelectedItems[i]);
                    }
                }

                lastSelectedItems = newSelectedItems;
                parent.saveStickyMidiPortsToDisk();
            }
        }

        //-----------------------------------------------------------------------------
        void syncSelectedItemsWithDeviceList(const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices)
        {
            SparseSet<int> selectedRows;
            for (auto i = 0; i < midiDevices.size(); ++i)
                if (midiDevices[i]->inDevice != nullptr || midiDevices[i]->outDevice != nullptr)
                    selectedRows.addRange(Range<int>(i, i + 1));

            if (isInput && parent.isVirtualKeyboardInputEnabled())
            {
                const int virtualRow = parent.getVirtualKeyboardRowIndex();
                selectedRows.addRange(Range<int>(virtualRow, virtualRow + 1));
            }

            lastSelectedItems = selectedRows;
            updateContent();
            setSelectedRows(selectedRows, dontSendNotification);

            // Reset all Midi Controllers
            lblmididevices->resetAllControllers();
        }

    private:
        MidiStartPage& parent;
        bool isInput;
        SparseSet<int> lastSelectedItems;

        MidiDevices* lblmididevices = nullptr;

    };
    //*** End of MidiDeviceListBox Class ***

    //-----------------------------------------------------------------------------
    void addLabelAndSetStyle(Label& label)
    {
        label.setFont(Font(FontOptions(15.00f, Font::plain)));
        label.setJustificationType(Justification::centredLeft);
        label.setEditable(false, false, false);
        label.setColour(TextEditor::textColourId, Colours::black);
        label.setColour(TextEditor::backgroundColourId, Colour(0x00000000));

        addAndMakeVisible(label);
    }

    //-----------------------------------------------------------------------------
    void handleNoteOn(MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {

        MidiMessage m(MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity));
        m.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
        mididevices->sendToOutputs(m);
    }

    void handleNoteOff(MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {

        MidiMessage m(MidiMessage::noteOff(midiChannel, midiNoteNumber, velocity));
        m.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
        mididevices->sendToOutputs(m);
    }

    //-----------------------------------------------------------------------------
    void handleIncomingMidiMessage(MidiInput*, const MidiMessage& message) override
    {
        // This is called on the MIDI thread
        const ScopedLock sl(midiMonitorLock);
        incomingMessages.add(message);
        triggerAsyncUpdate();

        //--- To do Jippo
        mididevices->sendToOutputs(message);
    }

    void handleAsyncUpdate() override
    {
        // This is called on the message loop
        //Array<MidiMessage> messages;

        //{
        //    const ScopedLock sl(midiMonitorLock);
        //    messages.swapWith(incomingMessages);
        //}

        //String messageText;

        //for (auto& m : messages)
        //    messageText << m.getDescription() << "\n";

        //midiMonitor.insertTextAtCaret(messageText);
    }

    //-----------------------------------------------------------------------------
    bool hasDeviceListChanged(const Array<MidiDeviceInfo>& availableDevices, bool isInputDevice)
    {
        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? mididevices->midiInputs
            : mididevices->midiOutputs;

        if (availableDevices.size() != midiDevices.size())
            return true;

        for (auto i = 0; i < availableDevices.size(); ++i)
            if (availableDevices[i] != midiDevices[i]->deviceInfo)
                return true;


        return false;
    }

    //-----------------------------------------------------------------------------
    ReferenceCountedObjectPtr<MidiDeviceListEntry> findDevice(MidiDeviceInfo device, bool isInputDevice) const
    {
        const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? mididevices->midiInputs
            : mididevices->midiOutputs;

        for (auto& d : midiDevices)
            if (d->deviceInfo == device)
                return d;

        return nullptr;
    }

    //-----------------------------------------------------------------------------
    void closeUnpluggedDevices(const Array<MidiDeviceInfo>& currentlyPluggedInDevices, bool isInputDevice)
    {
        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? mididevices->midiInputs
            : mididevices->midiOutputs;

        for (auto i = midiDevices.size(); --i >= 0;)
        {
            auto& d = *midiDevices[i];

            if (!currentlyPluggedInDevices.contains(d.deviceInfo))
            {
                if (isInputDevice ? d.inDevice.get() != nullptr
                    : d.outDevice.get() != nullptr)
                    mididevices->closeDevice(isInputDevice, i);

                midiDevices.remove(i);
            }
        }
    }

    //-----------------------------------------------------------------------------
    void updateDeviceList(bool isInputDeviceList)
    {
        auto availableDevices = isInputDeviceList ? MidiInput::getAvailableDevices()
            : MidiOutput::getAvailableDevices();

        if (hasDeviceListChanged(availableDevices, isInputDeviceList))
        {

            ReferenceCountedArray<MidiDeviceListEntry>& midiDevices
                = isInputDeviceList ? mididevices->midiInputs : mididevices->midiOutputs;

            closeUnpluggedDevices(availableDevices, isInputDeviceList);

            ReferenceCountedArray<MidiDeviceListEntry> newDeviceList;

            // add all currently plugged-in devices to the device list
            for (auto& newDevice : availableDevices)
            {
                MidiDeviceListEntry::Ptr entry = findDevice(newDevice, isInputDeviceList);

                if (entry == nullptr)
                    entry = new MidiDeviceListEntry(newDevice);

                newDeviceList.add(entry);
            }

            // actually update the device list
            midiDevices = newDeviceList;

            applyStickyMidiPortsForList(isInputDeviceList);

            // update the selection status of the combo-box
            if (auto* midiSelector = isInputDeviceList ? midiInputSelector.get() : midiOutputSelector.get())
                midiSelector->syncSelectedItemsWithDeviceList(midiDevices);
        }
    }

    void updateDeviceLists()
    {
        for (const auto isInput : { true, false })
            updateDeviceList(isInput);

        midiModulesToOutputChannels();
    }

    //-----------------------------------------------------------------------------
    MidiDeviceListConnection connection;

    //-----------------------------------------------------------------------------
    // Map Static Supported Midi Mpdules assigned to Button Groups to Channels in order to
    // support quick Channel to output Module during realtime Note Routing
    bool midiModulesToOutputChannels() {

        const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = mididevices->midiOutputs;

        // Reset all channel output module mappings.
        for (int i = 0; i < 17; i++)
            mididevices->moduleout[i].clearQuick();

        // Update all the Channel outs for every Button Group
        for (int i = 0; i < numberbuttongroups; i++) {

            bool bfound = false;

            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(i);
            int grpmidimod = ptrbuttongroup->getMidiModule();
            int grpmidichan = ptrbuttongroup->midiout;

            if (grpmidichan <= 0 || grpmidichan >= 17)
                continue;

            const int moduleCount = instrumentmodules->getNumModules();
            if (grpmidimod < 0 || grpmidimod >= moduleCount)
            {
                juce::Logger::writeToLog("ModuleMapper: Invalid module index on Button Group " + ptrbuttongroup->groupname
                    + " and Channel " + std::to_string(grpmidichan));
                continue;
            }

            // Lookup Module short/search string and find match in the active Modules.
            const String modidstring = instrumentmodules->getModuleIdString(grpmidimod).trim().toLowerCase();
            if (modidstring.isEmpty())
            {
                juce::Logger::writeToLog("ModuleMapper: Empty module id on Button Group " + ptrbuttongroup->groupname
                    + " and Channel " + std::to_string(grpmidichan));
                continue;
            }

            const bool genericMidiMatch = (modidstring == "midi");

            int outmod = 0;
            for (auto& dev : midiDevices) {
                String strdevicename = dev->deviceInfo.name.toLowerCase();

                // Avoid broad generic "midi" substring matches (e.g. "Integra ... MIDI 1")
                // which can incorrectly map unrelated modules.
                bfound = genericMidiMatch
                    ? (strdevicename.startsWith("midi") || strdevicename == "midi")
                    : strdevicename.contains(modidstring);
                if (bfound) {
                    // Route this output channel to the configured module (fan-out safe).
                    mididevices->moduleout[grpmidichan].addIfNotAlreadyThere(outmod);

                    juce::Logger::writeToLog("ModuleMapper: Out Device " + dev->deviceInfo.name
                        + " on Button Group " + ptrbuttongroup->groupname
                        + " to Channel " + std::to_string(grpmidichan)
                    );
                    break;
                }

                outmod++;
            }

            // No match for Button Group module specified
            if (bfound == false) {
                juce::Logger::writeToLog("ModuleMapper: No match for Button Group " + ptrbuttongroup->groupname
                    + " and Channel " + std::to_string(grpmidichan)
                );
            }
        }

        for (int i = 0; i < 17; i++) {
            juce::String mappedModules;
            for (int j = 0; j < mididevices->moduleout[i].size(); ++j)
            {
                if (j > 0)
                    mappedModules << ", ";
                mappedModules << juce::String(mididevices->moduleout[i].getUnchecked(j));
            }
            DBG("MIDI Out channel " + juce::String(i)
                + " to [" + mappedModules + "]"
            );
        }

        return true;
    }

    //-----------------------------------------------------------------------------
    // Quit the original WIndows Application.
    void ApplicationQuit() {
        if (shutdownRequested)
            return;

        shutdownRequested = true;

        // Stop live callbacks before scheduling app shutdown.
        connection.reset();
        filechooser.reset();
        juce::PopupMenu::dismissAllActiveMenus();
        if (midiInputSelector != nullptr)
            midiInputSelector->setEnabled(false);
        if (midiOutputSelector != nullptr)
            midiOutputSelector->setEnabled(false);

        if (auto* topLevel = getTopLevelComponent())
        {
            auto safeTop = juce::Component::SafePointer<juce::Component>(topLevel);
            juce::MessageManager::callAsync([safeTop]()
                {
                    if (safeTop != nullptr)
                        safeTop->userTriedToCloseWindow();
                });
            return;
        }

        if (auto* app = juce::JUCEApplication::getInstance())
        {
            juce::MessageManager::callAsync([app]()
                {
                    app->systemRequestedQuit();
                });
        }
    }

    //-----------------------------------------------------------------------------
    Label midiInputLabel{ "Midi Input Label",  "MIDI Input:" };
    Label midiOutputLabel{ "Midi Output Label", "MIDI Output:" };
    Label incomingMidiLabel{ "Incoming Midi Label", "Received MIDI messages:" };

    MidiKeyboardState keyboardState;
    MidiKeyboardComponent midiKeyboard;

    TextButton pairButton{ "MIDI Bluetooth Devices" };

    TextButton exitButton{ "Exit" };
    TextButton loadConfigButton{ "Load Config" };
    TextButton loadPanelButton{ "Load Panel" };

    TextButton toModule{ "To Modules" };
    TextButton toHelp{ "To Help" };
    TextButton toUpperKBD{ "To Upper" };
    TextButton toLowerKBD{ "To Lower" };
    TextButton toBassKBD{ "To Bass" };
    TextButton toConfig{ "To Config" };

    Label panelPrefixLabel{ "Panel Prefix", "Panel:" };
    Label panelfileLabel{ "Panel File" ,  "Panel File" };
    Label configPrefixLabel{ "Config Prefix", "Config:" };
    Label configfileLabel { "Config File",  "Config File" };
    Label statusLabel{ "" };
    Label buildInfoLabel{ "BuildInfo", "" };

    ////ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;
    std::unique_ptr<MidiDeviceListBox> midiInputSelector, midiOutputSelector;

    InstrumentModules* instrumentmodules = nullptr;

    MidiDevices* mididevices = nullptr;
    InstrumentPanel* instrumentpanel = nullptr;
    MidiInstruments* midiInstruments = nullptr;
    std::function<bool(const juce::String&)> loadConfigFromBasename;
    std::function<void()> refreshKeyboardPanelSaveAvailability;
    std::shared_ptr<std::atomic<bool>> virtualKeyboardInputEnabled;
    AppState& appState;
    bool shutdownRequested = false;

    std::unique_ptr<FileChooser> filechooser;

    std::unique_ptr<BubbleMessageComponent> bubbleMessage;

    CriticalSection midiMonitorLock;

    Array<MidiMessage> incomingMessages;

    //-----------------------------------------------------------------------------
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiStartPage)
};


//==============================================================================
// Class: HelpPage
//==============================================================================
class MarkdownHelpContent final : public Component
{
public:
    void setMarkdownText(const String& markdownText)
    {
        markdown = markdownText;
    }

    void refreshLayoutForWidth(int targetWidth)
    {
        const int safeWidth = jmax(260, targetWidth);
        const float textWidth = (float)jmax(220, safeWidth - 16);
        const int textHeight = buildLayout(textWidth);
        const int targetHeight = jmax(260, textHeight + 16);

        setSize(safeWidth, targetHeight);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));

        g.setColour(juce::Colours::grey.withAlpha(0.22f));
        g.drawRect(getLocalBounds(), 1);

        layout.draw(g, juce::Rectangle<float>(8.0f, 8.0f, (float)getWidth() - 16.0f, (float)getHeight() - 16.0f));
    }

private:
    int buildLayout(float textWidth)
    {
        auto parseLine = [](const String& rawLine, String& outText, Font& outFont, Colour& outColour)
            {
                const String line = rawLine.trimEnd();
                const String trimmed = line.trimStart();

                outText = line;
                outFont = Font(FontOptions(15.0f, Font::plain));
                outColour = Colours::whitesmoke;

                if (trimmed.startsWith("### "))
                {
                    outText = trimmed.fromFirstOccurrenceOf("### ", false, false);
                    outFont = Font(FontOptions(17.0f, Font::bold));
                    outColour = Colours::lightgoldenrodyellow;
                    return;
                }

                if (trimmed.startsWith("## "))
                {
                    outText = trimmed.fromFirstOccurrenceOf("## ", false, false);
                    outFont = Font(FontOptions(20.0f, Font::bold));
                    outColour = Colours::antiquewhite;
                    return;
                }

                if (trimmed.startsWith("# "))
                {
                    outText = trimmed.fromFirstOccurrenceOf("# ", false, false);
                    outFont = Font(FontOptions(24.0f, Font::bold));
                    outColour = Colours::white;
                    return;
                }

                if (trimmed.startsWith("- "))
                {
                    outText = String(CharPointer_UTF8("\xE2\x80\xA2 ")) + trimmed.substring(2);
                    outFont = Font(FontOptions(15.0f, Font::plain));
                    outColour = Colours::whitesmoke;
                    return;
                }

                int digitCount = 0;
                while (digitCount < trimmed.length() && CharacterFunctions::isDigit(trimmed[digitCount]))
                    ++digitCount;
                if (digitCount > 0
                    && digitCount + 1 < trimmed.length()
                    && trimmed[digitCount] == '.'
                    && trimmed[digitCount + 1] == ' ')
                {
                    outText = trimmed;
                    outFont = Font(FontOptions(15.0f, Font::plain));
                    outColour = Colours::whitesmoke;
                    return;
                }

                if (trimmed.isEmpty())
                {
                    outText = "";
                    outFont = Font(FontOptions(8.0f, Font::plain));
                    outColour = Colours::transparentWhite;
                    return;
                }
            };

        AttributedString attributed;
        attributed.setJustification(Justification::topLeft);

        StringArray lines;
        lines.addLines(markdown);
        if (lines.isEmpty())
            lines.add(markdown);

        for (auto& line : lines)
        {
            String parsed;
            Font font(FontOptions(15.0f, Font::plain));
            Colour colour;
            parseLine(line, parsed, font, colour);

            attributed.append(parsed, font, colour);
            attributed.append("\n", Font(FontOptions(6.0f, Font::plain)), Colours::transparentWhite);
        }

        layout.createLayout(attributed, textWidth);
        return juce::roundToInt(layout.getHeight() + 0.5f);
    }

    String markdown;
    TextLayout layout;
};

class HelpPage final : public Component
{
public:
    HelpPage()
    {
        DBG("*** HelpPage(): Constructing HelpPage");

        viewport.reset(new juce::Viewport());
        addAndMakeVisible(viewport.get());
        viewport->setScrollBarsShown(true, true);

        markdownContent.reset(new MarkdownHelpContent());
        viewport->setViewedComponent(markdownContent.get(), false);

        // Load embedded markdown so Help is portable across machines.
        const auto* helpData = reinterpret_cast<const char*>(BinaryData::help_md);
        const auto helpSize = BinaryData::help_mdSize;
        if (helpData != nullptr && helpSize > 0)
        {
            markdownContent->setMarkdownText(juce::String::fromUTF8(helpData, helpSize));
        }
        else
        {
            markdownContent->setMarkdownText("# AMidiOrgan Help\n\nEmbedded help content is not available.");
            juce::Logger::writeToLog("=== HelpPage(): Embedded help.md content was not found in BinaryData");
        }

        resized();
    }

    void resized() override
    {
        const int margin = 10;
        viewport->setBounds(margin, margin, getWidth() - margin * 2, getHeight() - margin * 2);

        const int contentWidth = jmax(260, viewport->getWidth() - viewport->getScrollBarThickness());
        markdownContent->refreshLayoutForWidth(contentWidth);
    }

    void paint(juce::Graphics&) override {}

private:
    std::unique_ptr<juce::Viewport> viewport;
    std::unique_ptr<MarkdownHelpContent> markdownContent;

    //-----------------------------------------------------------------------------
    //JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpPage)
};

class MonitorPage final : public Component,
                          private juce::AsyncUpdater,
                          private juce::MidiKeyboardState::Listener
{
public:
    MonitorPage(std::shared_ptr<std::atomic<bool>> virtualKeyboardInputEnabledState = {})
        : monitorKeyboardState(),
          monitorKeyboard(monitorKeyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
          mididevices(MidiDevices::getInstance()),
          virtualKeyboardInputEnabled(std::move(virtualKeyboardInputEnabledState))
    {
        setOpaque(true);

        monitorGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colours::grey.darker());
        addAndMakeVisible(monitorGroup);
        keyboardGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colours::grey.darker());
        addAndMakeVisible(keyboardGroup);

        addAndMakeVisible(enableButton);
        enableButton.setClickingTogglesState(true);
        enableButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        enableButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        enableButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black.darker());
        enableButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::antiquewhite);
        enableButton.setToggleState(false, juce::dontSendNotification);
        enableButton.setTooltip("Monitoring appends MIDI rows globally while enabled.");
        updateEnableButtonText(false);
        enableButton.onClick = [this]()
            {
                const bool enabled = enableButton.getToggleState();
                monitorEnabled.store(enabled, std::memory_order_relaxed);
                updateEnableButtonText(enabled);
                ensureMonitorHookRegistered();
            };

        addAndMakeVisible(clearButton);
        clearButton.setClickingTogglesState(false);
        clearButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        clearButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        clearButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black.darker());
        clearButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::black.brighter());
        clearButton.setTooltip("Clear monitor text history.");
        clearButton.onClick = [this]()
            {
                monitorTextArea.clear();
            };

        addAndMakeVisible(monitorTextArea);
        monitorTextArea.setMultiLine(true, true);
        monitorTextArea.setReadOnly(true);
        monitorTextArea.setScrollbarsShown(true);
        monitorTextArea.setCaretVisible(false);
        monitorTextArea.setPopupMenuEnabled(true);
        monitorTextArea.setColour(juce::TextEditor::textColourId, juce::Colours::whitesmoke);
        monitorTextArea.setColour(juce::TextEditor::backgroundColourId, findColour(juce::ResizableWindow::backgroundColourId));
        monitorTextArea.setColour(juce::TextEditor::outlineColourId, juce::Colours::grey.withAlpha(0.35f));
        monitorTextArea.setTextToShowWhenEmpty("Enable monitoring to see outgoing MIDI messages.", juce::Colours::grey);

        addAndMakeVisible(startOctaveLabel);
        startOctaveLabel.setText("Start Octave", juce::dontSendNotification);
        startOctaveLabel.setJustificationType(juce::Justification::centredLeft);

        addAndMakeVisible(startOctaveCombo);
        for (int octave = 1; octave <= 8; ++octave)
            startOctaveCombo.addItem("C" + juce::String(octave), octave);
        startOctaveCombo.setSelectedId(3, juce::dontSendNotification);
        startOctaveCombo.onChange = [this]()
            {
                updateKeyboardRangeFromStartOctave();
            };

        addAndMakeVisible(midiChannelLabel);
        midiChannelLabel.setText("MIDI Channel", juce::dontSendNotification);
        midiChannelLabel.setJustificationType(juce::Justification::centredLeft);

        addAndMakeVisible(midiChannelCombo);
        for (int channel = 1; channel <= 16; ++channel)
            midiChannelCombo.addItem(juce::String(channel), channel);
        midiChannelCombo.setSelectedId(1, juce::dontSendNotification);
        midiChannelCombo.onChange = [this]()
            {
                monitorKeyboard.setMidiChannel(juce::jlimit(1, 16, midiChannelCombo.getSelectedId()));
            };

        addAndMakeVisible(monitorKeyboard);
        updateKeyboardRangeFromStartOctave();
        monitorKeyboard.setMidiChannel(1);
        monitorKeyboardState.addListener(this);
        monitorKeyboard.setEnabled(isVirtualKeyboardInputEnabled());
    }

    ~MonitorPage() override
    {
        monitorKeyboardState.removeListener(this);
        if (mididevices != nullptr)
            mididevices->setOutgoingMidiMonitor({});
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(10);
        const int columnGap = 10;
        auto left = r.removeFromLeft(r.getWidth() / 2);
        r.removeFromLeft(columnGap);
        auto right = r;

        monitorGroup.setBounds(left);
        keyboardGroup.setBounds(right);

        auto leftInner = left.reduced(12);
        leftInner.removeFromTop(10);
        auto leftButtons = leftInner.removeFromTop(30);
        enableButton.setBounds(leftButtons.removeFromLeft(100));
        leftButtons.removeFromLeft(8);
        clearButton.setBounds(leftButtons.removeFromLeft(90));
        leftInner.removeFromTop(8);
        monitorTextArea.setBounds(leftInner);

        auto rightInner = right.reduced(12);
        rightInner.removeFromTop(10);
        auto labelsRow = rightInner.removeFromTop(22);
        auto controlsRow = rightInner.removeFromTop(28);
        rightInner.removeFromTop(8);

        auto labelLeft = labelsRow.removeFromLeft(labelsRow.getWidth() / 2);
        labelsRow.removeFromLeft(8);
        auto labelRight = labelsRow;
        startOctaveLabel.setBounds(labelLeft);
        midiChannelLabel.setBounds(labelRight);

        auto comboLeft = controlsRow.removeFromLeft(controlsRow.getWidth() / 2);
        controlsRow.removeFromLeft(8);
        auto comboRight = controlsRow;
        startOctaveCombo.setBounds(comboLeft);
        midiChannelCombo.setBounds(comboRight);

        monitorKeyboard.setBounds(rightInner);
    }

    void setTabActive(bool active)
    {
        if (isTabActive == active) return;
        isTabActive = active;
        monitorKeyboard.setEnabled(isVirtualKeyboardInputEnabled());
        ensureMonitorHookRegistered();
    }

private:
    struct MonitorMessageEntry
    {
        juce::MidiMessage message;
        juce::String moduleName;
    };

    bool isVirtualKeyboardInputEnabled() const
    {
        return virtualKeyboardInputEnabled != nullptr
            && virtualKeyboardInputEnabled->load(std::memory_order_relaxed);
    }

    int getSelectedMidiChannel() const
    {
        return juce::jlimit(1, 16, midiChannelCombo.getSelectedId());
    }

    void updateKeyboardRangeFromStartOctave()
    {
        const int selectedOctave = juce::jlimit(1, 8, startOctaveCombo.getSelectedId());
        const int startNote = juce::jlimit(0, 127, (selectedOctave + 1) * 12);
        const int endNote = juce::jlimit(0, 127, startNote + 47);
        monitorKeyboard.setAvailableRange(startNote, endNote);
    }

    void handleNoteOn(juce::MidiKeyboardState*, int, int midiNoteNumber, float velocity) override
    {
        if (mididevices == nullptr || !isVirtualKeyboardInputEnabled())
            return;

        juce::MidiMessage m(juce::MidiMessage::noteOn(getSelectedMidiChannel(), midiNoteNumber, velocity));
        m.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
        mididevices->handleIncomingMidiMessage(nullptr, m);
    }

    void handleNoteOff(juce::MidiKeyboardState*, int, int midiNoteNumber, float velocity) override
    {
        if (mididevices == nullptr || !isVirtualKeyboardInputEnabled())
            return;

        juce::MidiMessage m(juce::MidiMessage::noteOff(getSelectedMidiChannel(), midiNoteNumber, velocity));
        m.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
        mididevices->handleIncomingMidiMessage(nullptr, m);
    }

    void updateEnableButtonText(bool enabled)
    {
        enableButton.setButtonText(enabled ? "Enabled" : "Disabled");
    }

    void ensureMonitorHookRegistered()
    {
        if (monitorHookInstalled)
            return;

        if (mididevices == nullptr)
            mididevices = MidiDevices::getInstance();

        if (mididevices != nullptr)
        {
            auto safeThis = juce::Component::SafePointer<MonitorPage>(this);
            mididevices->setOutgoingMidiMonitor([safeThis](const juce::MidiMessage& message, const juce::String& routedModuleName)
                {
                    if (safeThis == nullptr)
                        return;
                    if (!safeThis->monitorEnabled.load(std::memory_order_relaxed))
                        return;
                    safeThis->enqueueMessage(message, routedModuleName);
                });
            monitorHookInstalled = true;
        }
    }

    void enqueueMessage(const juce::MidiMessage& message, const juce::String& routedModuleName)
    {
        const juce::ScopedLock lock(queueLock);
        pendingMessages.add({ message, routedModuleName });
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        juce::Array<MonitorMessageEntry> flushMessages;
        {
            const juce::ScopedLock lock(queueLock);
            flushMessages.swapWith(pendingMessages);
        }

        if (flushMessages.isEmpty())
            return;

        juce::String lines;
        lines.preallocateBytes(flushMessages.size() * 48);
        for (const auto& entry : flushMessages)
            lines << formatMessage(entry) << "\n";

        monitorTextArea.moveCaretToEnd();
        monitorTextArea.insertTextAtCaret(lines);
    }

    juce::String formatMessage(const MonitorMessageEntry& entry) const
    {
        const auto& msg = entry.message;
        juce::String description = msg.getDescription();
        if (description.isEmpty())
            description = juce::String::toHexString(msg.getRawData(), msg.getRawDataSize());

        const int channel = msg.getChannel();
        if (channel > 0) {
            if (entry.moduleName.isNotEmpty())
                return "Ch " + juce::String(channel) + " | " + description + " (" + entry.moduleName + ")";
            return "Ch " + juce::String(channel) + " | " + description;
        }

        return description;
    }

    juce::GroupComponent monitorGroup{ "monitorGroup", "MIDI Out Monitor" };
    juce::GroupComponent keyboardGroup{ "keyboardGroup", "Virtual Keyboard" };
    juce::TextButton enableButton{ "Enable" };
    juce::TextButton clearButton{ "Clear" };
    juce::TextEditor monitorTextArea;
    juce::Label startOctaveLabel{ "startOctaveLabel", "Start Octave" };
    juce::ComboBox startOctaveCombo;
    juce::Label midiChannelLabel{ "midiChannelLabel", "MIDI Channel" };
    juce::ComboBox midiChannelCombo;
    juce::MidiKeyboardState monitorKeyboardState;
    juce::MidiKeyboardComponent monitorKeyboard;

    MidiDevices* mididevices = nullptr;
    std::shared_ptr<std::atomic<bool>> virtualKeyboardInputEnabled;

    juce::CriticalSection queueLock;
    juce::Array<MonitorMessageEntry> pendingMessages;

    std::atomic<bool> monitorEnabled { false };
    bool isTabActive = false;
    bool monitorHookInstalled = false;
};


//==============================================================================
// Class: Config Page
//==============================================================================
static int zinstcntvoicespage = 0;
class ConfigPage final : public Component
{
public:
    ConfigPage(TabbedComponent& tabs, std::function<void()> refreshKeyboardPanelSaveAvailabilityFn = {}) :
        instrumentmodules(InstrumentModules::getInstance()),
        mididevices(MidiDevices::getInstance()), // Singleton if there isn't already one
        instrumentpanel(InstrumentPanel::getInstance()), // Singleton if there isn't already one
        appState(getAppState()),
        refreshKeyboardPanelSaveAvailability(std::move(refreshKeyboardPanelSaveAvailabilityFn))
    {
        juce::Logger::writeToLog("== ConfigPage(): Constructor " + std::to_string(zinstcntvoicespage++));

        // 1. Load the system Confg from disk - mostly Button Group IO mappings 
        loadConfigs(appState.configfname);
        updateConfigFileStatusLabel();

        captureModuleIdxBaselineFromPanel();

        // 2. Prepare Midi IO Map router Map by reading initial Confg into IOMap array
        //    for quick access during Midi IO routing (sendToOutputs())
        presetMidiIOMap();

        int xgroup = 10, ygroup = 10, gwidth = 660, gheight = 260;
        addAndMakeVisible(group);
        group.setColour(GroupComponent::outlineColourId, Colours::grey.darker());
        group.setBounds(xgroup, ygroup, gwidth, gheight);

        // Add controls to view and edit Button Group
        // To do: Combobox colors not working and not easy since it consists of multiple items!
        addAndMakeVisible(comboConfig);
        comboConfig.setColour(TextButton::textColourOffId, Colours::black);
        comboConfig.setColour(TextButton::textColourOnId, Colours::black);
        comboConfig.setColour(TextButton::buttonColourId, Colours::lightgrey);
        comboConfig.setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
        comboConfig.setBounds(20, 30, 200, 30);
        comboConfig.setEditableText(false);
        comboConfig.setJustificationType(Justification::centred);
        comboConfig.onChange = [=]() {
            int i = comboConfig.getSelectedId() - 1;   // Combobox is 1 based

            String sgroup = "Group (None): ";
            if (instrumentpanel->getButtonGroup(i)->midikeyboard == KBDUPPER)
            {
                sgroup = "Panel Upper ";
                group.setColour(GroupComponent::outlineColourId, Colours::palevioletred.darker());
            }
            else if (instrumentpanel->getButtonGroup(i)->midikeyboard == KBDLOWER)
            {
                sgroup = "Panel Lower ";
                group.setColour(GroupComponent::outlineColourId, Colours::palegreen.darker());
            }
            else if (instrumentpanel->getButtonGroup(i)->midikeyboard == KBDBASS)
            {
                sgroup = "Panel Bass ";
                group.setColour(GroupComponent::outlineColourId, Colours::antiquewhite.darker());
            }
            //textEditor1.setText(sgroup);
            label11.setText(sgroup, {});

            txtGroupName.setText(instrumentpanel->getButtonGroup(i)->groupname);

            int locmoduleidx = instrumentpanel->getButtonGroup(i)->getMidiModule();
            SoundModule.setButtonText(instrumentmodules->getDisplayName(locmoduleidx));
            txtModuleAlias.setText(instrumentpanel->getButtonGroup(i)->modulealias, juce::dontSendNotification);

            //textEditor3.setText(std::to_string(instrumentpanel->getButtonGroup(i)->buttoncount));
            label31.setText(std::to_string(instrumentpanel->getButtonGroup(i)->buttoncount), {});

            txtMidiIn.setText(std::to_string(instrumentpanel->getButtonGroup(i)->midiin));
            txtMidiOut.setText(std::to_string(instrumentpanel->getButtonGroup(i)->midiout));

            //textEditor6.setText(std::to_string(instrumentpanel->getButtonGroup(i)->splitout));
            int notenum = instrumentpanel->getButtonGroup(i)->splitout;
            String notename = getNoteName(notenum);
            txtSplit.setText(notename);

            if ((i == 3) || (i == 7))
                txtSplit.setEnabled(true);
            else
                txtSplit.setEnabled(false);

            txtOctave.setText(std::to_string(instrumentpanel->getButtonGroup(i)->octxpose));

            if (instrumentpanel->getButtonGroup(i)->velocity == true)
                toggleVelocity.setToggleState(true, dontSendNotification);
            else
                toggleVelocity.setToggleState(false, dontSendNotification);
            };

        for (int i = 1; i < 13; ++i) {
            String sgroup = "Group (None): ";
            if (instrumentpanel->getButtonGroup(i - 1)->midikeyboard == KBDUPPER) sgroup = "Panel Upper: ";
            else if (instrumentpanel->getButtonGroup(i - 1)->midikeyboard == KBDLOWER) sgroup = "Panel Lower: ";
            else if (instrumentpanel->getButtonGroup(i - 1)->midikeyboard == KBDBASS) sgroup = "Panel Bass: ";

            comboConfig.addItem(sgroup + instrumentpanel->getButtonGroup(i - 1)->groupname, i);
        }

        // Select first itemin dropdown on creation
        comboConfig.setSelectedId(1);

        addAndMakeVisible(lblModule);
        lblModule.setBounds(400, 30, 100, 30);
        lblModule.setText("Sound Module", {});

        addAndMakeVisible(SoundModule);
        SoundModule.setButtonText(instrumentmodules->getDisplayName(appState.moduleidx));
        SoundModule.setClickingTogglesState(false);
        SoundModule.setColour(TextButton::textColourOffId, Colours::white);
        SoundModule.setColour(TextButton::textColourOnId, Colours::white);
        SoundModule.setColour(TextButton::buttonColourId, Colours::black);
        SoundModule.setColour(TextButton::buttonOnColourId, Colours::black);
        SoundModule.setBounds(500, 30, 160, 30);
        SoundModule.setToggleState(true, dontSendNotification);
        SoundModule.onClick = [=]()
            {
                PopupMenu menu;
                int icount = instrumentmodules->getNumModules();
                // Do not use 0 as the index count as 0 is used to indicate no selection
                for (int i = 1; i <= icount; ++i)
                {
                    menu.addItem(i, instrumentmodules->getDisplayName(i - 1), true, true);
                }
                menu.showMenuAsync(PopupMenu::Options{}.withTargetComponent(SoundModule),
                    [=](int result) {
                        // If incomplete selection, abort
                        if (result == 0) return;

                        int locmoduleidx = result - 1;
                        int i = comboConfig.getSelectedId() - 1;
                        instrumentpanel->getButtonGroup(i)->setMidiModule(locmoduleidx);

                        SoundModule.setButtonText(instrumentmodules->getDisplayName(locmoduleidx));

                        setConfigSaveButtonEnabled(true);
                    });
            };

        addAndMakeVisible(lblModuleAlias);
        lblModuleAlias.setBounds(400, 75, 100, 24);
        lblModuleAlias.setText("Module Alias", {});

        addAndMakeVisible(txtModuleAlias);
        txtModuleAlias.setBounds(560, 75, 80, 24);
        txtModuleAlias.setInputRestrictions(2, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        txtModuleAlias.onFocusLost = [this]()
            {
                const int i = comboConfig.getSelectedId() - 1;   // Combobox is 1 based
                if (i < 0 || i >= numberbuttongroups)
                    return;

                ButtonGroup* const bg = instrumentpanel->getButtonGroup(i);
                const juce::String previousAlias = bg->modulealias;
                const juce::String trimmed = txtModuleAlias.getText().trim();

                if (trimmed.isEmpty())
                {
                    txtModuleAlias.setText("", juce::dontSendNotification);
                    if (previousAlias.isNotEmpty())
                    {
                        bg->modulealias = "";
                        appState.configchanged = true;
                        setConfigSaveButtonEnabled(true);
                    }
                    return;
                }

                const bool validAlias = (trimmed.length() == 2) && trimmed.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
                if (!validAlias)
                {
                    txtModuleAlias.setText(previousAlias, juce::dontSendNotification);
                    return;
                }

                const juce::String normalized = trimmed.toUpperCase();
                txtModuleAlias.setText(normalized, juce::dontSendNotification);

                if (previousAlias != normalized)
                {
                    bg->modulealias = normalized;
                    appState.configchanged = true;
                    setConfigSaveButtonEnabled(true);
                }
            };

        addAndMakeVisible(lblKeyboard);
        lblKeyboard.setBounds(20, 75, 200, 24);
        lblKeyboard.setText("Midi Keyboard", {});

        //addAndMakeVisible(textEditor1);
        //textEditor1.setBounds(170, 55, 200, 24);
        //textEditor1.setText("Keyboard");
        //textEditor1.setReadOnly(true);

        addAndMakeVisible(label11);
        label11.setBounds(180, 75, 200, 24);
        label11.setText("", {});

        addAndMakeVisible(lblGroupName);
        lblGroupName.setBounds(20, 105, 200, 24);
        lblGroupName.setText("Button Group Name", {});

        addAndMakeVisible(txtGroupName);
        txtGroupName.setBounds(180, 105, 200, 24);
        txtGroupName.setText("Group Name");
        txtGroupName.onFocusLost = [=]() {
            int i = comboConfig.getSelectedId() - 1;   // Combobox is 1 based
            instrumentpanel->getButtonGroup(i)->groupname = txtGroupName.getText();

            setConfigSaveButtonEnabled(true);
            };

        addAndMakeVisible(lblButtonCount);
        lblButtonCount.setBounds(400, 105, 200, 24);
        lblButtonCount.setText("Button Group Count", {});

        //addAndMakeVisible(textEditor3);
        //textEditor3.setBounds(170, 115, 200, 24);
        //textEditor3.setText("Group Count");
        //textEditor3.setReadOnly(true);

        addAndMakeVisible(label31);
        label31.setBounds(560, 105, 200, 24);
        label31.setText("", {});

        addAndMakeVisible(lblMidiIn);
        lblMidiIn.setBounds(20, 135, 200, 24);
        lblMidiIn.setText("Midi In (1 - 16)", {});

        addAndMakeVisible(txtMidiIn);
        txtMidiIn.setBounds(180, 135, 100, 24);
        txtMidiIn.setText("Midi In");
        txtMidiIn.onFocusLost = [=]() {
            int i = comboConfig.getSelectedId() - 1;   // Combobox is 1 based

            int val = txtMidiIn.getText().getIntValue();
            if ((val < 1) || (val > 16)) {
                txtMidiIn.setText("");

                setConfigSaveButtonEnabled(false);
                return;
            }

            instrumentpanel->getButtonGroup(i)->midiin = txtMidiIn.getText().getIntValue();

            setConfigSaveButtonEnabled(true);
            };

        addAndMakeVisible(lblMidiOut);
        lblMidiOut.setBounds(400, 135, 200, 24);
        lblMidiOut.setText("Midi Out (1 - 16)", {});

        addAndMakeVisible(txtMidiOut);
        txtMidiOut.setBounds(560, 135, 100, 24);
        txtMidiOut.setText("Midi Out");
        txtMidiOut.onFocusLost = [=]() {
            int i = comboConfig.getSelectedId() - 1;   // Combobox is 1 based

            int val = txtMidiOut.getText().getIntValue();
            if ((val < 1) || (val > 16)) {
                txtMidiOut.setText("");

                setConfigSaveButtonEnabled(false);
                return;
            }
            instrumentpanel->getButtonGroup(i)->midiout = txtMidiOut.getText().getIntValue();

            setConfigSaveButtonEnabled(true);
            };

        // Keyboard Split
        addAndMakeVisible(lblSplit);
        lblSplit.setBounds(20, 195, 200, 24);
        lblSplit.setText("Solo Out Split", {});

        //https://studiocode.dev/resources/midi-middle-c/
        addAndMakeVisible(txtSplit);
        txtSplit.setEnabled(false);
        txtSplit.setBounds(180, 195, 100, 24);
        txtSplit.setText("--");
        txtSplit.onFocusLost = [=]() {
            int i = comboConfig.getSelectedId() - 1;   // Combobox is 1 based

            String sval = txtSplit.getText();
            if ((sval.length() != 2) && (sval.length() != 3)) {
                txtSplit.setText("--");

                setConfigSaveButtonEnabled(false);
                return;
            }

            int splitoutnumber = getNoteNumber(sval);
            if (splitoutnumber != 0) {
                instrumentpanel->getButtonGroup(i)->splitout = splitoutnumber;
                instrumentpanel->getButtonGroup(i)->splitoutname = sval;
            }
            else {
                instrumentpanel->getButtonGroup(i)->splitout = 0;
                instrumentpanel->getButtonGroup(i)->splitoutname = "--";
            }

            setConfigSaveButtonEnabled(true);
            };

        // Octave Transpose
        addAndMakeVisible(lblOctave);
        lblOctave.setBounds(20, 165, 200, 24);
        lblOctave.setText("Oct Transpose (+/-3)", {});

        addAndMakeVisible(txtOctave);
        txtOctave.setBounds(180, 165, 100, 24);
        txtOctave.setText("0");
        txtOctave.onFocusLost = [=]() {
            int i = comboConfig.getSelectedId() - 1;   // Combobox is 1 based

            int val = txtOctave.getText().getIntValue();
            if ((val < -3) || (val > 3)) {
                txtOctave.setText("");

                setConfigSaveButtonEnabled(false);
                return;
            }
            instrumentpanel->getButtonGroup(i)->octxpose = txtOctave.getText().getIntValue();

            setConfigSaveButtonEnabled(true);
            };

        // Keyboard Velocity on or off
        addAndMakeVisible(lblVelocity);
        lblVelocity.setBounds(20, 225, 200, 24);
        lblVelocity.setText("Velocity Out", {});

        addAndMakeVisible(toggleVelocity);
        toggleVelocity.setBounds(180, 225, 100, 24);
        toggleVelocity.onClick = [=]() {
            velocitystate = toggleVelocity.getToggleState();

            int i = comboConfig.getSelectedId() - 1;   // Combobox is 1 based
            instrumentpanel->getButtonGroup(i)->velocity = velocitystate;

            setConfigSaveButtonEnabled(true);

            juce::String stateString = velocitystate ? "ON" : "OFF";
            juce::Logger::outputDebugString("Velocity changed to " + stateString);
        };

        // Two columns (5 + 5) inside "Effects Defaults" group (border matches Button Group Configs).
        const int defFxLabelX0 = 750;
        const int defFxLabelW = 118; // room for "Effect …" labels
        const int defFxTxtW = 48;
        const int defFxFieldGap = 6;
        const int defFxColGap = 10;
        const int defFxColStride = defFxLabelW + defFxFieldGap + defFxTxtW + defFxColGap;
        const int defFxY0 = 36;
        const int defFxRowH = 24; // same height as Button Group row fields (e.g. txtMidiIn)
        const int defFxDy = 28;
        const int defFxPad = 10;
        const int defFxInnerRight = defFxLabelX0 + defFxColStride + defFxLabelW + defFxFieldGap + defFxTxtW;
        const int defFxBoxLeft = defFxLabelX0 - defFxPad;
        const int defFxBoxTop = 10;
        const int defFxBoxW = (defFxInnerRight - defFxBoxLeft) + defFxPad;
        const int defFxBoxH = (defFxY0 + 4 * defFxDy + defFxRowH) - defFxBoxTop + defFxPad;

        addAndMakeVisible(effectsDefaultsGroup);
        effectsDefaultsGroup.setColour(GroupComponent::outlineColourId, Colours::antiquewhite);
        effectsDefaultsGroup.setBounds(defFxBoxLeft, defFxBoxTop, defFxBoxW, defFxBoxH);

        int defFxRowInCol[2] = { 0, 0 };
        auto placeDefaultEffectRow = [&](juce::Label& lbl, juce::TextEditor& txt, const String& labelText, int column)
        {
            jassert(column == 0 || column == 1);
            const int row = defFxRowInCol[column]++;
            const int xL = defFxLabelX0 + column * defFxColStride;
            const int y = defFxY0 + row * defFxDy;
            lbl.setBounds(xL, y, defFxLabelW, defFxRowH);
            lbl.setText(labelText, {});
            txt.setBounds(xL + defFxLabelW + defFxFieldGap, y, defFxTxtW, defFxRowH);
        };

        addAndMakeVisible(lblDefaultEffectsVol);
        addAndMakeVisible(txtDefaultEffectsVol);
        placeDefaultEffectRow(lblDefaultEffectsVol, txtDefaultEffectsVol, "Effect Vol", 0);
        txtDefaultEffectsVol.setText(std::to_string(appState.defaultEffectsVol));
        wireDefaultEffectsNumberField(txtDefaultEffectsVol, &appState.defaultEffectsVol, 0, 127);

        addAndMakeVisible(lblDefaultEffectsBri);
        addAndMakeVisible(txtDefaultEffectsBri);
        placeDefaultEffectRow(lblDefaultEffectsBri, txtDefaultEffectsBri, "Effect Bri", 0);
        txtDefaultEffectsBri.setText(std::to_string(appState.defaultEffectsBri));
        wireDefaultEffectsNumberField(txtDefaultEffectsBri, &appState.defaultEffectsBri, 0, 127);

        addAndMakeVisible(lblDefaultEffectsExp);
        addAndMakeVisible(txtDefaultEffectsExp);
        placeDefaultEffectRow(lblDefaultEffectsExp, txtDefaultEffectsExp, "Effect Exp", 0);
        txtDefaultEffectsExp.setText(std::to_string(appState.defaultEffectsExp));
        wireDefaultEffectsNumberField(txtDefaultEffectsExp, &appState.defaultEffectsExp, 0, 127);

        addAndMakeVisible(lblDefaultEffectsRev);
        addAndMakeVisible(txtDefaultEffectsRev);
        placeDefaultEffectRow(lblDefaultEffectsRev, txtDefaultEffectsRev, "Effect Rev", 0);
        txtDefaultEffectsRev.setText(std::to_string(appState.defaultEffectsRev));
        wireDefaultEffectsNumberField(txtDefaultEffectsRev, &appState.defaultEffectsRev, 0, 127);

        addAndMakeVisible(lblDefaultEffectsCho);
        addAndMakeVisible(txtDefaultEffectsCho);
        placeDefaultEffectRow(lblDefaultEffectsCho, txtDefaultEffectsCho, "Effect Cho", 0);
        txtDefaultEffectsCho.setText(std::to_string(appState.defaultEffectsCho));
        wireDefaultEffectsNumberField(txtDefaultEffectsCho, &appState.defaultEffectsCho, 0, 127);

        addAndMakeVisible(lblDefaultEffectsMod);
        addAndMakeVisible(txtDefaultEffectsMod);
        placeDefaultEffectRow(lblDefaultEffectsMod, txtDefaultEffectsMod, "Effect Mod", 1);
        txtDefaultEffectsMod.setText(std::to_string(appState.defaultEffectsMod));
        wireDefaultEffectsNumberField(txtDefaultEffectsMod, &appState.defaultEffectsMod, 0, 127);

        addAndMakeVisible(lblDefaultEffectsTim);
        addAndMakeVisible(txtDefaultEffectsTim);
        placeDefaultEffectRow(lblDefaultEffectsTim, txtDefaultEffectsTim, "Effect Tim", 1);
        txtDefaultEffectsTim.setText(std::to_string(appState.defaultEffectsTim));
        wireDefaultEffectsNumberField(txtDefaultEffectsTim, &appState.defaultEffectsTim, 0, 127);

        addAndMakeVisible(lblDefaultEffectsAtk);
        addAndMakeVisible(txtDefaultEffectsAtk);
        placeDefaultEffectRow(lblDefaultEffectsAtk, txtDefaultEffectsAtk, "Effect Atk", 1);
        txtDefaultEffectsAtk.setText(std::to_string(appState.defaultEffectsAtk));
        wireDefaultEffectsNumberField(txtDefaultEffectsAtk, &appState.defaultEffectsAtk, 0, 127);

        addAndMakeVisible(lblDefaultEffectsRel);
        addAndMakeVisible(txtDefaultEffectsRel);
        placeDefaultEffectRow(lblDefaultEffectsRel, txtDefaultEffectsRel, "Effect Rel", 1);
        txtDefaultEffectsRel.setText(std::to_string(appState.defaultEffectsRel));
        wireDefaultEffectsNumberField(txtDefaultEffectsRel, &appState.defaultEffectsRel, 0, 127);

        addAndMakeVisible(lblDefaultEffectsPan);
        addAndMakeVisible(txtDefaultEffectsPan);
        placeDefaultEffectRow(lblDefaultEffectsPan, txtDefaultEffectsPan, "Effect Pan", 1);
        txtDefaultEffectsPan.setText(std::to_string(appState.defaultEffectsPan));
        wireDefaultEffectsNumberField(txtDefaultEffectsPan, &appState.defaultEffectsPan, 0, 127);

        styleDefaultEffectsLikeButtonGroupSection();

        addAndMakeVisible(loadConfigButton);
        loadConfigButton.setButtonText("Load Config");
        loadConfigButton.setColour(TextButton::textColourOffId, Colours::white);
        loadConfigButton.setColour(TextButton::textColourOnId, Colours::white);
        loadConfigButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        loadConfigButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        loadConfigButton.setBounds(1140, 235, 80, 30);
        loadConfigButton.setToggleState(false, dontSendNotification);
        loadConfigButton.onClick = [=]()
            {
                File fileToSave = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    .getChildFile(organdir)
                    .getChildFile(appState.configdir);

                bool useNativeVersion = false;
                filechooser.reset(new FileChooser("Select Config file to load",
                    File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    .getChildFile(organdir)
                    .getChildFile(appState.configdir),
                    "*.cfg", useNativeVersion));

                filechooser->launchAsync(
                    FileBrowserComponent::openMode
                    | FileBrowserComponent::canSelectFiles
                    | FileBrowserComponent::filenameBoxIsReadOnly,
                    [this](const FileChooser& chooser)
                    {
                        String selectedfname = "";
                        String selectedfullpathfname;
                        auto results = chooser.getURLResults();

                        //for (auto result : results)
                        //    selectedfname << (result.isLocalFile() ? result.getLocalFile().getFileName() //.getFullPathName()
                        //        : result.toString(false));

                        for (auto result : results) {
                            if (result.isLocalFile()) {
                                selectedfname = result.getLocalFile().getFileName();
                                selectedfullpathfname = result.getLocalFile().getFullPathName();
                            }
                            else {
                                result.toString(false);
                            }
                        }

                        if (selectedfname == "") return;

                        juce::Logger::writeToLog("*** ConfigPage(): Loading Config file: " + selectedfname);

                        auto bubbleResult = [this](bool bloaded)
                            {
                                String msgloaded;
                                if (bloaded == true)
                                    msgloaded = "Loading:\n " + appState.configfname;
                                else
                                    msgloaded = "Load failed:\n " + appState.configfname;

                                if (TextButton* focused = &loadConfigButton)
                                    BubbleMessage(*focused, msgloaded, this->bubbleMessage);
                            };

                        if (selectedfname == appState.pnlconfigfname)
                        {
                            appState.configPanelPairingMismatchAcknowledged = false;
                            const bool bloaded = loadConfigFileBasename(selectedfname);
                            bubbleResult(bloaded);
                            if (refreshKeyboardPanelSaveAvailability)
                                refreshKeyboardPanelSaveAvailability();
                            return;
                        }

                        const String msg = juce::String("The current instrument panel was saved with \"") + appState.pnlconfigfname
                            + "\".\nYou selected \"" + selectedfname + "\".\n\n"
                            "Routing may not match this song/style until configs align.\n\n"
                            "Load anyway?";

                        auto safeThis = juce::Component::SafePointer<ConfigPage>(this);
                        juce::AlertWindow::showOkCancelBox(
                            juce::AlertWindow::WarningIcon,
                            "Config / panel mismatch",
                            msg,
                            "Load anyway",
                            "Abort",
                            this,
                            juce::ModalCallbackFunction::create([safeThis, selectedfname](int result)
                                {
                                    if (safeThis == nullptr || result != 1)
                                        return;
                                    safeThis->appState.configPanelPairingMismatchAcknowledged = true;
                                    const bool bloaded = safeThis->loadConfigFileBasename(selectedfname);
                                    const juce::String msgloaded = bloaded ? ("Loading:\n " + safeThis->appState.configfname)
                                        : ("Load failed:\n " + selectedfname);
                                    if (juce::TextButton* focused = &safeThis->loadConfigButton)
                                        BubbleMessage(*focused, msgloaded, safeThis->bubbleMessage);
                                    if (safeThis->refreshKeyboardPanelSaveAvailability)
                                        safeThis->refreshKeyboardPanelSaveAvailability();
                                }));
                    });
            };

        // Same horizontal margin as KeyboardPanelPage::mgroup for Save / filename row.
        constexpr int kbPanelMargin = 10;

        addAndMakeVisible(lblconfigfileprefix);
        lblconfigfileprefix.setColour(juce::Label::textColourId, juce::Colours::grey);
        lblconfigfileprefix.setJustificationType(juce::Justification::left);
        lblconfigfileprefix.setText("Config:", {});
        lblconfigfileprefix.setBounds(kbPanelMargin + 1200, 205, 60, 30);

        addAndMakeVisible(lblconfigfile);
        lblconfigfile.setColour(juce::Label::textColourId, juce::Colours::grey);
        lblconfigfile.setJustificationType(juce::Justification::left);
        lblconfigfile.setMinimumHorizontalScale(0.8f);
        // Align with panel filename label on Upper/Lower/Bass (mgroup + 1240, 205, 200x30).
        lblconfigfile.setBounds(kbPanelMargin + 1260, 205, 200, 30);
        updateConfigFileStatusLabel();

        // Quick Access Keyboard Buttons
        int xaccess = 820;
        int mgroup = 10;
        ygroup = 190;
        {
            addAndMakeVisible(toUpperKBD);
            toUpperKBD.setButtonText("Upper");
            toUpperKBD.setClickingTogglesState(false);
            toUpperKBD.setColour(TextButton::textColourOffId, Colours::black);
            toUpperKBD.setColour(TextButton::textColourOnId, Colours::white);
            toUpperKBD.setColour(TextButton::buttonColourId, Colours::white);
            toUpperKBD.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            toUpperKBD.setBounds(mgroup + xaccess, ygroup + mgroup * 2, 80, 50);
            toUpperKBD.setToggleState(true, dontSendNotification);
            toUpperKBD.onClick = [=, &tabs]()
                {
                    tabs.setCurrentTabIndex(PTUpper, false);
                };
            xaccess = xaccess + 100;

            addAndMakeVisible(toLowerKBD);
            toLowerKBD.setButtonText("Lower");
            toLowerKBD.setClickingTogglesState(false);
            toLowerKBD.setColour(TextButton::textColourOffId, Colours::black);
            toLowerKBD.setColour(TextButton::textColourOnId, Colours::white);
            toLowerKBD.setColour(TextButton::buttonColourId, Colours::white);
            toLowerKBD.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            toLowerKBD.setBounds(mgroup + xaccess, ygroup + mgroup * 2, 80, 50);
            toLowerKBD.setToggleState(true, dontSendNotification);
            toLowerKBD.onClick = [=, &tabs]()
                {
                    tabs.setCurrentTabIndex(PTLower, false);
                };
            xaccess = xaccess + 100;

            addAndMakeVisible(toBassKBD);
            toBassKBD.setButtonText("Bass");
            toBassKBD.setClickingTogglesState(false);
            toBassKBD.setColour(TextButton::textColourOffId, Colours::black);
            toBassKBD.setColour(TextButton::textColourOnId, Colours::white);
            toBassKBD.setColour(TextButton::buttonColourId, Colours::white);
            toBassKBD.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            toBassKBD.setBounds(mgroup + xaccess, ygroup + mgroup * 2, 80, 50);
            toBassKBD.setToggleState(true, dontSendNotification);
            toBassKBD.onClick = [=, &tabs]()
                {
                    tabs.setCurrentTabIndex(PTBass, false);
                };
        }

        addAndMakeVisible(globalConfigsGroup);
        globalConfigsGroup.setColour(GroupComponent::outlineColourId, Colours::grey.darker());
        globalConfigsGroup.setBounds(1135, 8, 305, 60);

        addAndMakeVisible(lblPassthrough);
        lblPassthrough.setBounds(1150, 32, 135, 24);
        lblPassthrough.setText("Allow all MIDI In", {});

        //https://docs.juce.com/master/tutorial_radio_buttons_checkboxes.html
        addAndMakeVisible(togglePassthrough);
        togglePassthrough.setBounds(1295, 32, 50, 24);
        togglePassthrough.onClick = [=]() {
            passthroughstate = togglePassthrough.getToggleState();

            setConfigSaveButtonEnabled(true);

            juce::String stateString = passthroughstate ? "ON" : "OFF";
            juce::Logger::outputDebugString("Passthrough changed to " + stateString);
            };

        addAndMakeVisible(resetButton);
        resetButton.setButtonText("MIDI Reset");
        resetButton.setColour(TextButton::textColourOffId, Colours::white);
        resetButton.setColour(TextButton::textColourOnId, Colours::white);
        resetButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        resetButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        resetButton.setBounds(1350, 24, 80, 30);

        resetButton.onClick = [=]() {

            mididevices->resetAllControllers();
            };

        addAndMakeVisible(exitButton);
        exitButton.setButtonText("Exit");
        exitButton.setColour(TextButton::textColourOffId, Colours::white);
        exitButton.setColour(TextButton::textColourOnId, Colours::white);
        exitButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        exitButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        // Keep off the Save As slot (Upper tab uses mgroup+1340,235 for Save As); global Exit is used instead.
        exitButton.setBounds(0, 0, 1, 1);
        // Header-level Exit button is provided globally in AMidiControl.
        exitButton.setVisible(false);

        exitButton.onClick = [=]() {
            ApplicationQuit();
            };

        // Config Status Bar
        addAndMakeVisible(statusLabel);
        statusLabel.setText("", {});
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        statusLabel.setJustificationType(juce::Justification::right);
        statusLabel.setBounds(900, 210, 200, 20);

        // Save / Save As: same bounds as Upper/Lower/Bass panel (mgroup + 1240 / + 1340, y=235). Added last so
        // they stack above quick-access buttons and stay clickable.
        addAndMakeVisible(saveButton);
        saveButton.setButtonText("Save");
        saveButton.setColour(TextButton::textColourOffId, Colours::white);
        saveButton.setColour(TextButton::textColourOnId, Colours::white);
        saveButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        saveButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        saveButton.setTooltip("Save the current config to the existing file.");
        saveButton.setBounds(kbPanelMargin + 1260, 235, 80, 30);
        setConfigSaveButtonEnabled(false);
        saveButton.onClick = [this]() {
            handleConfigSaveRequest(saveButton);
        };

        addAndMakeVisible(saveAsButton);
        saveAsButton.setButtonText("Save As");
        saveAsButton.setColour(TextButton::textColourOffId, Colours::white);
        saveAsButton.setColour(TextButton::textColourOnId, Colours::white);
        saveAsButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        saveAsButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        saveAsButton.setTooltip("Save the current config under a new file name.");
        saveAsButton.setBounds(kbPanelMargin + 1360, 235, 80, 30);
        saveAsButton.setToggleState(false, dontSendNotification);
        saveAsButton.onClick = [this]() {
            File fileToSave = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                .getChildFile(organdir)
                .getChildFile(appState.configdir);

            bool useNativeVersion = false;
            filechooser.reset(new FileChooser("Save Config As: Select existing or create new *.cfg",
                File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    .getChildFile(organdir)
                    .getChildFile(appState.configdir),
                "*.cfg", useNativeVersion));

            const String previousConfigFname = appState.configfname;

            filechooser->launchAsync(
                FileBrowserComponent::saveMode
                | FileBrowserComponent::canSelectFiles
                | FileBrowserComponent::warnAboutOverwriting,
                [this, previousConfigFname](const FileChooser& chooser)
                {
                    bool bsaved = false;

                    auto results = chooser.getURLResults();
                    selconfigfname.clear();

                    for (auto result : results)
                        selconfigfname << (result.isLocalFile() ? result.getLocalFile().getFileName()
                            : result.toString(false));

                    if (selconfigfname.length() != 0) {
                        int extpos = selconfigfname.indexOf(0, ".");
                        if (extpos == -1) {
                            DBG("*** Save Config: Extension not defined as .cfg. Adding extension");

                            selconfigfname = selconfigfname + ".cfg";
                        }

                        appState.configfname = selconfigfname;

                        if (selconfigfname == previousConfigFname)
                        {
                            handleConfigSaveRequest(saveAsButton);
                            updateConfigFileStatusLabel();
                            return;
                        }

                        bsaved = saveConfigs(appState.configfname);

                        appState.configchanged = true;

                        String msgloaded;
                        if (!bsaved) {
                            msgloaded = "Save as failed: " + appState.configfname;
                            juce::Logger::writeToLog("*** ConfigPage(): Save As failed! " + appState.configfname);
                        }
                        else {
                            captureModuleIdxBaselineFromPanel();
                            msgloaded = "Saved as: " + appState.configfname;
                            juce::Logger::writeToLog("*** ConfigPage(): Saved As " + appState.configfname);
                        }

                        if (TextButton* focused = &saveAsButton)
                            BubbleMessage(*focused, msgloaded, this->bubbleMessage);

                        setConfigSaveButtonEnabled(false);
                    }
                    else {
                        ////statusLabel->setText("Empty file name save! " + appState.configfname, juce::dontSendNotification);
                    }

                    updateConfigFileStatusLabel();
                });
        };
    }

    void lookAndFeelChanged() override
    {
        txtGroupName.applyFontToAllText(txtGroupName.getFont());
        styleDefaultEffectsLikeButtonGroupSection();
        updateConfigSaveButtonsPendingStyle();
    }

    void resized() override
    {
        txtGroupName.applyFontToAllText(txtGroupName.getFont());
        styleDefaultEffectsLikeButtonGroupSection();
    }

    /** Load cfg from `configs/` by basename; updates `appState.configfname`, labels, and module baseline when successful. */
    bool loadConfigFileBasename(const String& basename)
    {
        const bool ok = loadConfigs(basename);
        if (!ok)
            return false;

        appState.configfname = basename;
        updateConfigFileStatusLabel();
        appState.configchanged = true;
        comboConfig.setSelectedId(1);
        captureModuleIdxBaselineFromPanel();
        clearConfigPanelPairingMismatchIfAligned(appState);
        return true;
    }

    ~ConfigPage() override {
        DBG("=== ConfigPage(): Destructor " + std::to_string(--zinstcntvoicespage));
    }

private:
    MidiDevices* mididevices = nullptr;
    InstrumentPanel* instrumentpanel = nullptr;
    InstrumentModules* instrumentmodules = nullptr;
    AppState& appState;
    std::function<void()> refreshKeyboardPanelSaveAvailability;

    ValueTree vtconfigs;

    ComboBox comboConfig{ "ConfigCombo" };
    GroupComponent group{ "group", "Button Group Configs" };
    GroupComponent effectsDefaultsGroup{ "effectsDefaultsGroup", "Effects Defaults" };
    GroupComponent globalConfigsGroup{ "globalConfigsGroup", "Global Configs" };
    juce::TextButton SoundModule;
    juce::TextEditor txtGroupName, txtMidiIn, txtMidiOut, txtSplit, txtOctave, txtModuleAlias;
    juce::TextEditor txtDefaultEffectsVol, txtDefaultEffectsBri, txtDefaultEffectsExp, txtDefaultEffectsRev;
    juce::TextEditor txtDefaultEffectsCho, txtDefaultEffectsMod, txtDefaultEffectsTim, txtDefaultEffectsAtk;
    juce::TextEditor txtDefaultEffectsRel, txtDefaultEffectsPan;
    juce::Label lblKeyboard, lblGroupName, lblButtonCount, lblMidiIn, lblMidiOut, lblSplit, lblOctave, lblModule, lblModuleAlias;
    juce::Label lblPassthrough, lblVelocity;
    juce::Label lblDefaultEffectsVol, lblDefaultEffectsBri, lblDefaultEffectsExp, lblDefaultEffectsRev;
    juce::Label lblDefaultEffectsCho, lblDefaultEffectsMod, lblDefaultEffectsTim, lblDefaultEffectsAtk;
    juce::Label lblDefaultEffectsRel, lblDefaultEffectsPan;
    juce::Label lblconfigfileprefix, lblconfigfile, label11, label31;
    juce::ToggleButton togglePassthrough, toggleVelocity;
    juce::TextButton loadConfigButton, saveButton, saveAsButton, resetButton, exitButton;
    juce::TextButton toUpperKBD, toLowerKBD, toBassKBD;
    bool mutestate, velocitystate, passthroughstate;

    String selconfigfname;
    std::unique_ptr<FileChooser> filechooser;

    std::unique_ptr<BubbleMessageComponent> bubbleMessage;

    //juce::StringArray* midinotesnames = 
    juce::StringArray notenames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    juce::Label statusLabel;

    std::array<int, numberbuttongroups> cfgModuleIdxBaseline{};

    void updateConfigFileStatusLabel()
    {
        lblconfigfile.setText(appState.configfname, juce::dontSendNotification);
        lblconfigfile.setTooltip(appState.configfname);
    }

    void captureModuleIdxBaselineFromPanel()
    {
        for (int i = 0; i < numberbuttongroups; ++i)
            cfgModuleIdxBaseline[(size_t) i] = instrumentpanel->getButtonGroup(i)->getMidiModule();
    }

    bool hasHardModuleChangeVersusBaseline() const
    {
        for (int i = 0; i < numberbuttongroups; ++i)
            if (instrumentpanel->getButtonGroup(i)->getMidiModule() != cfgModuleIdxBaseline[(size_t) i])
                return true;

        return false;
    }

    /** Same red “pending save” style as keyboard panel Save/Save As (KeyboardPanelPage::updatePanelSaveButtonsPendingStyle). */
    bool isConfigSavePending() const { return saveButton.isEnabled(); }

    void updateConfigSaveButtonsPendingStyle()
    {
        auto applyStyle = [&](juce::TextButton& button)
            {
                button.setColour(TextButton::textColourOffId, Colours::white);
                button.setColour(TextButton::textColourOnId, Colours::white);

                if (isConfigSavePending())
                {
                    button.setColour(TextButton::buttonColourId, Colours::darkred);
                    button.setColour(TextButton::buttonOnColourId, Colours::darkred.brighter());
                }
                else
                {
                    button.setColour(TextButton::buttonColourId, Colours::black.darker());
                    button.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
                }
            };

        applyStyle(saveButton);
        applyStyle(saveAsButton);
    }

    /** Updates Save enabled state and reapplies Save/Save As colours (dirty = red). */
    void setConfigSaveButtonEnabled(bool enabled)
    {
        static_cast<juce::Button&>(saveButton).setEnabled(enabled);
        updateConfigSaveButtonsPendingStyle();
    }

    void wireDefaultEffectsNumberField(juce::TextEditor& txt, int* valuePtr, int lo, int hi)
    {
        // Digits only, max 3 chars (0–127); commit/range check on focus loss.
        txt.setInputRestrictions(3, "0123456789");
        txt.onFocusLost = [this, &txt, valuePtr, lo, hi]()
        {
            const juce::String t = txt.getText().trim();
            if (t.isEmpty() || ! t.containsOnly("0123456789"))
            {
                txt.setText(std::to_string(*valuePtr), false);
                setConfigSaveButtonEnabled(false);
                return;
            }
            const int val = t.getIntValue();
            if (val < lo || val > hi)
            {
                txt.setText(std::to_string(*valuePtr), false);
                setConfigSaveButtonEnabled(false);
                return;
            }
            *valuePtr = val;
            txt.setText(std::to_string(val), false);
            setConfigSaveButtonEnabled(true);
        };
    }

    void refreshDefaultEffectsEditorsFromAppState()
    {
        txtDefaultEffectsVol.setText(std::to_string(appState.defaultEffectsVol), false);
        txtDefaultEffectsBri.setText(std::to_string(appState.defaultEffectsBri), false);
        txtDefaultEffectsExp.setText(std::to_string(appState.defaultEffectsExp), false);
        txtDefaultEffectsRev.setText(std::to_string(appState.defaultEffectsRev), false);
        txtDefaultEffectsCho.setText(std::to_string(appState.defaultEffectsCho), false);
        txtDefaultEffectsMod.setText(std::to_string(appState.defaultEffectsMod), false);
        txtDefaultEffectsTim.setText(std::to_string(appState.defaultEffectsTim), false);
        txtDefaultEffectsAtk.setText(std::to_string(appState.defaultEffectsAtk), false);
        txtDefaultEffectsRel.setText(std::to_string(appState.defaultEffectsRel), false);
        txtDefaultEffectsPan.setText(std::to_string(appState.defaultEffectsPan), false);
        styleDefaultEffectsLikeButtonGroupSection();
    }

    /** Match the "Button Group Name" row using the same L&F font + colour resolution JUCE uses when painting. */
    void styleDefaultEffectsLikeButtonGroupSection()
    {
        auto& lf = getLookAndFeel();
        const auto labelFont = lf.getLabelFont(lblGroupName);
        // Base LookAndFeel has no getTextEditorFont; match the group-name editor to the L&F label metrics.
        const auto editorFont = txtGroupName.getFont().withHeight(labelFont.getHeight()).withStyle(labelFont.getStyleFlags());
        const auto labelJust = lblGroupName.getJustificationType();

        juce::Label* const labels[] = {
            &lblDefaultEffectsVol, &lblDefaultEffectsBri, &lblDefaultEffectsExp, &lblDefaultEffectsRev, &lblDefaultEffectsCho,
            &lblDefaultEffectsMod, &lblDefaultEffectsTim, &lblDefaultEffectsAtk, &lblDefaultEffectsRel, &lblDefaultEffectsPan
        };
        for (auto* l : labels)
        {
            l->setFont(labelFont);
            l->setJustificationType(labelJust);
            l->setColour(juce::Label::textColourId, lblGroupName.findColour(juce::Label::textColourId, true));
            l->setMinimumHorizontalScale(lblGroupName.getMinimumHorizontalScale());
        }

        juce::TextEditor* const editors[] = {
            &txtDefaultEffectsVol, &txtDefaultEffectsBri, &txtDefaultEffectsExp, &txtDefaultEffectsRev, &txtDefaultEffectsCho,
            &txtDefaultEffectsMod, &txtDefaultEffectsTim, &txtDefaultEffectsAtk, &txtDefaultEffectsRel, &txtDefaultEffectsPan
        };
        for (auto* e : editors)
        {
            e->setColour(juce::TextEditor::textColourId, txtGroupName.findColour(juce::TextEditor::textColourId, true));
            e->setColour(juce::TextEditor::backgroundColourId, txtGroupName.findColour(juce::TextEditor::backgroundColourId, true));
            e->setColour(juce::TextEditor::outlineColourId, txtGroupName.findColour(juce::TextEditor::outlineColourId, true));
            e->setColour(juce::TextEditor::focusedOutlineColourId, txtGroupName.findColour(juce::TextEditor::focusedOutlineColourId, true));
            e->setColour(juce::TextEditor::highlightColourId, txtGroupName.findColour(juce::TextEditor::highlightColourId, true));
            e->setColour(juce::TextEditor::highlightedTextColourId, txtGroupName.findColour(juce::TextEditor::highlightedTextColourId, true));
            e->setFont(editorFont);
            e->applyFontToAllText(editorFont);
        }
    }

    /** Non-empty if two or more button groups share the same MIDI module index and MIDI out channel (invalid routing). */
    String getDuplicateModuleMidiOutAssignmentDetails() const
    {
        using Key = std::pair<int, int>;
        std::map<Key, std::vector<int>> groupsByModuleAndOut;

        for (int i = 0; i < numberbuttongroups; ++i)
        {
            ButtonGroup* bg = instrumentpanel->getButtonGroup(i);
            const int mod = bg->getMidiModule();
            const int outch = bg->midiout;
            groupsByModuleAndOut[{ mod, outch }].push_back(i);
        }

        String lines;
        for (const auto& e : groupsByModuleAndOut)
        {
            const std::vector<int>& idxs = e.second;
            if (idxs.size() < 2)
                continue;

            const int mod = e.first.first;
            const int outch = e.first.second;
            String moduleLabel;
            if (instrumentmodules != nullptr)
            {
                const int n = instrumentmodules->getNumModules();
                if (mod >= 0 && mod < n)
                    moduleLabel = instrumentmodules->getModuleIdString(mod);
                else
                    moduleLabel = "(invalid index " + String(mod) + ")";
            }
            else
            {
                moduleLabel = String(mod);
            }

            String groupsList;
            for (size_t k = 0; k < idxs.size(); ++k)
            {
                if (k > 0)
                    groupsList << ", ";
                groupsList << "\"" << instrumentpanel->getButtonGroup(idxs[k])->groupname << "\"";
            }

            lines << "Module \"" << moduleLabel << "\" (index " << String(mod) << "), MIDI out ch " << String(outch)
                  << ": " << groupsList << "\n";
        }

        return lines;
    }

    void handleConfigSaveRequest(TextButton& bubbleButton)
    {
        if (!hasHardModuleChangeVersusBaseline())
        {
            const bool bsaved = saveConfigs(appState.configfname);

            appState.configchanged = true;

            String msgloaded;
            if (!bsaved) {
                msgloaded = "Config save failed!";
                juce::Logger::writeToLog("*** ConfigPage(): Config save failed! ");
            }
            else {
                msgloaded = "Config saved";
                juce::Logger::writeToLog("*** ConfigPage(): Config saved!");
                captureModuleIdxBaselineFromPanel();
            }

            BubbleMessage(bubbleButton, msgloaded, this->bubbleMessage);

            if (bsaved)
                setConfigSaveButtonEnabled(false);

            return;
        }

        const File organRoot = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir);
        const auto scan = countPanelsReferencingConfigFile(organRoot, appState.configfname);

        if (scan.referencingCount > 0)
        {
            const String msg = String::formatted(
                "This configuration file is referenced by %d instrument panel(s) (scanned %d .pnl files under panels/, instruments/, or legacy paths). "
                "Saving would change the MIDI sound module assignment for one or more button groups. "
                "To keep existing song/style panels valid, use Save As and choose a new file name.",
                scan.referencingCount,
                scan.panelsScanned);

            // JUCE AlertWindow (not NativeMessageBox) so styling matches app LookAndFeel.
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Cannot save configuration",
                msg,
                {},
                this,
                nullptr);
            return;
        }

        const bool bsaved = saveConfigs(appState.configfname);

        appState.configchanged = true;

        String msgloaded;
        if (!bsaved) {
            msgloaded = "Config save failed!";
            juce::Logger::writeToLog("*** ConfigPage(): Config save failed! ");
        }
        else {
            msgloaded = String::formatted(
                "Config saved. Checked %d panel files; none reference this config.",
                scan.panelsScanned);
            juce::Logger::writeToLog("*** ConfigPage(): Config saved!");
            captureModuleIdxBaselineFromPanel();
        }

        BubbleMessage(bubbleButton, msgloaded, this->bubbleMessage);

        if (bsaved)
            setConfigSaveButtonEnabled(false);
    }

    //-------------------------------------------------------------------------
    // Convert Note Names to and from Numbers for range Ocave 2 to 7
    // E.g C4 = 60
    //-------------------------------------------------------------------------
    int getNoteNumber(String& notename) {

        int notenumber = 0;
        bool noteissharp = false;

        // Only allow split on while Notes for now
        int notenamelen = notename.length();
        if ((notenamelen !=2) && (notenamelen != 3)) return notenumber;

        // Check for the '#' indicator
        if (notenamelen == 3) {
            if (notename[1] != '#') return notenumber;
            noteissharp = true;
        }

        // Convert note in string to integer and lookup a match
        int noteoctave = (notename[notenamelen - 1] - 48);
        if ((noteoctave < 2) || (noteoctave > 7)) return notenumber;

        // Now find the C or C# etc. in the list
        int noteinoctave = 0;
        bool bnotefount = false;
        String noteinoctavename = notename.substring(0,notenamelen - 1);
        for (const auto& name : notenames) {
            if (name == noteinoctavename) {
                bnotefount = true;
                break;
            }
            else noteinoctave = noteinoctave + 1;
        }
        if (bnotefount == false) return notenumber;

        notenumber = noteinoctave + (noteoctave + 1) * 12;

        return notenumber;
    }

    String getNoteName(int notenumber) {

        String notename = "0";

        int octave = (notenumber / 12) - 1;
        if ((octave > 1) && (octave < 8)) {
            int noteinoctave = notenumber % 12;
            notename = notenames[noteinoctave] + std::to_string(octave);
        }

        return notename;
    }

    //-------------------------------------------------------------------------
    // Prepare Midi IO Map router Map by reading initial confg into IOMap array
    //-------------------------------------------------------------------------
    bool presetMidiIOMap() {
        int i = 0, j = 0;

        // Start with no routes. Only configured Button Group mappings are allowed.
        for (i = 0; i < 17; i++) {
            for (j = 0; j < 5; j++) {
                mididevices->iomap[i][j] = 0;
            }
        }

        // Preset Octave Transpose by output Midi channel
        for (i = 0; i < 17; i++) {
            mididevices->octxpose[i] = 0;
        }

        // Preset SplitPoint settings by output Midi channel
        for (i = 0; i < 17; i++) {
            mididevices->splitout[i] = 0;
        }

        // Preset Veloctity settings by output Midi channel
        for (i = 0; i < 17; i++) {
            mididevices->velocityout[i] = true;
        }

        // Preset Input Passthrough to false (ignore). To be turned on only for 
        // Button Group input channels and when passthrough option is selected in Config
        // When Config optons is false, the following Button Group settings will override
        // and enable only the configured input channels
        for (i = 0; i < 17; i++) {
            if (passthroughstate == true)
                mididevices->passthroughin[i] = true;
            else
                mididevices->passthroughin[i] = false;

            // Enable Midi Control Channel 16 Passthrough, e.g. for Volume from Organ
            mididevices->passthroughin[16] = true;
        }

        // Now update and add additional channel mappings (layering) 
        for (i = 0; i < numberbuttongroups; i++) {
            ButtonGroup* buttongroup = instrumentpanel->getButtonGroup(i);
            int midiin = buttongroup->midiin;
            int midiout = buttongroup->midiout;

            int octxpose = buttongroup->octxpose;
            int splitout = buttongroup->splitout;
            bool velocityout = buttongroup->velocity;

            // Overwrite the first 0 valued member in midi out array for this Button Group
            j = 0;
            while (j < 5) {
                if (mididevices->iomap[midiin][0] == midiout) break;

                if (mididevices->iomap[midiin][j] == 0) {
                    mididevices->iomap[midiin][j] = midiout;
                    break;
                }
                j++;
            }

            // Map the Octave Transpose for Button Group Midi Output Channel
            if ((midiout >= 0) && (midiout < 17)) {
                mididevices->octxpose[midiout] = octxpose;
                mididevices->splitout[midiout] = splitout;
                mididevices->velocityout[midiout] = velocityout;
            }

            // Enable only Midi Inputs configured in Button Groups if passthrough 
            // opion is selected in Config.
            mididevices->passthroughin[midiin] = true;
        }

        // To do: See how to eliminate static variables
        updateSoloLookups();

        return true;
    }

    //-------------------------------------------------------------------------
    // https://docs.juce.com/master/classValueTree.html#a93d639299ef9dfedc651544e05f06693
    // https://cpp.hotexamples.com/examples/-/ValueTree/-/cpp-valuetree-class-examples.html
    //-------------------------------------------------------------------------
    ValueTree createVTConfigs()
    {
        static Identifier configsType("configs");           // Pre-create an Identifier
        static Identifier buttongroupType("group");         // Child
        static Identifier defaultEffectsVolType("defaultEffectsVol");
        static Identifier defaultEffectsBriType("defaultEffectsBri");
        static Identifier defaultEffectsExpType("defaultEffectsExp");
        static Identifier defaultEffectsRevType("defaultEffectsRev");
        static Identifier defaultEffectsChoType("defaultEffectsCho");
        static Identifier defaultEffectsModType("defaultEffectsMod");
        static Identifier defaultEffectsTimType("defaultEffectsTim");
        static Identifier defaultEffectsAtkType("defaultEffectsAtk");
        static Identifier defaultEffectsRelType("defaultEffectsRel");
        static Identifier defaultEffectsPanType("defaultEffectsPan");

        static Identifier indexType("index");               // Child Properties
        static Identifier midikeyboardType("keyboard");
        static Identifier buttongroupnameType("groupname");
        static Identifier buttongroupcountType("groupcount");

        static Identifier midiinType("midiin");
        static Identifier midioutType("midiout");

        static Identifier octxposeType("octxpose");
        static Identifier splitoutType("splitout");
        static Identifier splitoutnameType("splitoutname");
        static Identifier isvelocityType("isvelocity");

        static Identifier ispassthroughType("ispassthrough");
        static Identifier moduleidxType("moduleidx");
        static Identifier modulealiasType("modulealias");

        // Configs ValueTree
        ValueTree configsTree(configsType);
        configsTree.setProperty(defaultEffectsVolType, appState.defaultEffectsVol, nullptr);
        configsTree.setProperty(defaultEffectsBriType, appState.defaultEffectsBri, nullptr);
        configsTree.setProperty(defaultEffectsExpType, appState.defaultEffectsExp, nullptr);
        configsTree.setProperty(defaultEffectsRevType, appState.defaultEffectsRev, nullptr);
        configsTree.setProperty(defaultEffectsChoType, appState.defaultEffectsCho, nullptr);
        configsTree.setProperty(defaultEffectsModType, appState.defaultEffectsMod, nullptr);
        configsTree.setProperty(defaultEffectsTimType, appState.defaultEffectsTim, nullptr);
        configsTree.setProperty(defaultEffectsAtkType, appState.defaultEffectsAtk, nullptr);
        configsTree.setProperty(defaultEffectsRelType, appState.defaultEffectsRel, nullptr);
        configsTree.setProperty(defaultEffectsPanType, appState.defaultEffectsPan, nullptr);

        for (int i = 0; i < numberbuttongroups; i++) {
            ValueTree vtcfg(buttongroupType);

            vtcfg.setProperty(indexType, i, nullptr);
            //String sgroup = "none";
            //if (instrumentpanel->getButtonGroup(i)->midikeyboard == KBDUPPER) sgroup = "kbdupper";
            //else if (instrumentpanel->getButtonGroup(i)->midikeyboard == KBDLOWER) sgroup = "kbdlower";
            //else if (instrumentpanel->getButtonGroup(i)->midikeyboard == KBDBASS) sgroup = "kbdbass";
            //vtcfg.setProperty(midikeyboardType, sgroup, nullptr);
            vtcfg.setProperty(midikeyboardType, instrumentpanel->getButtonGroup(i)->midikeyboard, nullptr);

            vtcfg.setProperty(buttongroupnameType, instrumentpanel->getButtonGroup(i)->groupname, nullptr);
            vtcfg.setProperty(buttongroupcountType, instrumentpanel->getButtonGroup(i)->buttoncount, nullptr);

            vtcfg.setProperty(midiinType, instrumentpanel->getButtonGroup(i)->midiin, nullptr);
            vtcfg.setProperty(midioutType, instrumentpanel->getButtonGroup(i)->midiout, nullptr);

            vtcfg.setProperty(octxposeType, instrumentpanel->getButtonGroup(i)->octxpose, nullptr);
            vtcfg.setProperty(splitoutType, instrumentpanel->getButtonGroup(i)->splitout, nullptr);
            vtcfg.setProperty(splitoutnameType, instrumentpanel->getButtonGroup(i)->splitoutname, nullptr);

            if (instrumentpanel->getButtonGroup(i)->velocity == true)
                vtcfg.setProperty(isvelocityType, true, nullptr);
            else
                vtcfg.setProperty(isvelocityType, false, nullptr);

            if (passthroughstate == true)
                vtcfg.setProperty(ispassthroughType, true, nullptr);
            else
                vtcfg.setProperty(ispassthroughType, false, nullptr);
            
            vtcfg.setProperty(moduleidxType, instrumentpanel->getButtonGroup(i)->moduleidx, nullptr);
            vtcfg.setProperty(modulealiasType, instrumentpanel->getButtonGroup(i)->modulealias, nullptr);

            // Added new config to parent
            if (vtcfg.isValid()) {
                configsTree.appendChild(vtcfg, nullptr);
            }
            else
            {
                juce::Logger::writeToLog("*** createVTConfigs(): An error occurred while creating Configs Button Group ValueTree...");
                // To do: Add more error handling
            }
        }

        if (!configsTree.isValid()) {
            juce::Logger::writeToLog("*** createVTConfigs(): An error occurred while creating final Configs ValueTree...");
            // To do: Add more error handling
        }

        return configsTree;
    }

    //-------------------------------------------------------------------------
    // https://forum.juce.com/t/example-for-creating-a-file-and-doing-something-with-it/31998
    // Save current Configs to Disk
    //-------------------------------------------------------------------------
    bool saveConfigs(String configfile) {

        DBG("*** saveConfigs(): Saving Config file from disk");

        const String dupDetails = getDuplicateModuleMidiOutAssignmentDetails();
        if (dupDetails.isNotEmpty())
        {
            juce::Logger::writeToLog(
                "*** saveConfigs(): Blocked — duplicate MIDI module + output channel across button groups:\n"
                + dupDetails);
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Cannot save configuration",
                "Two or more button groups use the same MIDI sound module and the same MIDI output channel. "
                "Only one group can use each module/channel pair (otherwise routing is ambiguous).\n\n"
                + dupDetails,
                {},
                this,
                nullptr);
            return false;
        }

        // Prepare to save Instrument Configs to Disk
        File outputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(appState.configdir)
            .getChildFile(configfile);

        if (outputFile.existsAsFile())
        {
            DBG("*** saveConfigs(): Configs file exists! Deleting to store new copy");
            outputFile.deleteFile();
        }
        FileOutputStream output(outputFile);

        vtconfigs = createVTConfigs();

        // For testing
        //String vtxml = vtconfigs.toXmlString();

        //void ValueTree::writeToStream(OutputStream & output)	const
        vtconfigs.writeToStream(output);
        output.flush();
        if (output.getStatus().failed())
        {
            juce::Logger::writeToLog("*** saveConfigs(): Config file save failed " + configfile);
            // To do: Add more error handling
            return false;
        }
        juce::Logger::writeToLog("*** saveConfigs(): Config file saved " + configfile);

        // Roundtip Save/Reload and update vars
        //loadConfigs(configsname);

        // If not save and reload, then update Keyboard Handler fast lookup variables
        presetMidiIOMap();

        // Recalculate Module to Midi Output 
        ModulesToChannelMap();

        return true;
    }


    //-------------------------------------------------------------------------
    // Configs Load from known ValueTree
    //-------------------------------------------------------------------------
    bool loadVTConfigs()
    {

        static Identifier configsType("configs");       // Pre-create an Identifier
        static Identifier buttongroupType("group");     // Child
        static Identifier defaultEffectsVolType("defaultEffectsVol");
        static Identifier defaultEffectsBriType("defaultEffectsBri");
        static Identifier defaultEffectsExpType("defaultEffectsExp");
        static Identifier defaultEffectsRevType("defaultEffectsRev");
        static Identifier defaultEffectsChoType("defaultEffectsCho");
        static Identifier defaultEffectsModType("defaultEffectsMod");
        static Identifier defaultEffectsTimType("defaultEffectsTim");
        static Identifier defaultEffectsAtkType("defaultEffectsAtk");
        static Identifier defaultEffectsRelType("defaultEffectsRel");
        static Identifier defaultEffectsPanType("defaultEffectsPan");

        static Identifier indexType("index");           // Child Properties
        static Identifier midikeyboardType("keyboard");
        static Identifier buttongroupnameType("groupname");
        static Identifier buttongroupcountType("groupcount");

        static Identifier midiinType("midiin");
        static Identifier midioutType("midiout");

        static Identifier octxposeType("octxpose");
        static Identifier splitoutType("splitout");
        static Identifier splitoutnameType("splitoutname");
        static Identifier isvelocityType("isvelocity");

        static Identifier ispassthroughType("ispassthrough");
        static Identifier moduleidxType("moduleidx");
        static Identifier modulealiasType("modulealias");

        if (!(vtconfigs.isValid() && (vtconfigs.getNumChildren() > 0))) {
            juce::Logger::writeToLog("*** loadVTConfigs(): An error occurred while reading temo Configs ValueTree...");

            // To do: Add more error handling
            return false;
        }

        // For testing
        //String vtxml = vtconfigs.toXmlString();

        if (vtconfigs.hasProperty(defaultEffectsVolType))
            appState.defaultEffectsVol = juce::jlimit(0, 127, (int)vtconfigs.getProperty(defaultEffectsVolType));
        else
            appState.defaultEffectsVol = 100;

        if (vtconfigs.hasProperty(defaultEffectsBriType))
            appState.defaultEffectsBri = juce::jlimit(0, 127, (int)vtconfigs.getProperty(defaultEffectsBriType));
        else
            appState.defaultEffectsBri = 30;

        if (vtconfigs.hasProperty(defaultEffectsExpType))
            appState.defaultEffectsExp = juce::jlimit(0, 127, (int)vtconfigs.getProperty(defaultEffectsExpType));
        else
            appState.defaultEffectsExp = 127;

        if (vtconfigs.hasProperty(defaultEffectsRevType))
            appState.defaultEffectsRev = juce::jlimit(0, 127, (int)vtconfigs.getProperty(defaultEffectsRevType));
        else
            appState.defaultEffectsRev = 20;

        if (vtconfigs.hasProperty(defaultEffectsChoType))
            appState.defaultEffectsCho = juce::jlimit(0, 127, (int)vtconfigs.getProperty(defaultEffectsChoType));
        else
            appState.defaultEffectsCho = 10;

        if (vtconfigs.hasProperty(defaultEffectsModType))
            appState.defaultEffectsMod = juce::jlimit(0, 127, (int)vtconfigs.getProperty(defaultEffectsModType));
        else
            appState.defaultEffectsMod = 0;

        if (vtconfigs.hasProperty(defaultEffectsTimType))
            appState.defaultEffectsTim = juce::jlimit(0, 127, (int)vtconfigs.getProperty(defaultEffectsTimType));
        else
            appState.defaultEffectsTim = 0;

        if (vtconfigs.hasProperty(defaultEffectsAtkType))
            appState.defaultEffectsAtk = juce::jlimit(0, 127, (int)vtconfigs.getProperty(defaultEffectsAtkType));
        else
            appState.defaultEffectsAtk = 0;

        if (vtconfigs.hasProperty(defaultEffectsRelType))
            appState.defaultEffectsRel = juce::jlimit(0, 127, (int)vtconfigs.getProperty(defaultEffectsRelType));
        else
            appState.defaultEffectsRel = 0;

        if (vtconfigs.hasProperty(defaultEffectsPanType))
            appState.defaultEffectsPan = juce::jlimit(0, 127, (int)vtconfigs.getProperty(defaultEffectsPanType));
        else
            appState.defaultEffectsPan = 64;

        for (int i = 0; i < numberbuttongroups; i++) {
            ValueTree vtcfg = vtconfigs.getChild(i);

            if (!vtcfg.isValid()) {
                juce::Logger::writeToLog("*** loadVTConfigs(): An error occurred while reading temp Configs ValueTree child: " + std::to_string(i));

                // To do: Add more error handling
                return false;
            }

            if ((int)vtcfg.getProperty(indexType) != i) {
                juce::Logger::writeToLog("*** loadVTConfigs(): An error occurred in Config VaueTree read: " + std::to_string(i));

                // To do: Add more error handling
                return false;
            };

            instrumentpanel->getButtonGroup(i)->midikeyboard = vtcfg.getProperty(midikeyboardType);
            instrumentpanel->getButtonGroup(i)->groupname = vtcfg.getProperty(buttongroupnameType);
            instrumentpanel->getButtonGroup(i)->buttoncount = vtcfg.getProperty(buttongroupcountType);

            instrumentpanel->getButtonGroup(i)->midiin = vtcfg.getProperty(midiinType);
            instrumentpanel->getButtonGroup(i)->midiout = vtcfg.getProperty(midioutType);

            instrumentpanel->getButtonGroup(i)->octxpose = vtcfg.getProperty(octxposeType);
            instrumentpanel->getButtonGroup(i)->splitout = vtcfg.getProperty(splitoutType);
            instrumentpanel->getButtonGroup(i)->splitoutname = vtcfg.getProperty(splitoutnameType);
            instrumentpanel->getButtonGroup(i)->velocity = vtcfg.getProperty(isvelocityType);

            passthroughstate = vtcfg.getProperty(ispassthroughType);
            instrumentpanel->getButtonGroup(i)->moduleidx = vtcfg.getProperty(moduleidxType);

            juce::String alias = vtcfg.getProperty(modulealiasType, "").toString().trim();
            if (alias.isNotEmpty())
            {
                const bool validAlias = (alias.length() == 2) && alias.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
                alias = validAlias ? alias.toUpperCase() : "";
            }
            instrumentpanel->getButtonGroup(i)->modulealias = alias;
        }

        return true;
    }

    //-------------------------------------------------------------------------
    // Load Config File from Disk
    // https://www.includehelp.com/code-snippets/cpp-program-to-write-and-read-an-object-in-from-a-binary-file.aspx
    // https://forum.juce.com/t/example-for-creating-a-file-and-doing-something-with-it/31998/2
    //-------------------------------------------------------------------------
    bool loadConfigs(const String& configsname) {
        juce::Logger::writeToLog("*** loadConfigs(): Loading Config file " + configsname);

        // Reload Value tree to test
        // Prepare to read Configs Panel from Disk
        File inputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(appState.configdir)
            .getChildFile(configsname);

        if (!inputFile.existsAsFile())
        {
            juce::Logger::writeToLog("*** loadConfigs(): No Config file " + configsname + " to load!");

            inputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                .getChildFile(organdir)
                .getChildFile(appState.configdir)
                .getChildFile("masterconfigs.cfg");
            juce::Logger::writeToLog("*** loadConfigs(): Loading file configs/masterconfigs.cfg instead");
        }

        FileInputStream input(inputFile);
        if (input.failedToOpen())
        {
            juce::Logger::writeToLog(" ***loadConfigs(): Config file " + configsname + " did not open! Need to recover from this...");

            return false;
        }
        else
        {
            static Identifier configsType("configs");   // Pre-create an Identifier
            ValueTree temp_vtconfigs(configsType);

            // static ValueTree ValueTree::readFromStream(InputStream & input)
            temp_vtconfigs = temp_vtconfigs.readFromStream(input);

            // For testing
            //String xmlstring = temp_vtconfigs.toXmlString();

            if (!(temp_vtconfigs.isValid() && (temp_vtconfigs.getNumChildren() > 0)))
            {
                juce::Logger::writeToLog("*** loadConfigs(): Panel file read into temp ValueTree failed! No change to Configs");

                // To do: Add some other error handling
                return false;
            }
            else
                vtconfigs = temp_vtconfigs;

            loadVTConfigs();
        }

        // Update Keyboard Handler fast lookup variables
        presetMidiIOMap();

        // Preset MIDI Passthrough checkbox to passthrough state
        if (passthroughstate == true) {
            togglePassthrough.setToggleState(true, dontSendNotification);
        }
        else {
            togglePassthrough.setToggleState(false, dontSendNotification);
        }

        refreshDefaultEffectsEditorsFromAppState();

        return true;
    }

    //-------------------------------------------------------------------------
    // Load Keyboard Handler Quick Lookups
    // To do: Refactor away from static vars!
    //-------------------------------------------------------------------------
    void updateSoloLookups() {

        upperchan = instrumentpanel->getButtonGroup(3)->midiin;
        uppersolo = instrumentpanel->getButtonGroup(3)->midiout;

        lowerchan = instrumentpanel->getButtonGroup(7)->midiin;
        lowersolo = instrumentpanel->getButtonGroup(7)->midiout;
    }

    //-------------------------------------------------------------------------
    // ModulesToChannelMap() - Maps Midi Modules to Output Channels
    //-------------------------------------------------------------------------
    bool ModulesToChannelMap() {

        const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = mididevices->midiOutputs;

        // Reset all channel output module mappings.
        for (int i = 0; i < 17; i++)
            mididevices->moduleout[i].clearQuick();

        // Update all the Channel outs for every Button Group
        for (int i = 0; i < numberbuttongroups; i++) {

            bool bfound = false;

            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(i);
            int grpmidimod = ptrbuttongroup->getMidiModule();
            int grpmidichan = ptrbuttongroup->midiout;

            if (grpmidichan <= 0 || grpmidichan >= 17)
                continue;

            const int moduleCount = instrumentmodules->getNumModules();
            if (grpmidimod < 0 || grpmidimod >= moduleCount)
            {
                juce::Logger::writeToLog("*** ModulesToChannelMap: Invalid module index on Button Group " + ptrbuttongroup->groupname
                    + " and Channel " + std::to_string(grpmidichan));
                continue;
            }

            // Lookup Module short/search string and find match in the active Modules
            const String modidstring = instrumentmodules->getModuleIdString(grpmidimod).trim().toLowerCase();
            if (modidstring.isEmpty())
            {
                juce::Logger::writeToLog("*** ModulesToChannelMap: Empty module id on Button Group " + ptrbuttongroup->groupname
                    + " and Channel " + std::to_string(grpmidichan));
                continue;
            }

            const bool genericMidiMatch = (modidstring == "midi");

            int outmod = 0;
            for (auto& dev : midiDevices) {
                String strdevicename = dev->deviceInfo.name.toLowerCase();

                // Avoid broad generic "midi" substring matches (e.g. "Integra ... MIDI 1")
                // which can incorrectly map unrelated modules.
                bfound = genericMidiMatch
                    ? (strdevicename.startsWith("midi") || strdevicename == "midi")
                    : strdevicename.contains(modidstring);
                if (bfound) {
                    // Route this output channel to the configured module (fan-out safe).
                    mididevices->moduleout[grpmidichan].addIfNotAlreadyThere(outmod);

                    juce::Logger::writeToLog("*** ModulesToChannelMap: Out Device " + strdevicename
                        + " on Button Group " + ptrbuttongroup->groupname
                        + " to Channel " + std::to_string(grpmidichan)
                    );
                    break;
                }

                outmod++;
            }

            // No match for Button Group module specified
            if (bfound == false) {
                juce::Logger::writeToLog("*** ModulesToChannelMap: No match for Button Group " + ptrbuttongroup->groupname
                    + " and Channel " + std::to_string(grpmidichan)
                );
            }
        }

        for (int i = 0; i < 17; i++) {
            juce::String mappedModules;
            for (int j = 0; j < mididevices->moduleout[i].size(); ++j)
            {
                if (j > 0)
                    mappedModules << ", ";
                mappedModules << juce::String(mididevices->moduleout[i].getUnchecked(j));
            }
            DBG("*** ModulesToChannelMap: MIDI Out channel " + juce::String(i)
                + " to [" + mappedModules + "]"
            );
        }

        return true;
    }


    //-------------------------------------------------------------------------
    // Quit the original Windows Application.
    //-------------------------------------------------------------------------
    void ApplicationQuit() {

        juce::Logger::writeToLog("*** ApplicationQuit(): Quiting application");

        filechooser.reset();
        juce::PopupMenu::dismissAllActiveMenus();

        if (auto* topLevel = getTopLevelComponent())
        {
            auto safeTop = juce::Component::SafePointer<juce::Component>(topLevel);
            juce::MessageManager::callAsync([safeTop]()
                {
                    if (safeTop != nullptr)
                        safeTop->userTriedToCloseWindow();
                });
            return;
        }

        if (auto* app = juce::JUCEApplication::getInstance())
        {
            juce::MessageManager::callAsync([app]()
                {
                    app->systemRequestedQuit();
                });
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigPage)
};


//==============================================================================
// Configure the Menu Tabs and load Page Componens into the tabs
//==============================================================================
class MenuTabs final : public TabbedComponent
{
public:
    MenuTabs ([[maybe_unused]] bool isRunningComponenTransforms,
              juce::ApplicationCommandManager& commandManager,
              KeyPressTarget& keyPressTarget)
        : TabbedComponent (TabbedButtonBar::TabsAtTop)
    {
        juce::Logger::writeToLog("== MenuTabs(): Constructor ");

        auto colour = findColour (ResizableWindow::backgroundColourId);

        std::function<void()> refreshKbPanelSaves = [this]()
            {
                for (int i = 0; i < getNumTabs(); ++i)
                    if (auto* k = dynamic_cast<KeyboardPanelPage*>(getTabContentComponent(i)))
                        k->refreshPanelSaveAvailability();
            };

        ConfigPage* configspage = new ConfigPage(*this, refreshKbPanelSaves);
        EffectsPage* effectspage = new EffectsPage(*this);
        VoicesPage* voicespage = new VoicesPage(*this);
        auto virtualKeyboardInputEnabled = std::make_shared<std::atomic<bool>>(false);
        MonitorPage* monitorpage = new MonitorPage(virtualKeyboardInputEnabled);
        effectsPageRef = effectspage;
        voicesPageRef = voicespage;
        monitorPageRef = monitorpage;

        // Create MidiStart Page
        addTab("Start",        colour, new MidiStartPage(*this, [configspage](const juce::String& b)
            {
                return configspage->loadConfigFileBasename(b);
            }, refreshKbPanelSaves, virtualKeyboardInputEnabled), true);

        // Create Upper, Lower and Bass&Drum Pages
        //KeyboardPanelPage* upperpanel = new KeyboardPanelPage(*this, PTUpper, *effectspage, *voicespage);
        addTab("Upper",        colour, new KeyboardPanelPage(*this, PTUpper, *effectspage, *voicespage), true);
        //KeyboardPanelPage* lowerpanel = new KeyboardPanelPage(*this, PTLower, *effectspage, *voicespage);
        addTab("Lower",        colour, new KeyboardPanelPage(*this, PTLower, *effectspage, *voicespage), true);
        //KeyboardPanelPage* basspanel = new KeyboardPanelPage(*this, PTBass, *effectspage, *voicespage);
        addTab("Bass&Drums",   colour, new KeyboardPanelPage(*this, PTBass, *effectspage, *voicespage), true);

        // Create Voices and Effects Pages
        addTab("Sounds",       colour, voicespage, true);
        addTab("Effects",      colour, effectspage, true);

        // Create Config, Hotkeys, Monitor, Help, Exit
        addTab("Config",       colour, configspage, true);
        addTab("Hotkeys",      colour, new HotkeysPage(commandManager, keyPressTarget), true);
        addTab("Monitor",      colour, monitorpage, true);
        addTab("Help",         colour, new HelpPage(), true);
        addTab("Exit",         colour, new Component(), true);

        this->setTabBarDepth(30);

        // Set default tab
        this->setCurrentTabIndex(PTStart, true);
        String sname = this->getCurrentTabName();
        getAppState().lastSelectedPanelButtonIdx = -1;
        getAppState().hasExplicitVoiceSelection = false;

        gNotifyPanelSaveAvailabilityChanged = std::move(refreshKbPanelSaves);

        std::function<void()> syncManualRotaryKb = [this]()
            {
                for (int i = 0; i < getNumTabs(); ++i)
                    if (auto* k = dynamic_cast<KeyboardPanelPage*>(getTabContentComponent(i)))
                        k->applyManualRotaryFromAppState();
            };
        gNotifyManualRotarySyncFromAppState = std::move(syncManualRotaryKb);

        std::function<void()> syncPresetRotaryKb = [this]()
            {
                for (int i = 0; i < getNumTabs(); ++i)
                    if (auto* k = dynamic_cast<KeyboardPanelPage*>(getTabContentComponent(i)))
                        k->applyPresetRotaryUiFromStoredGroups();
            };
        gNotifyPresetRotarySyncFromButtonGroups = std::move(syncPresetRotaryKb);
        gNotifyVoiceEditTabAccessChanged = [this]()
            {
                updateVoiceEditTabAccessUi();
            };
        updateVoiceEditTabAccessUi();

        //this->grabKeyboardFocus();
    }

    ~MenuTabs() override
    {
        gNotifyPanelSaveAvailabilityChanged = {};
        gNotifyManualRotarySyncFromAppState = {};
        gNotifyPresetRotarySyncFromButtonGroups = {};
        gNotifyVoiceEditTabAccessChanged = {};
    }

    /** Phase 1 hotkeys: recall preset on all keyboard pages (single loadPreset + radio sync). */
    void recallPresetFromHotkey(int pstidx)
    {
        if (pstidx < 0 || pstidx >= numberpresets) return;

        auto* ip = InstrumentPanel::getInstance();
        if (ip == nullptr) return;

        ip->panelpresets->setActivePresetIdx(pstidx);

        KeyboardPanelPage* kbForLoad = nullptr;
        for (int i = 0; i < getNumTabs(); ++i)
        {
            if (auto* k = dynamic_cast<KeyboardPanelPage*>(getTabContentComponent(i)))
            {
                if (kbForLoad == nullptr)
                    kbForLoad = k;
                k->setPresetRadioSelection(pstidx);
            }
        }

        if (kbForLoad != nullptr)
            kbForLoad->loadPreset(pstidx);
    }

    void triggerNextPresetHotkey()
    {
        for (int i = 0; i < getNumTabs(); ++i)
        {
            if (auto* k = dynamic_cast<KeyboardPanelPage*>(getTabContentComponent(i)))
            {
                k->triggerNextPresetHotkey();
                break;
            }
        }
    }

    void triggerUpperRotaryFastSlowHotkey()
    {
        if (auto* k = dynamic_cast<KeyboardPanelPage*>(getTabContentComponent(PTUpper)))
            k->triggerRotaryFastSlowHotkey();
    }

    void triggerUpperRotaryBrakeHotkey()
    {
        if (auto* k = dynamic_cast<KeyboardPanelPage*>(getTabContentComponent(PTUpper)))
            k->triggerRotaryBrakeHotkey();
    }

    void triggerLowerRotaryFastSlowHotkey()
    {
        if (auto* k = dynamic_cast<KeyboardPanelPage*>(getTabContentComponent(PTLower)))
            k->triggerRotaryFastSlowHotkey();
    }

    void triggerLowerRotaryBrakeHotkey()
    {
        if (auto* k = dynamic_cast<KeyboardPanelPage*>(getTabContentComponent(PTLower)))
            k->triggerRotaryBrakeHotkey();
    }

    bool canOpenVoiceEditTabs()
    {
        return KeyboardPanelPage::anyVoiceEditShortcutsEnabledInTabs(*this)
            || hasValidLastSelectedVoiceButton();
    }

    void mouseDown(const MouseEvent& e) override
    {
        draggingFromTabBarBackground = false;

        // Enable window drag from empty tab-bar strip space only.
        if (e.originalComponent == this && e.y <= getTabBarDepth())
        {
            if (auto* topLevel = getTopLevelComponent())
            {
                windowDragger.startDraggingComponent(topLevel, e);
                draggingFromTabBarBackground = true;
            }
        }

        TabbedComponent::mouseDown(e);
    }

    void mouseDrag(const MouseEvent& e) override
    {
        if (draggingFromTabBarBackground)
        {
            if (auto* topLevel = getTopLevelComponent())
                windowDragger.dragComponent(topLevel, e, nullptr);
        }

        TabbedComponent::mouseDrag(e);
    }

    void mouseUp(const MouseEvent& e) override
    {
        draggingFromTabBarBackground = false;
        TabbedComponent::mouseUp(e);
    }

    void currentTabChanged(int newCurrentTabIndex, const String& newCurrentTabName) override
    {
        if (monitorPageRef != nullptr)
            monitorPageRef->setTabActive(newCurrentTabIndex == PTMonitor);

        updateVoiceEditTabAccessUi();

        if (newCurrentTabIndex == PTVoices || newCurrentTabIndex == PTEffects)
        {
            if (!ensureVoiceEditTabHasContext(newCurrentTabIndex))
            {
                const int fallbackTab = (lastNonExitTabIndex == PTVoices || lastNonExitTabIndex == PTEffects)
                    ? PTUpper
                    : lastNonExitTabIndex;
                setCurrentTabIndex(fallbackTab, false);
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon,
                    "Voice Selection Required",
                    "Select a voice on Upper, Lower, or Bass&Drums first. "
                    "Then Sounds and Effects tabs become available.");
                return;
            }
        }

        if (newCurrentTabName == "Exit")
        {
            const auto previousTab = lastNonExitTabIndex;
            setCurrentTabIndex(previousTab, false);

            auto closeApplication = [safeTop = juce::Component::SafePointer<juce::Component>(getTopLevelComponent())]()
            {
                if (safeTop != nullptr)
                    {
                    juce::MessageManager::callAsync([safeTop]()
                    {
                            if (safeTop != nullptr)
                                safeTop->userTriedToCloseWindow();
                    });
                }
                else if (auto* app = juce::JUCEApplication::getInstance())
                {
                    juce::MessageManager::callAsync([app]()
                        {
                            app->systemRequestedQuit();
                        });
                }
            };

            if (hasPendingExitSavePrompt())
            {
                auto safeThis = juce::Component::SafePointer<MenuTabs>(this);
                AlertWindow::showYesNoCancelBox(
                    AlertWindow::WarningIcon,
                    "Unsaved Panel Changes",
                    "Sounds/Effects/Preset Set actions were used since the last panel save.\n\n"
                    "Do you want to save before exiting?",
                    "Save and Exit",
                    "Exit Without Saving",
                    "Cancel",
                    this,
                    ModalCallbackFunction::create([safeThis, closeApplication](int result)
                        {
                            if (safeThis == nullptr)
                                return;

                            if (result == 1)
                            {
                                auto& appState = getAppState();
                                auto saveAndExit = [safeThis, closeApplication](bool allowMismatch)
                                    {
                                        if (safeThis == nullptr)
                                            return;

                                        auto* panel = InstrumentPanel::getInstance();
                                        bool saved = false;
                                        if (panel != nullptr)
                                            saved = panel->saveInstrumentPanel(getAppState().panelfullpathname, allowMismatch);

                                        if (!saved)
                                        {
                                            AlertWindow::showMessageBoxAsync(
                                                AlertWindow::WarningIcon,
                                                "Save Failed",
                                                "Panel save failed. Please save manually before exiting.");
                                            return;
                                        }

                                        clearPendingExitSavePrompt();
                                        closeApplication();
                                    };

                                if (appState.configPanelPairingMismatchAcknowledged)
                                {
                                    AlertWindow::showOkCancelBox(
                                        AlertWindow::WarningIcon,
                                        "Config / panel mismatch",
                                        getPanelSavePairingMismatchMessage(),
                                        "Save anyway",
                                        "Cancel",
                                        safeThis.getComponent(),
                                        ModalCallbackFunction::create([safeThis, saveAndExit](int r2)
                                            {
                                                if (safeThis == nullptr || r2 != 1)
                                                    return;
                                                saveAndExit(true);
                                            }));
                                }
                                else
                                {
                                    saveAndExit(false);
                                }
                            }
                            else if (result == 2)
                            {
                                clearPendingExitSavePrompt();
                                closeApplication();
                            }
                        }));
            }
            else
            {
                closeApplication();
            }
            return;
        }

        lastNonExitTabIndex = newCurrentTabIndex;
        TabbedComponent::currentTabChanged(newCurrentTabIndex, newCurrentTabName);

        // Each keyboard tab is a separate KeyboardPanelPage with its own rotary widgets; refresh from
        // AppState when switching Upper/Lower so Fast/Slow/Brake match the stored manual state.
        if (newCurrentTabIndex == PTUpper || newCurrentTabIndex == PTLower || newCurrentTabIndex == PTBass)
        {
            if (auto* k = dynamic_cast<KeyboardPanelPage*>(getTabContentComponent(newCurrentTabIndex)))
                k->applyManualRotaryFromAppState(false);
        }
    }

    // Small star button that is put inside one of the tabs
    // Use this technique to create things like "close tab" buttons, etc.
    class CustomTabButton final : public Component
    {
    public:
        CustomTabButton (bool isRunningComponenTransforms)
            : runningComponenTransforms (isRunningComponenTransforms)
        {
            setSize (30, 20);
        }

        void paint (Graphics& g) override
        {
            Path star;
            star.addStar ({}, 7, 1.0f, 2.0f);

            g.setColour (Colours::green);
            g.fillPath (star, star.getTransformToScaleToFit (getLocalBounds().reduced (2).toFloat(), true));
        }

        void mouseDown (const MouseEvent&) override
        {
        }

    private:
        bool runningComponenTransforms;

    };

private:
    bool hasValidLastSelectedVoiceButton() const
    {
        if (!getAppState().hasExplicitVoiceSelection)
            return false;

        const int panelBtnIdx = getAppState().lastSelectedPanelButtonIdx;
        if (panelBtnIdx < 0 || panelBtnIdx >= numbervoicebuttons)
            return false;

        const int panelGroup = lookupPanelGroup(panelBtnIdx);
        return panelGroup >= 0 && panelGroup < numberbuttongroups;
    }

    bool tryPopulateVoiceEditContextFromLastSelected(int targetTab)
    {
        if (!hasValidLastSelectedVoiceButton())
            return false;

        auto* ip = InstrumentPanel::getInstance();
        auto* im = InstrumentModules::getInstance();
        if (ip == nullptr || im == nullptr)
            return false;

        const int panelBtnIdx = getAppState().lastSelectedPanelButtonIdx;
        const int panelGroup = lookupPanelGroup(panelBtnIdx);
        if (panelGroup < 0 || panelGroup >= numberbuttongroups)
            return false;

        ButtonGroup* bg = ip->getButtonGroup(panelGroup);
        if (bg == nullptr)
            return false;

        const int keyboardTab = (bg->midikeyboard == KBDLOWER) ? PTLower
            : (bg->midikeyboard == KBDBASS) ? PTBass
            : PTUpper;

        const juce::String panelGroupTitle = bg->groupname
            + "   [In:" + std::to_string(bg->midiin)
            + " | Out:" + std::to_string(bg->midiout) + "]";

        if (targetTab == PTVoices && voicesPageRef != nullptr)
        {
            const juce::String soundFile = im->getFileName(bg->moduleidx);
            return voicesPageRef->setPanelButton(panelBtnIdx, panelGroupTitle, soundFile, bg->midiout, keyboardTab);
        }
        if (targetTab == PTEffects && effectsPageRef != nullptr)
            return effectsPageRef->setPanelButton(panelBtnIdx, panelGroupTitle, bg->midiout, keyboardTab);

        return false;
    }

    bool ensureVoiceEditTabHasContext(int targetTab)
    {
        const bool hasContext = (targetTab == PTVoices)
            ? (voicesPageRef != nullptr && voicesPageRef->hasPanelContext())
            : (effectsPageRef != nullptr && effectsPageRef->hasPanelContext());

        if (hasContext)
            return true;

        return tryPopulateVoiceEditContextFromLastSelected(targetTab);
    }

    void updateVoiceEditTabAccessUi()
    {
        const bool allowVoiceEditTabs = KeyboardPanelPage::anyVoiceEditShortcutsEnabledInTabs(*this)
            || hasValidLastSelectedVoiceButton();
        const juce::String blockedHint =
            "Select a voice on Upper, Lower, or Bass&Drums first.";

        if (auto* soundsTab = getTabbedButtonBar().getTabButton(PTVoices))
        {
            soundsTab->setEnabled(allowVoiceEditTabs);
            soundsTab->setTooltip(allowVoiceEditTabs ? juce::String() : blockedHint);
        }

        if (auto* effectsTab = getTabbedButtonBar().getTabButton(PTEffects))
        {
            effectsTab->setEnabled(allowVoiceEditTabs);
            effectsTab->setTooltip(allowVoiceEditTabs ? juce::String() : blockedHint);
        }
    }

    ComponentDragger windowDragger;
    bool draggingFromTabBarBackground = false;
    int lastNonExitTabIndex = PTStart;
    VoicesPage* voicesPageRef = nullptr;
    EffectsPage* effectsPageRef = nullptr;
    MonitorPage* monitorPageRef = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenuTabs)
};


//==============================================================================
// Struct: AMidiControl - Make Menu Tabs run
//==============================================================================
class AMidiControl final : public Component
{
public:
    AMidiControl(bool isRunningComponenTransforms = false)
        : commandManager(),
          keyTarget(),
          shortcutKeyListener(commandManager.getKeyMappings()),
          tabs(isRunningComponenTransforms, commandManager, keyTarget)
    {
        {
            juce::String hotkeyLoadErr;
            HotkeyBindings loaded = HotkeyBindings::withDefaults();

            if (getHotkeysFile().existsAsFile())
            {
                if (!loadHotkeyBindingsFromFile(loaded, hotkeyLoadErr))
                    juce::Logger::writeToLog("Hotkeys load failed: " + hotkeyLoadErr);
            }

            keyTarget.setHotkeyBindings(loaded);
        }

        commandManager.registerAllCommandsForTarget(&keyTarget);
        // Required: otherwise getTargetForCommand() never resolves to KeyPressTarget (it is not in the component tree).
        commandManager.setFirstCommandTarget(&keyTarget);

        keyTarget.onTabUpper = [this] { tabs.setCurrentTabIndex(PTUpper, true); };
        keyTarget.onTabLower = [this] { tabs.setCurrentTabIndex(PTLower, true); };
        keyTarget.onTabBass = [this] { tabs.setCurrentTabIndex(PTBass, true); };
        keyTarget.onTabSounds = [this] { tabs.setCurrentTabIndex(PTVoices, true); };
        keyTarget.onTabEffects = [this] { tabs.setCurrentTabIndex(PTEffects, true); };
        keyTarget.onTabMonitor = [this] { tabs.setCurrentTabIndex(PTMonitor, true); };
        keyTarget.onVoiceEditTabHotkeysAllowed = [this] { return tabs.canOpenVoiceEditTabs(); };
        keyTarget.onPresetRecall = [this](int idx) { tabs.recallPresetFromHotkey(idx); };
        keyTarget.onPresetNext = [this] { tabs.triggerNextPresetHotkey(); };
        keyTarget.onUpperRotaryFastSlow = [this] { tabs.triggerUpperRotaryFastSlowHotkey(); };
        keyTarget.onUpperRotaryBrake = [this] { tabs.triggerUpperRotaryBrakeHotkey(); };
        keyTarget.onLowerRotaryFastSlow = [this] { tabs.triggerLowerRotaryFastSlowHotkey(); };
        keyTarget.onLowerRotaryBrake = [this] { tabs.triggerLowerRotaryBrakeHotkey(); };

        setOpaque (true);
        setWantsKeyboardFocus (true);
        setMouseClickGrabsKeyboardFocus (true);
        addAndMakeVisible (tabs);
        addKeyListener (&shortcutKeyListener);
        tabs.addKeyListener (&shortcutKeyListener);

        setSize (1480, 320);
    }

    ~AMidiControl() override
    {
        tabs.removeKeyListener (&shortcutKeyListener);
        removeKeyListener (&shortcutKeyListener);

        if (topLevelWithAttachedKeyListener != nullptr)
            topLevelWithAttachedKeyListener->removeKeyListener (&shortcutKeyListener);
    }

    void parentHierarchyChanged() override
    {
        Component::parentHierarchyChanged();
        auto* top = getTopLevelComponent();
        if (top != nullptr && top != topLevelWithAttachedKeyListener)
        {
            if (topLevelWithAttachedKeyListener != nullptr)
                topLevelWithAttachedKeyListener->removeKeyListener (&shortcutKeyListener);

            top->addKeyListener (&shortcutKeyListener);
            topLevelWithAttachedKeyListener = top;
            keyListenerAttached = true;
        }

        if (!hasKeyboardFocus (true))
            grabKeyboardFocus();
    }

    bool keyPressed (const KeyPress& key) override
    {
        return shortcutKeyListener.keyPressed (key, this);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::lightgrey);
    }

    void resized() override
    {
        tabs.setBounds (getLocalBounds().reduced (3));
    }

private:
    // Keyboard Command Manager (must precede MenuTabs — tabs reference commandManager / keyTarget)
    ApplicationCommandManager commandManager;
    KeyPressTarget keyTarget;
    ShortcutRoutingKeyListener shortcutKeyListener;
    bool keyListenerAttached = false;
    Component* topLevelWithAttachedKeyListener = nullptr;

public:
    MenuTabs tabs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AMidiControl)
};

//==============================================================================
void BubbleMessage(Component& targetComponent, const String& textToShow,
    std::unique_ptr<BubbleMessageComponent>& bmc)
{
    bmc.reset(new BubbleMessageComponent());

    // Attach to the most reliable host component so the bubble is always visible.
    if (auto* host = targetComponent.findParentComponentOfClass<AMidiControl>())
        host->addAndMakeVisible(bmc.get());
    else if (auto* topLevel = targetComponent.getTopLevelComponent())
        topLevel->addAndMakeVisible(bmc.get());
    else
        targetComponent.addAndMakeVisible(bmc.get());

    bmc->toFront(false);

    AttributedString text(textToShow);
    text.setJustification(Justification::centred);
    text.setColour(targetComponent.findColour(TextButton::textColourOffId));

    bmc->showAt(&targetComponent, text, 4000, true, false);
}



