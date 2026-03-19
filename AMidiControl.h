/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2022 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             WidgetsDemo
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Showcases various widgets.

 dependencies:     juce_core, juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2022, linux_make, androidstudio, xcode_iphone

 moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:             Component
 mainClass:        WidgetsDemo

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

#ifndef PIP_DEMO_UTILITIES_INCLUDED
#include "DemoUtilities.h"
#endif

#include "AMidiUtils.h"
#include "AMidiDevices.h"
#include "AMidiInstruments.h"
#include "AMidiButtons.h"
#include "AMidiRotors.h"

#include<iostream>
#include<fstream>

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
            saveInstrumentPanel(instrumentDirectoryName, panelFileName);
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
                // Compile Panel File name from components and remember as full path for future saves
                inputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    .getChildFile(organdir)
                    .getChildFile(instrumentDirectoryName)
                    .getChildFile(panelFileName);

                appState.panelfullpathname = inputFile.getFullPathName();
            }
            else {
                inputFile = File(panelFileName);
            }

            if (!inputFile.existsAsFile())
            {
                juce::Logger::writeToLog("loadInstrumentPanel(): Panel file " + panelFileName + " does not exists! We will revert to the Master Panel File");

                // To do: Add some other error handling
                ///return false;
                inputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    .getChildFile(organdir)
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
            }
        }

        setPanelUpdated(true);

        return true;
    }

    //-------------------------------------------------------------------------
    // https://forum.juce.com/t/example-for-creating-a-file-and-doing-something-with-it/31998
    // Save current Instrument Panel to Disk
    //-------------------------------------------------------------------------
    bool saveInstrumentPanel(String instrumentDirectoryName, const String& panelFileName) {

        juce::Logger::writeToLog("*** saveInstrumentPanel(): Saving Instrument Panel " + panelFileName);

        {   // Scoping
            // Prepare to save Instrument Panel to Disk
            File outputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                .getChildFile(organdir)
                .getChildFile(instrumentDirectoryName)
                .getChildFile(panelFileName);

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

        return true;
    }

    // Save Panel with Full Path Name
    bool saveInstrumentPanel(const String& panelfullpathfname) {

        juce::Logger::writeToLog("*** saveInstrumentPanel(): Saving Instrument Panel " + panelfullpathfname);

        {   // Scoping
            // Prepare to save Instrument Panel to Disk
            File outputFile = File(panelfullpathfname);

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
        group = addToList(new GroupComponent("group", "Sounds for Voice Button"));
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
        lblsvendornametxt.setBounds(margin + 220, margin + 200, 160, 50);

        tbroute = addToList(new TextButton("To Upper"));
        tbroute->setColour(TextButton::textColourOffId, Colours::black);
        tbroute->setColour(TextButton::textColourOnId, Colours::black);
        tbroute->setColour(TextButton::buttonColourId, Colours::lightgrey);
        tbroute->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);

        tbroute->setClickingTogglesState(false);
        tbroute->setBounds(margin, 4 * margin + 100, 80, 30);
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
            VoiceButton* ptrvoicebutton = instrumentpanel->getVoiceButton(panelbuttonidx);
            ptrvoicebutton->setInstrument(instrument);

            // Tab redraw: https://forum.juce.com/t/how-to-capture-tabbedcomponents-tab-active/29203/9
            ptrvoicebutton->setButtonText(instrument.getVoice());
            ptrvoicebutton->triggerClick();
        };
        // Disable the button until Voice or Effects button selected in Keyboard Panels
        tbroute->setEnabled(false);

        tbcancel = addToList(new TextButton("Cancel"));
        tbcancel->setColour(TextButton::textColourOffId, Colours::black);
        tbcancel->setColour(TextButton::textColourOnId, Colours::black);
        tbcancel->setColour(TextButton::buttonColourId, Colours::lightgrey);
        tbcancel->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);

        tbcancel->setClickingTogglesState(false);
        tbcancel->setBounds(margin + 100, 4 * margin + 100, 80, 30);
        tbcancel->onClick = [=, &tabs]()
            {
                // Populate the Effects Page with last Button Instrument pressed and switch to tab
                tabs.setCurrentTabIndex(currenttabidx, true);
                //String sname = tabs.getCurrentTabName();

                // Tab redraw: https://forum.juce.com/t/how-to-capture-tabbedcomponents-tab-active/29203/9
                VoiceButton* ptrvoicebutton = instrumentpanel->getVoiceButton(panelbuttonidx);
                ptrvoicebutton->triggerClick();
            };

        addAndMakeVisible(nestedMenusButton);
        nestedMenusButton.setColour(TextButton::textColourOffId, Colours::black);
        nestedMenusButton.setColour(TextButton::textColourOnId, Colours::black);
        nestedMenusButton.setColour(TextButton::buttonColourId, Colours::lightgrey);
        nestedMenusButton.setColour(TextButton::buttonOnColourId, Colours::antiquewhite);

        // Select Voices Menu and Sub-Menu
        nestedMenusButton.setBounds(margin + 220, 30, 150, 30);
        ////nestedMenusButton.onClick = [=, &selvoice, &selvoicebank, &midiInstruments]()
        nestedMenusButton.onClick = [=]()
        {
            try 
            {
                tbroute->setEnabled(false);

                PopupMenu menu;
                int ccount = (int)midiInstruments->getCategoryCount();
                for (int i = 0; i < ccount; ++i)
                {
                    int inscount = (int)midiInstruments->getCategoryVoiceCount(i);

                    PopupMenu subMenu;
                    for (int j = 1; j < inscount; ++j)
                    {
                        //subMenu.addItem("Sub-item " + String(j), nullptr);
                        String svoice = midiInstruments->getVoice(i, j);
                        subMenu.addItem(i * 1000 + j, svoice);
                    }
                    //menu.addSubMenu("Item " + String(i), subMenu);
                    menu.addSubMenu(midiInstruments->getCategory(i), subMenu);
                }
                //menu.showMenuAsync(PopupMenu::Options{}.withTargetComponent(nestedMenusButton), handleMenuClick);
                menu.showMenuAsync(PopupMenu::Options{}.withTargetComponent(nestedMenusButton),
                    ////[&selvoice, &selvoicebank](int result) { 
                    [=](int result) {
                        // If incomplete selection, abort
                        if (result == 0) return;

                        selvoice = result % 1000;
                        selvoicebank = (int)(result / 1000);

                        // Check if we still waiting for async voice select
                        if ((selvoicebank < 0) || (selvoice < 0)) return;

                        // Update the displayed voice name to newly selected
                        lblsvoicetxt.setText(midiInstruments->getVoice(selvoicebank, selvoice), {});

                        // Apply the sound font Program Change to enable testing sound with keyboard
                        int controllerNumber = CCMSB; // MSB
                        juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
                            buttongroupmidiout,
                            controllerNumber,
                            midiInstruments->getMSB(selvoicebank, selvoice)
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        // Send the MIDI CC message to MIDI output(s)
                        //mididevices->sendMessageNow(ccMessage);
                        mididevices->sendToOutputs(ccMessage);

                        controllerNumber = CCLSB;    // LSB
                        ccMessage = juce::MidiMessage::controllerEvent(
                            buttongroupmidiout,
                            controllerNumber,
                            midiInstruments->getLSB(selvoicebank, selvoice)
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        // Send PC
                        ccMessage = juce::MidiMessage::programChange(
                            buttongroupmidiout,
                            midiInstruments->getFont(selvoicebank, selvoice)
                        );
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        if ((currenttabidx == PTUpper) || (currenttabidx == PTLower) || (currenttabidx == PTBass))
                            tbroute->setEnabled(true);
                    });
            }
            catch(...) {
                DBG("=== VoicesPage(): Aborted Voices select");
            }
        };
        // Disable the button until Voice or Effects button selected in Keyboard Panels
        nestedMenusButton.setEnabled(false);
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

        // Trigger button to display first level menu
        nestedMenusButton.setEnabled(true);
        ////nestedMenusButton.triggerClick();

        return true;
    }

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

    juce::Label lblskeyboard{ "Keyboard" };
    juce::Label lblskeyboardtxt{ "No Keyboard" };
    juce::Label lblsgroup{ "Sound Group" };
    juce::Label lblsgrouptxt{ "No Group" };
    juce::Label lblsvoice{ "Button Voice" };
    juce::Label lblsvoicetxt{ "No Voice" };
    juce::Label lblsvendornametxt{ "Sound File" };

    TextButton  nestedMenusButton{ "Voices Menu" };

    void resized() override {}

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

        return true;
    }

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
        statusLabel->setBounds(1250, 185, 200, 20);

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
            int buttongroupmidiin = ptrbuttongroup->midiin;
            int buttongroupmidiout = ptrbuttongroup->midiout;
            String buttongrouptitle = buttongroupname 
                + " [In:" + std::to_string(buttongroupmidiin) 
                + " | Out:" + std::to_string(buttongroupmidiout) 
                + "]";

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
                        int buttongroupismuted = ptrbuttongroup->muted;

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
                    sendCombinedGroupVolume(selectedButtonGroup, buttongroupmidiout, effectiveCc7);
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

                        auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, 7, 0);
                        // To do: Which one to use
                        //ccMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime);
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        g1svol->setEnabled(false);
                        setArrowButtonsMutedState(g1bvup, g1bvdwn, true);

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
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
                        sendCombinedGroupVolume(ptrbuttongroup, buttongroupmidiout, effectiveCc7);
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
            int buttongroupmidiin = ptrbuttongroup->midiin;
            int buttongroupmidiout = ptrbuttongroup->midiout;
            String buttongrouptitle = buttongroupname 
                + " [In:" + std::to_string(buttongroupmidiin) 
                + " | Out:" + std::to_string(buttongroupmidiout) 
                + "]";

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
                        int buttongroupismuted = ptrbuttongroup->muted;

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
                    sendCombinedGroupVolume(selectedButtonGroup, buttongroupmidiout, effectiveCc7);
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

                        auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, 7, 0);
                        // To do: Which one to use
                        //ccMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime);
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        g2svol->setEnabled(false);
                        setArrowButtonsMutedState(g2bvup, g2bvdwn, true);

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
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
                        sendCombinedGroupVolume(ptrbuttongroup, buttongroupmidiout, effectiveCc7);
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
            int buttongroupmidiin = ptrbuttongroup->midiin;
            int buttongroupmidiout = ptrbuttongroup->midiout;
            String buttongrouptitle = buttongroupname 
                + " [In:" + std::to_string(buttongroupmidiin) 
                + " | Out:" + std::to_string(buttongroupmidiout) 
                + "]";

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
                        int buttongroupismuted = ptrbuttongroup->muted;

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
                    sendCombinedGroupVolume(selectedButtonGroup, buttongroupmidiout, effectiveCc7);
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

                        auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, 7, 0);
                        // To do: Which one to use
                        //ccMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime);
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        g3svol->setEnabled(false);
                        setArrowButtonsMutedState(g3bvup, g3bvdwn, true);

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
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
                        sendCombinedGroupVolume(ptrbuttongroup, buttongroupmidiout, effectiveCc7);
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
            int buttongroupmidiin = ptrbuttongroup->midiin;
            int buttongroupmidiout = ptrbuttongroup->midiout;
            String buttongroupsplitname = ptrbuttongroup->splitoutname;
            String buttongrouptitle;
            if (tabidx == PTBass) {
                buttongrouptitle = buttongroupname
                    + " [In:" + std::to_string(buttongroupmidiin)
                    + " | Out:" + std::to_string(buttongroupmidiout)
                    + "]";
            }
            else {
                buttongrouptitle = buttongroupname
                    + " [In:" + std::to_string(buttongroupmidiin)
                    + " | Out:" + std::to_string(buttongroupmidiout)
                    + " | Spl:" + buttongroupsplitname
                    + "]";
            }

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
                        int buttongroupismuted = selectedButtonGroup->muted;

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
                    sendCombinedGroupVolume(selectedButtonGroup, buttongroupmidiout, effectiveCc7);
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

                        auto ccMessage = juce::MidiMessage::controllerEvent(buttongroupmidiout, 7, 0);
                        // To do: Which one to use
                        //ccMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime);
                        ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
                        mididevices->sendToOutputs(ccMessage);

                        g4svol->setEnabled(false);
                        setArrowButtonsMutedState(g4bvup, g4bvdwn, true);

                        ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(buttongroupidx);
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
                        sendCombinedGroupVolume(ptrbuttongroup, buttongroupmidiout, effectiveCc7);
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
                    if (cbSave != nullptr)
                        cbSave->setEnabled(true);
                    updatePanelSaveButtonsPendingStyle();

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
                    if (cbSave != nullptr)
                        cbSave->setEnabled(true);
                    updatePanelSaveButtonsPendingStyle();

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

            int cbuttons = 8;
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
            auto* tbsetpreset = addToList(new PresetButton("Set"));
            tbsetpreset->setClickingTogglesState(true);
            tbsetpreset->setColour(TextButton::textColourOffId, Colours::white);
            tbsetpreset->setColour(TextButton::textColourOnId, Colours::black);
            tbsetpreset->setColour(TextButton::buttonColourId, Colours::black.darker());
            tbsetpreset->setColour(TextButton::buttonOnColourId, Colours::antiquewhite);
            tbsetpreset->setBounds(g10xoffset + mgroup + col * bwidth, ygroup + mgroup * 2, bwidth, bheight);
            tbsetpreset->setConnectedEdges(Button::ConnectedOnRight);
            tbsetpreset->onClick = [=]()
                {
                    if (tbsetpreset->getToggleState()) {
                        bsetpreset = true;
                        bpendingPresetSet = true;
                        if (cbSave != nullptr)
                            cbSave->setEnabled(true);
                        updatePanelSaveButtonsPendingStyle();

                        DBG("PresetButton.onClick(): Set");
                    }
                    else {
                        bsetpreset = false;

                        DBG("PresetButton.onClick(): Unset");
                    }
                };

            // Create the Panel Buttons for Group 1
            col = 1;
            for (int i = 0; i < (cbuttons - 1); ++i)
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
                    pstb->setColour(TextButton::buttonColourId, Colours::darkred);
                    pstb->setColour(TextButton::buttonOnColourId, Colours::darkred.brighter());
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
            int buttongroupmoduleidx = ptrbuttongroup->moduleidx;
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
                };

            // Preset rotary change control value to slow
            RotaryFastSlow(gmidichan, 
                instrumentmodules->getRotorCC(buttongroupmoduleidx),        //74
                instrumentmodules->getRotorSlow(buttongroupmoduleidx)
            );
            //// Check slow/fast startup as organ rotary seems to be out of sync at startp
            //// If we keep, then is the above redundant?
            tbrotslow->onClick();

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

                        tbrotslow->setEnabled(true);     // Enable rotor fast/slow changes
                    }
                };
        }   // End - Rotary Group

        // Quick Access Keyboard Buttons
        int xaccess = 1020;
        {
            if ((tabidx == PTLower) || (tabidx == PTBass)) {
                auto* toUpperKBD = addToList(new CommandButton());
                toUpperKBD->setButtonText("Upper");
                toUpperKBD->setClickingTogglesState(false);
                toUpperKBD->setColour(TextButton::textColourOffId, Colours::black);
                toUpperKBD->setColour(TextButton::textColourOnId, Colours::white);
                toUpperKBD->setColour(TextButton::buttonColourId, Colours::white);
                toUpperKBD->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
                toUpperKBD->setBounds(mgroup + xaccess, ygroup + mgroup * 2, 80, 50);
                toUpperKBD->setToggleState(true, dontSendNotification);
                toUpperKBD->onClick = [=, &tabs]()
                    {
                        tabs.setCurrentTabIndex(PTUpper, false);
                    };
                xaccess = xaccess + 100;
            }

            if ((tabidx == PTUpper) || (tabidx == PTBass)) {
                auto* toLowerKBD = addToList(new CommandButton());
                toLowerKBD->setButtonText("Lower");
                toLowerKBD->setClickingTogglesState(false);
                toLowerKBD->setColour(TextButton::textColourOffId, Colours::black);
                toLowerKBD->setColour(TextButton::textColourOnId, Colours::white);
                toLowerKBD->setColour(TextButton::buttonColourId, Colours::white);
                toLowerKBD->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
                toLowerKBD->setBounds(mgroup + xaccess, ygroup + mgroup * 2, 80, 50);
                toLowerKBD->setToggleState(true, dontSendNotification);
                toLowerKBD->onClick = [=, &tabs]()
                    {
                        tabs.setCurrentTabIndex(PTLower, false);
                    };
                xaccess = xaccess + 100;
            }

            if ((tabidx == PTUpper) || (tabidx == PTLower)) {
                auto* toBassKBD = addToList(new CommandButton());
                toBassKBD->setButtonText("Bass");
                toBassKBD->setClickingTogglesState(false);
                toBassKBD->setColour(TextButton::textColourOffId, Colours::black);
                toBassKBD->setColour(TextButton::textColourOnId, Colours::white);
                toBassKBD->setColour(TextButton::buttonColourId, Colours::white);
                toBassKBD->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
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
            auto* tbExit = addToList(new CommandButton());
            tbExit->setButtonText("Exit");
            tbExit->setColour(TextButton::textColourOffId, Colours::white);
            tbExit->setColour(TextButton::textColourOnId, Colours::white);
            tbExit->setColour(TextButton::buttonColourId, Colours::black.darker());
            tbExit->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            tbExit->setBounds(1350, 235, 80, 30);
            tbExit->setToggleState(false, dontSendNotification);
            tbExit->onClick = [=]()
                {
                    if (auto* topLevel = getTopLevelComponent())
                    {
                        auto safeTop = juce::Component::SafePointer<juce::Component>(topLevel);
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

            auto* tbSave = addToList(new CommandButton());
            cbSave = tbSave;
            tbSave->setButtonText("Save");
            tbSave->setColour(TextButton::textColourOffId, Colours::white);
            tbSave->setColour(TextButton::textColourOnId, Colours::white);
            tbSave->setColour(TextButton::buttonColourId, Colours::black.darker());
            tbSave->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            tbSave->setBounds(mgroup + 1240, 235, 80, 30);
            tbSave->setToggleState(false, dontSendNotification);
            tbSave->onClick = [=]()
                {
                    DBG("*** KeyboardManualPage(): Saving Panel");

                    //bool bsaved = instrumentpanel->saveInstrumentPanel(appState.instrumentdname, appState.panelfname);
                    bool bsaved = instrumentpanel->saveInstrumentPanel(appState.panelfullpathname);

                    String msgloaded;
                    if (!bsaved) {
                        msgloaded = "Saved failed: " + appState.panelfname;
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
                    updatePanelSaveButtonsPendingStyle();

                    tbSave->setEnabled(false);
                };
            tbSave->setEnabled(false);

            auto* tbSaveAs = addToList(new CommandButton());
            cbSaveAs = tbSaveAs;
            tbSaveAs->setButtonText("Save As");
            tbSaveAs->setColour(TextButton::textColourOffId, Colours::white);
            tbSaveAs->setColour(TextButton::textColourOnId, Colours::white);
            tbSaveAs->setColour(TextButton::buttonColourId, Colours::black.darker());
            tbSaveAs->setColour(TextButton::buttonOnColourId, Colours::black.brighter());
            tbSaveAs->setBounds(mgroup + 1340, 235, 80, 30);
            tbSaveAs->setToggleState(false, dontSendNotification);
            tbSaveAs->onClick = [=]()
                {
                    File fileToSave = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                        .getChildFile(organdir)
                        .getChildFile(appState.instrumentdname);

                    bool useNativeVersion = false;
                    //filechooser.reset(new FileChooser("Save Panel As: Select existing or create new *.pnl",
                    //    File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    //    .getChildFile(organdir)
                    //    .getChildFile(appState.instrumentdname),
                    //    "*.pnl", useNativeVersion));

                    filechooser.reset(new FileChooser("Save Panel As: Select existing or create new *.pnl",
                        File(appState.panelfullpathname),
                        "*.pnl", useNativeVersion));

                    filechooser->launchAsync(
                        FileBrowserComponent::saveMode
                            | FileBrowserComponent::canSelectFiles
                            | FileBrowserComponent::warnAboutOverwriting,
                        [=](const FileChooser& chooser)
                        {
                            String selectedfname;
                            bool bsaved = false;

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

                                bsaved = instrumentpanel->saveInstrumentPanel(appState.instrumentdname, selectedfname);

                                String msgloaded;
                                if (!bsaved) {
                                    msgloaded = "Save as failed: " + selectedfname;
                                    juce::Logger::writeToLog("*** KeyboardManualPage(): Save as failed! " + selectedfname);
                                }
                                else {
                                    msgloaded = "Saved as: " + selectedfname;
                                    juce::Logger::writeToLog("*** KeyboardManualPage(): Saved as " + selectedfname);
                                }

                                if (TextButton* focused = tbSaveAs)
                                    BubbleMessage(*focused, msgloaded, this->bubbleMessage);

                                if (bsaved)
                                    clearPendingExitSavePrompt();
                                updatePanelSaveButtonsPendingStyle();
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
        lblpanelfile->setBounds(mgroup + 1240, 205, 200, 30);

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

        cbSave->setEnabled(true);
        updatePanelSaveButtonsPendingStyle();

        // Read Rotary Status for every Button Group
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
        }

        // Activate Rotary for every Button Group
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

            cbSave->setEnabled(true);
            updatePanelSaveButtonsPendingStyle();

            return;
        }

        // Effects cheanged via effects updatd
        else if (beffectsupdated) {
            beffectsupdated = false;

            cbSave->setEnabled(true);
            updatePanelSaveButtonsPendingStyle();

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

                String buttongroupname = ptrbuttongroup->groupname;
                int buttongroupmidiin = ptrbuttongroup->midiin;
                int buttongroupmidiout = ptrbuttongroup->midiout;
                String buttongroupsplitname = ptrbuttongroup->splitoutname;

                String buttongrouptitle = buttongroupname 
                    + " [In:" + std::to_string(buttongroupmidiin) 
                    + " | Out:" + std::to_string(buttongroupmidiout) 
                    + "]";

                // Add Octave Split indicator
                if ((bgidx == 3) || (bgidx == 7)) {
                    buttongrouptitle = buttongroupname
                        + " [In:" + std::to_string(buttongroupmidiin) 
                        + " | Out:" + std::to_string(buttongroupmidiout)
                        + " | Spl:" + buttongroupsplitname 
                        + "]";
                }

                GroupComponent* ptrgroupcomponent = ptrbuttongroup->getGroupComponentPtr();
                ptrgroupcomponent->setText(buttongrouptitle);
            }

            appState.configchanged = false;
        }

        lblpanelfile->setText(appState.panelfname, {});
    }

    //-----------------------------------------------------------------------------
    ~KeyboardPanelPage()
    {
        DBG("== KeyboardManualPage(): Destructor " + std::to_string(--zinstcntmanualpage));
    }

private:
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
    MidiStartPage(TabbedComponent& tabs) : midiKeyboard(keyboardState, MidiKeyboardComponent::horizontalKeyboard),
        midiInputSelector(new MidiDeviceListBox("Midi Input Selector", *this, true)),
        midiOutputSelector(new MidiDeviceListBox("Midi Output Selector", *this, false)),
        instrumentmodules(InstrumentModules::getInstance()),
        midiInstruments(MidiInstruments::getInstance()),
        mididevices(MidiDevices::getInstance()),
        instrumentpanel(InstrumentPanel::getInstance()),
        appState(getAppState())
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
                        String selectedfname;
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
                            else
                                result.toString(false);
                        }

                        if (selectedfname == "") return;

                        juce::Logger::writeToLog("*** MidiStartPage(): Loading Config Panel: " + selectedfname);

                        // Replace with Load Config functionality
                        //bool bloaded = instrumentpanel->loadInstrumentPanel(instrumentdname, selectedfname, true);
                        //bool bloaded = instrumentpanel->loadInstrumentPanel(instrumentdname, selectedfullpathfname, true);
                        bool bloaded = true;

                        appState.configfname = selectedfname;
                        configfileLabel.setText(appState.configfname, {});

                        String msgloaded;
                        if (bloaded == true) {
                            // Successful explicit config load becomes the new active baseline.
                            appState.pnlconfigfname = appState.configfname;
                            appState.configreload = false;
                            configfileLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
                            msgloaded = "Loaded: " + appState.configfname;
                        }
                        else {
                            configfileLabel.setColour(juce::Label::textColourId, juce::Colours::red.darker());
                            msgloaded = "Load failed: " + appState.configfname;
                        }

                        if (TextButton* focused = &loadConfigButton)
                            BubbleMessage(*focused, msgloaded, this->bubbleMessage);
                    });
            };

        addAndMakeVisible(loadPanelButton);
        loadPanelButton.setButtonText("Load Panel");
        loadPanelButton.setColour(TextButton::textColourOffId, Colours::white);
        loadPanelButton.setColour(TextButton::textColourOnId, Colours::white);
        loadPanelButton.setColour(TextButton::buttonColourId, Colours::black);
        loadPanelButton.setColour(TextButton::buttonOnColourId, Colours::black);
        loadPanelButton.setToggleState(true, dontSendNotification);
        loadPanelButton.onClick = [=]()
            {
                File fileToSave = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    .getChildFile(organdir)
                    .getChildFile(appState.instrumentdname);

                bool useNativeVersion = false;
                filechooser.reset(new FileChooser("Select Button Panel file to load",
                    File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                    .getChildFile(organdir)
                    .getChildFile(appState.instrumentdname),
                    "*.pnl", useNativeVersion));

                filechooser->launchAsync(
                    FileBrowserComponent::openMode
                    | FileBrowserComponent::canSelectFiles
                    | FileBrowserComponent::filenameBoxIsReadOnly,
                    [this](const FileChooser& chooser)
                    {
                        String selectedfname;
                        String selectedfullpathfname;
                        auto results = chooser.getURLResults();

                        //for (auto result : results)
                        //    selectedfname << (result.isLocalFile() 
                        //        ? result.getLocalFile().getFileName() //.getFullPathName()
                        //        : result.toString(false));

                        for (auto result : results) {
                            if (result.isLocalFile()) {
                                selectedfname = result.getLocalFile().getFileName();
                                selectedfullpathfname = result.getLocalFile().getFullPathName();
                            }
                            else
                                result.toString(false);
                        }

                        if (selectedfname == "") return;

                        juce::Logger::writeToLog("*** MidiStartPage(): Loading Instrument Panel: " + selectedfname);

                        bool bloaded = instrumentpanel->loadInstrumentPanel(appState.instrumentdname, selectedfullpathfname, true);

                        // Save Panel File globally
                        appState.panelfname = selectedfname;
                        appState.panelfullpathname = selectedfullpathfname; 

                        String msgloaded;
                        if (bloaded == true) {
                            msgloaded = "Loaded panel:\n " + appState.panelfname;
                        }
                        else {
                            msgloaded = "Panel load failed: " + appState.panelfname;
                        }

                        if (TextButton* focused = &loadPanelButton)
                            BubbleMessage(*focused, msgloaded, this->bubbleMessage);

                        panelfileLabel.setText(appState.panelfname, {});

                        // Flag mismatch between panel file and incorrect config file in red
                        appState.configreload = (appState.configfname.compare(appState.pnlconfigfname) != 0);

                        if (appState.configreload == true)
                            configfileLabel.setColour(juce::Label::textColourId, juce::Colours::red.darker());
                        else
                            configfileLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
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
            return lblmididevices->getNumDevices(isInput);
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
                if (rowNumber < lblmididevices->getNumMidiInputs())
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
                        lblmididevices->closeDevice(isInput, lastSelectedItems[i]);
                }

                for (auto i = 0; i < newSelectedItems.size(); ++i)
                {
                    if (!lastSelectedItems.contains(newSelectedItems[i]))
                        lblmididevices->openDevice(isInput, newSelectedItems[i]);
                }

                lastSelectedItems = newSelectedItems;
            }
        }

        //-----------------------------------------------------------------------------
        void syncSelectedItemsWithDeviceList(const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices)
        {
            SparseSet<int> selectedRows;
            for (auto i = 0; i < midiDevices.size(); ++i)
                if (midiDevices[i]->inDevice != nullptr || midiDevices[i]->outDevice != nullptr)
                    selectedRows.addRange(Range<int>(i, i + 1));

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

        // Reset all Channel outputs. Note that any channel with device out 
        // of 255 will not be output 
        for (int i = 0; i < 17; i++)
            mididevices->moduleout[i] = 255;

        // Update all the Channel outs for every Button Group
        for (int i = 0; i < numberbuttongroups; i++) {

            bool bfound = false;

            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(i);
            int grpmidimod = ptrbuttongroup->getMidiModule();
            int grpmidichan = ptrbuttongroup->midiout;

            // Lookup Module short/search string and find match in the active Modules
            String modidstring = instrumentmodules->getModuleIdString(grpmidimod).toLowerCase();

            int outmod = 0;
            for (auto& dev : midiDevices) {
                String strdevicename = dev->deviceInfo.name.toLowerCase();

                bfound = strdevicename.contains(modidstring);
                if (bfound) {
                    // Update the Midi send Channel to the Midi Out Modele confgured in Button Group
                    mididevices->moduleout[grpmidichan] = outmod;

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
            DBG("MIDI Out channel " + std::to_string(i)
                + " to " + std::to_string(mididevices->moduleout[i])
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

    ////ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;
    std::unique_ptr<MidiDeviceListBox> midiInputSelector, midiOutputSelector;

    InstrumentModules* instrumentmodules = nullptr;

    MidiDevices* mididevices = nullptr;
    InstrumentPanel* instrumentpanel = nullptr;
    MidiInstruments* midiInstruments = nullptr;
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
        g.fillAll(juce::Colour(0xff141414));

        g.setColour(juce::Colours::grey.withAlpha(0.45f));
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


//==============================================================================
// Class: Config Page
//==============================================================================
static int zinstcntvoicespage = 0;
class ConfigPage final : public Component
{
public:
    ConfigPage(TabbedComponent& tabs) :
        instrumentmodules(InstrumentModules::getInstance()),
        mididevices(MidiDevices::getInstance()), // Singleton if there isn't already one
        instrumentpanel(InstrumentPanel::getInstance()), // Singleton if there isn't already one
        appState(getAppState())
    {
        juce::Logger::writeToLog("== ConfigPage(): Constructor " + std::to_string(zinstcntvoicespage++));

        // 1. Load the system Confg from disk - mostly Button Group IO mappings 
        loadConfigs(appState.configfname);
        lblconfigfile.setText(appState.configfname, {});

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

                        saveButton.setEnabled(true);
                    });
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

            saveButton.setEnabled(true);
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

                saveButton.setEnabled(false);
                return;
            }

            instrumentpanel->getButtonGroup(i)->midiin = txtMidiIn.getText().getIntValue();

            saveButton.setEnabled(true);
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

                saveButton.setEnabled(false);
                return;
            }
            instrumentpanel->getButtonGroup(i)->midiout = txtMidiOut.getText().getIntValue();

            saveButton.setEnabled(true);
            };

        // Keyboard Split
        addAndMakeVisible(lblSplit);
        lblSplit.setBounds(400, 165, 200, 24);
        lblSplit.setText("Solo Out Split", {});

        //https://studiocode.dev/resources/midi-middle-c/
        addAndMakeVisible(txtSplit);
        txtSplit.setEnabled(false);
        txtSplit.setBounds(560, 165, 100, 24);
        txtSplit.setText("--");
        txtSplit.onFocusLost = [=]() {
            int i = comboConfig.getSelectedId() - 1;   // Combobox is 1 based

            String sval = txtSplit.getText();
            if ((sval.length() != 2) && (sval.length() != 3)) {
                txtSplit.setText("--");

                saveButton.setEnabled(false);
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

            saveButton.setEnabled(true);
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

                saveButton.setEnabled(false);
                return;
            }
            instrumentpanel->getButtonGroup(i)->octxpose = txtOctave.getText().getIntValue();

            saveButton.setEnabled(true);
            };

        // Keyboard Velocity on or off
        addAndMakeVisible(lblVelocity);
        lblVelocity.setBounds(400, 195, 200, 24);
        lblVelocity.setText("Velocity Out", {});

        addAndMakeVisible(toggleVelocity);
        toggleVelocity.setBounds(560, 195, 200, 24);
        toggleVelocity.onClick = [=]() {
            velocitystate = toggleVelocity.getToggleState();

            int i = comboConfig.getSelectedId() - 1;   // Combobox is 1 based
            instrumentpanel->getButtonGroup(i)->velocity = velocitystate;

            saveButton.setEnabled(true);

            juce::String stateString = velocitystate ? "ON" : "OFF";
            juce::Logger::outputDebugString("Velocity changed to " + stateString);
        };

        addAndMakeVisible(lblDefaultEffectsVol);
        lblDefaultEffectsVol.setBounds(1150, 50, 160, 24);
        lblDefaultEffectsVol.setText("Default Effects Vol", {});

        addAndMakeVisible(txtDefaultEffectsVol);
        txtDefaultEffectsVol.setBounds(1280, 50, 60, 24);
        txtDefaultEffectsVol.setText(std::to_string(appState.defaultEffectsVol));
        txtDefaultEffectsVol.onFocusLost = [=]() {
            const int val = txtDefaultEffectsVol.getText().getIntValue();
            if (val < 1 || val > 127) {
                txtDefaultEffectsVol.setText(std::to_string(appState.defaultEffectsVol), false);
                saveButton.setEnabled(false);
                return;
            }

            appState.defaultEffectsVol = val;
            saveButton.setEnabled(true);
        };

        addAndMakeVisible(loadConfigButton);
        loadConfigButton.setButtonText("Load");
        loadConfigButton.setColour(TextButton::textColourOffId, Colours::white);
        loadConfigButton.setColour(TextButton::textColourOnId, Colours::white);
        loadConfigButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        loadConfigButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        loadConfigButton.setBounds(20, 225, 80, 30);
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

                        juce::Logger::writeToLog("*** MidiStartPage(): Loading Config Panel: " + selectedfname);

                        // Replace with Load Config functionality
                        bool bloaded = loadConfigs(selectedfname);

                        appState.configfname = selectedfname;

                        lblconfigfile.setText(appState.configfname, {});

                        appState.configchanged = true;

                        // Select first itemin dropdown on creation
                        comboConfig.setSelectedId(1);

                        String msgloaded;
                        if (bloaded == true) {
                            msgloaded = "Loading:\n " + appState.configfname;
                        }
                        else {
                            msgloaded = "Load failed:\n " + appState.configfname;
                        }

                        if (TextButton* focused = &loadConfigButton)
                            BubbleMessage(*focused, msgloaded, this->bubbleMessage);
                    });
            };

        // Save Config changes, and enable VoiceButton ComponentGroup Title Updates
        addAndMakeVisible(saveButton);
        saveButton.setButtonText("Save");
        saveButton.setColour(TextButton::textColourOffId, Colours::white);
        saveButton.setColour(TextButton::textColourOnId, Colours::white);
        saveButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        saveButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        saveButton.setBounds(20, 225, 80, 30);
        saveButton.setBounds(120, 225, 80, 30);
        saveButton.setEnabled(false);
        saveButton.onClick = [=]() {
            bool bsaved = saveConfigs(appState.configfname);

            appState.configchanged = true;

            String msgloaded;
            if (!bsaved) {
                msgloaded = "Config save failed!";
                juce::Logger::writeToLog("*** ConfigPage(): Config save failed! ");
            }
            else {
                msgloaded = "Config saved";
                juce::Logger::writeToLog("*** ConfigPage(): Config saved!");
            }

            if (TextButton* focused = &saveButton)
                BubbleMessage(*focused, msgloaded, this->bubbleMessage);

            saveButton.setEnabled(false);
            };

        addAndMakeVisible(saveAsButton);
        saveAsButton.setButtonText("Save As");
        saveAsButton.setColour(TextButton::textColourOffId, Colours::white);
        saveAsButton.setColour(TextButton::textColourOnId, Colours::white);
        saveAsButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        saveAsButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        saveAsButton.setBounds(220, 225, 80, 30);
        saveAsButton.setToggleState(false, dontSendNotification);
        saveAsButton.onClick = [=]() {
            File fileToSave = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                .getChildFile(organdir)
                .getChildFile(appState.configdir);

            bool useNativeVersion = false;
            filechooser.reset(new FileChooser("Save Config As: Select existing or create new *.cfg",
                File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                .getChildFile(organdir)
                .getChildFile(appState.configdir),
                "*.cfg", useNativeVersion));

            filechooser->launchAsync(
                FileBrowserComponent::saveMode
                | FileBrowserComponent::canSelectFiles
                | FileBrowserComponent::warnAboutOverwriting,
                [=](const FileChooser& chooser)
                {
                    //String selectedfname;
                    bool bsaved = false;

                    auto results = chooser.getURLResults();

                    for (auto result : results)
                        selconfigfname << (result.isLocalFile() ? result.getLocalFile().getFileName()
                            : result.toString(false));

                    if (selconfigfname.length() != 0) {
                        int extpos = selconfigfname.indexOf(0, ".");
                        if (extpos == -1) {
                            DBG("*** Save Config: Extension not defined as .cfg. Adding extension");

                            selconfigfname = selconfigfname + ".cfg";
                        }

                        // Update static/global config file name and save to this file
                        appState.configfname = selconfigfname;
                        bsaved = saveConfigs(appState.configfname);

                        appState.configchanged = true;

                        String msgloaded;
                        if (!bsaved) {
                            msgloaded = "Save as failed: " + appState.configfname;
                            juce::Logger::writeToLog("*** ConfigPage(): Save As failed! " + appState.configfname);
                        }
                        else {
                            msgloaded = "Saved as: " + appState.configfname;
                            juce::Logger::writeToLog("*** ConfigPage(): Saved As " + appState.configfname);
                        }

                        if (TextButton* focused = &saveAsButton)
                            BubbleMessage(*focused, msgloaded, this->bubbleMessage);
                    }
                    else {
                        ////statusLabel->setText("Empty file name save! " + configfname, juce::dontSendNotification);
                    }
                });

                saveAsButton.setEnabled(false);

                lblconfigfile.setText(appState.configfname, {});
            };

        addAndMakeVisible(lblconfigfile);
        lblconfigfile.setColour(juce::Label::textColourId, juce::Colours::grey);
        lblconfigfile.setJustificationType(juce::Justification::left);
        lblconfigfile.setBounds(520, 225, 120, 30);

        // Quick Access Keyboard Buttons
        int xaccess = 920;
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

        addAndMakeVisible(lblPassthrough);
        lblPassthrough.setBounds(1150, 20, 150, 24);
        lblPassthrough.setText("MIDI In Passthru", {});

        //https://docs.juce.com/master/tutorial_radio_buttons_checkboxes.html
        addAndMakeVisible(togglePassthrough);
        togglePassthrough.setBounds(1280, 20, 50, 24);
        togglePassthrough.onClick = [=]() {
            passthroughstate = togglePassthrough.getToggleState();

            saveButton.setEnabled(true);

            juce::String stateString = passthroughstate ? "ON" : "OFF";
            juce::Logger::outputDebugString("Passthrough changed to " + stateString);
            };

        addAndMakeVisible(resetButton);
        resetButton.setButtonText("MIDI Reset");
        resetButton.setColour(TextButton::textColourOffId, Colours::white);
        resetButton.setColour(TextButton::textColourOnId, Colours::white);
        resetButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        resetButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        resetButton.setBounds(1350, 20, 80, 30);

        resetButton.onClick = [=]() {

            mididevices->resetAllControllers();
            };

        addAndMakeVisible(exitButton);
        exitButton.setButtonText("Exit");
        exitButton.setColour(TextButton::textColourOffId, Colours::white);
        exitButton.setColour(TextButton::textColourOnId, Colours::white);
        exitButton.setColour(TextButton::buttonColourId, Colours::black.darker());
        exitButton.setColour(TextButton::buttonOnColourId, Colours::black.brighter());
        exitButton.setBounds(1350, 235, 80, 30);
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
        statusLabel.setBounds(1220, 210, 200, 20);
    }

    void lookAndFeelChanged() override
    {
        txtGroupName.applyFontToAllText(txtGroupName.getFont());
    }

    ~ConfigPage() override {
        DBG("=== ConfigPage(): Destructor " + std::to_string(--zinstcntvoicespage));
    }

private:
    MidiDevices* mididevices = nullptr;
    InstrumentPanel* instrumentpanel = nullptr;
    InstrumentModules* instrumentmodules = nullptr;
    AppState& appState;

    ValueTree vtconfigs;

    ComboBox comboConfig{ "ConfigCombo" };
    GroupComponent group{ "group", "Button Group Configs" };
    juce::TextButton SoundModule;
    juce::TextEditor txtGroupName, txtMidiIn, txtMidiOut, txtSplit, txtOctave, txtDefaultEffectsVol;
    juce::Label lblKeyboard, lblGroupName, lblButtonCount, lblMidiIn, lblMidiOut, lblSplit, lblOctave;
    juce::Label lblPassthrough,lblVelocity, lblDefaultEffectsVol, lblconfigfile, label11, label31;
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

        // Preset all channels to map Midi Input to Output by default to avoid midi message loss
        for (i = 0; i < 17; i++) {
            for (j = 0; j < 5; j++) {
                if (j == 0) mididevices->iomap[i][j] = i;
                else mididevices->iomap[i][j] = 0;
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

        // Configs ValueTree
        ValueTree configsTree(configsType);
        configsTree.setProperty(defaultEffectsVolType, appState.defaultEffectsVol, nullptr);

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

        // Prepare to save Instrument Configs to Disk
        File outputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(configdir)
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
            juce::Logger::writeToLog("*** saveConfigs(): Config file save failed " + configfname);
            // To do: Add more error handling
            return false;
        }
        juce::Logger::writeToLog("*** saveConfigs(): Config file saved " + configfname);

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

        if (!(vtconfigs.isValid() && (vtconfigs.getNumChildren() > 0))) {
            juce::Logger::writeToLog("*** loadVTConfigs(): An error occurred while reading temo Configs ValueTree...");

            // To do: Add more error handling
            return false;
        }

        // For testing
        //String vtxml = vtconfigs.toXmlString();

        if (vtconfigs.hasProperty(defaultEffectsVolType))
            appState.defaultEffectsVol = juce::jlimit(1, 127, (int)vtconfigs.getProperty(defaultEffectsVolType));
        else
            appState.defaultEffectsVol = 100;

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
            .getChildFile(configdir)
            .getChildFile(configsname);

        if (!inputFile.existsAsFile())
        {
            juce::Logger::writeToLog("*** loadConfigs(): No Config file " + configsname + " to load!");

            inputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                .getChildFile(organdir)
                .getChildFile("masterconfigs.cfg");
            juce::Logger::writeToLog("*** loadConfigs(): Loading file masterConfigs.cfg instead");
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

        txtDefaultEffectsVol.setText(std::to_string(appState.defaultEffectsVol), false);

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

        // Reset all Channel outputs. Note that any channel with device out 
        // of 255 will not be output 
        for (int i = 0; i < 17; i++)
            mididevices->moduleout[i] = 255;

        // Update all the Channel outs for every Button Group
        for (int i = 0; i < numberbuttongroups; i++) {

            bool bfound = false;

            ButtonGroup* ptrbuttongroup = instrumentpanel->getButtonGroup(i);
            int grpmidimod = ptrbuttongroup->getMidiModule();
            int grpmidichan = ptrbuttongroup->midiout;

            // Lookup Module short/search string and find match in the active Modules
            String modidstring = instrumentmodules->getModuleIdString(grpmidimod);

            int outmod = 0;
            for (auto& dev : midiDevices) {
                String strdevicename = dev->deviceInfo.name;

                bfound = strdevicename.contains(modidstring);
                if (bfound) {
                    // Update the Midi send Channel to the Midi Out Modele confgured in Button Group
                    mididevices->moduleout[grpmidichan] = outmod;

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
            DBG("*** ModulesToChannelMap: MIDI Out channel " + std::to_string(i)
                + " to " + std::to_string(mididevices->moduleout[i])
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
    MenuTabs ([[maybe_unused]] bool isRunningComponenTransforms)
        : TabbedComponent (TabbedButtonBar::TabsAtTop)
    {
        juce::Logger::writeToLog("== MenuTabs(): Constructor ");

        auto colour = findColour (ResizableWindow::backgroundColourId);

        ConfigPage* configspage = new ConfigPage(*this);
        EffectsPage* effectspage = new EffectsPage(*this);
        VoicesPage* voicespage = new VoicesPage(*this);

        // Create MidiStart Page
        addTab("Start",        colour, new MidiStartPage(*this), true);

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

        // Create Config and Help Pages
        addTab("Config",       colour, configspage, true);
        addTab("Help",         colour, new HelpPage(), true);
        addTab("Exit",         colour, new Component(), true);

        this->setTabBarDepth(30);

        // Set default tab
        this->setCurrentTabIndex(PTStart, true);
        String sname = this->getCurrentTabName();

        //this->grabKeyboardFocus();
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
                                auto* panel = InstrumentPanel::getInstance();
                                auto& appState = getAppState();
                                bool saved = false;
                                if (panel != nullptr)
                                    saved = panel->saveInstrumentPanel(appState.panelfullpathname);

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
    ComponentDragger windowDragger;
    bool draggingFromTabBarBackground = false;
    int lastNonExitTabIndex = PTStart;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenuTabs)
};


//==============================================================================
// Struct: AMidiControl - Make Menu Tabs run
//==============================================================================
class AMidiControl final : public Component
{
public:
    AMidiControl(bool isRunningComponenTransforms = false)
        : tabs (isRunningComponenTransforms)
    {
        // Register the commands that the target component can perform
        commandManager.registerAllCommandsForTarget(&keyTarget);

        // Then add command manager key mappings as a KeyListener to the top-level component
        // so it is notified of key presses
        getTopLevelComponent()->addKeyListener(commandManager.getKeyMappings());

        setOpaque (true);
        addAndMakeVisible (tabs);

        setSize (1480, 320);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::lightgrey);
    }

    void resized() override
    {
        tabs.setBounds (getLocalBounds().reduced (3));
    }

    MenuTabs tabs;

private:
    // Keyboard Command Manager
    //ApplicationCommandManager& commandManager = getGlobalCommandManager();
    ApplicationCommandManager commandManager;

    KeyPressTarget keyTarget;

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



