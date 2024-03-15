/*
  ==============================================================================

    AMidiDevicesh.h
    Created: 14 Feb 2024 6:57:27pm
    Author:  a_min

  ==============================================================================
*/

#pragma once


//==============================================================================
struct MidiDeviceListEntry final : ReferenceCountedObject
{
    explicit MidiDeviceListEntry(MidiDeviceInfo info) : deviceInfo(info) {}

    MidiDeviceInfo deviceInfo;
    std::unique_ptr<MidiInput> inDevice;
    std::unique_ptr<MidiOutput> outDevice;

    using Ptr = ReferenceCountedObjectPtr<MidiDeviceListEntry>;

    void stopAndReset()
    {
        if (inDevice != nullptr)
            inDevice->stop();

        inDevice.reset();
        outDevice.reset();
    }
};



//==============================================================================
// Class: MidiDevices - Singleton
// 
// Usage Example
// MySingleton* m = MySingleton::getInstance(); // creates the singleton if there isn't already one.
// ...
// MySingleton::deleteInstance(); // safely deletes the singleton (if it's been created).
//==============================================================================
static int zinstcntmididevices = 0;
class MidiDevices : public Component, public MidiInputCallback
{
public:

    ~MidiDevices()
    {
        DBG("=S= MidiDevices(): Destructor " + std::to_string(--zinstcntmididevices));

        // Ensure no dangling pointers are left when singleton is deleted.
        clearSingletonInstance();
    }

    int getNumMidiInputs() //const noexcept
    {
        return midiInputs.size();
    }

    int getNumMidiOutputs() //const noexcept
    {
        return midiOutputs.size();
    }

    int getNumDevices(bool isInput)
    {
        return isInput ? getNumMidiInputs()
            : getNumMidiOutputs();
    }

    ReferenceCountedObjectPtr<MidiDeviceListEntry> getMidiDevice(int index, bool isInput) const noexcept
    {
        return isInput ? midiInputs[index] : midiOutputs[index];
    }

    void openDevice(bool isInput, int index)
    {
        if (isInput)
        {
            jassert(midiInputs[index]->inDevice.get() == nullptr);
            midiInputs[index]->inDevice = MidiInput::openDevice(midiInputs[index]->deviceInfo.identifier, this);

            if (midiInputs[index]->inDevice.get() == nullptr)
            {
                DBG("*** MidiDemo::openDevice: open input device for index = " << index << " failed!");
                return;
            }

            midiInputs[index]->inDevice->start();
        }
        else
        {
            jassert(midiOutputs[index]->outDevice.get() == nullptr);
            midiOutputs[index]->outDevice = MidiOutput::openDevice(midiOutputs[index]->deviceInfo.identifier);

            // Remember the MidiView Monitor so we can duplicate output to Monitor
            if (midiOutputs[index]->deviceInfo.name == "MidiView")
                midiviewidx = index;

            if (midiOutputs[index]->outDevice.get() == nullptr) {
                DBG("*** AMidiDevices::openDevice: open output device for index: " << index << " failed!");
            }
            else {
                juce::Logger::writeToLog("*** AMidiDevices: Discovered device " + midiOutputs[index]->deviceInfo.name);
            }
        }
    }

    void closeDevice(bool isInput, int index)
    {
        auto& list = isInput ? midiInputs : midiOutputs;
        list[index]->stopAndReset();
    }

    //-------------------------------------------------------------------------
    // Process Inbound Midi Messages and Send to Output
    //-------------------------------------------------------------------------
    void handleIncomingMidiMessage(MidiInput*, const MidiMessage& message) override
    {
        //--- To do: Should we be adding messages to buffer rather than straight out to midiOutput
        int inchan = message.getChannel();

        // Check to see if Midi passthrough is blocked for this channel and igore all messages if true
        if (passthroughin[inchan] == false)
            return;

        // Send all Midi messages that are not Notes on to output with no change
        if (!(message.isNoteOn()) && !(message.isNoteOff())) {
            sendToOutputs(message);

            DBG("*** handleIncomingMidiMessage(): Original midi message sent " + std::to_string(inchan));
        }

        // Route MIDI In Note Messages to the 1) designated output by rewriting and message and 2) 
        // fanning it out per the Midi IO routing map when the messages is a Note On and Off
        // https://cpp.hotexamples.com/examples/juce/MidiMessage/getChannel/cpp-midimessage-getchannel-method-examples.html

        // Rewrite and reroute Note On and Off messages only to achieve layering on Organ, Orhestral, Symphonic and Solo channes
        int i = 0;
        while (i < 5) {     // Up to four layers supported in addition to original Midi in channel
            if (iomap[inchan][i] == 0) break;

            int note = message.getNoteNumber();

            if ((inchan == lowerchan) && (splitout[lowersolo] != 0)) {
                // Only output notes above split point to solo channel
                if ((note >= splitout[lowersolo]) && (iomap[inchan][i] != lowersolo)) {
                    i++;
                    continue;
                }
                // Do not output Solo channel below split point
                if ((note < splitout[lowersolo]) && (iomap[inchan][i] == lowersolo)) {
                    i++;
                    continue;
                }
            }

            if ((inchan == upperchan) && (splitout[uppersolo] != 0)) {
                // Only output notes above split point to solo channel
                if ((note >= splitout[uppersolo]) && (iomap[inchan][i] != uppersolo)) {
                    i++;
                    continue;
                }
                // Do not output Solo channel below split point
                if ((note < splitout[uppersolo]) && (iomap[inchan][i] == uppersolo)) {
                    i++;
                    continue;
                }
            }

            int outchan = iomap[inchan][i];
            rewriteSendNoteMessage(message, outchan);

            DBG("*** handleIncomingMidiMessage(): Layering midi message to " + std::to_string(outchan));

            i++;
        }
    }

    //-------------------------------------------------------------------------
    // Change the channel on Note On and Off Messages to be layered
    // and transpose octave if Button Group so configured
    //-------------------------------------------------------------------------
    bool rewriteSendNoteMessage(MidiMessage message, int midioutchan) {

        MidiMessage newmessage;

        if (!(message.isNoteOn()) && !(message.isNoteOff())) {
            return false;
        }

        // Get the Input Note and Octave transpose if required for Output Channel
        // Ignore transposes larger han -2 and +2. 
        int outchan = midioutchan;
        int octtranspose = octxpose[outchan];

        if ((octtranspose < -3) || (octtranspose > 3))
            octtranspose = 0;

        // To do: Add note boundary checks when transpose is used - based on how it is handled by Midi
        auto note = message.getNoteNumber() + octtranspose * 12;
        juce::uint8 velocity = message.getControllerValue();

        if (message.isNoteOn()) {

            // Check if output channel has Note On Velocity sensitivuty off in which case default 
            // to fixed value. Used for e.g. Organ voices. To do: Test for optimum default value
            if (velocityout[outchan] == false) {
                velocity = defvelocityout;
            }

            newmessage = MidiMessage::noteOn(
                outchan,
                note,
                velocity);
        }
        else if (message.isNoteOff()) {
            newmessage = MidiMessage::noteOff(
                outchan,
                note,
                velocity);
        }
        else return false;

        sendToOutputs(newmessage);

        return true;
    };

    // Send Midi Messages to all connected Output Devices
    void sendToOutputs(const MidiMessage& msg)
    {
        // Send msg to all active Outputs
        //for (auto midiOutput : midiOutputs) {
        //    if (midiOutput->outDevice != nullptr) {
        //        midiOutput->outDevice->sendMessageNow(msg);
        //    }
        //}

        // Route Midi Message to appropriate channel
        int outchan = msg.getChannel();
        int outmodidx = 0;
        if (outchan < 17)
            outmodidx = moduleout[outchan];

        if (outmodidx < midiOutputs.size()) {
            if (midiOutputs[outmodidx]->outDevice.get() != nullptr) {
                midiOutputs[outmodidx]->outDevice->sendMessageNow(msg);

                DBG("*** sendToOutputs " << outchan << " to module " << outmodidx << " sent!");
            }
            else {
                DBG("*** sendToOutputs " << outchan << " to module " << outmodidx << " failed!");
            }
        }

        // If MidiView connected duplicate message for display in monitor
        if (midiviewidx < midiOutputs.size()) {
            midiOutputs[midiviewidx]->outDevice->sendMessageNow(msg);
        }
    }

    void resetAllControllers()
    {
        // Reset all Controllers - is it implemented by BlackBox?
        for (int ichan = 1; ichan <= 16; ichan++)
        {
            auto ccMessage = juce::MidiMessage::controllerEvent(ichan, 121, 0);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs(ccMessage);
        }

        // All Notes Off
        for (int ichan = 1; ichan <= 16; ichan++)
        {
            auto ccMessage = juce::MidiMessage::controllerEvent(ichan, 123, 0);
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs(ccMessage);
        }
    }

    // Prepare the Midi routng IO map
    // Note: Using array of 17 to accomodate lookups for channel 1 to 16
    // Default to a OUT = IN mapping, and override with ButtonGroup config settings
    void clearMidiIOMap()
    {
        for (int i = 0; i < 17; i++) {
            for (int j = 0; j < 5; j++) {
                iomap[i][j] = 0;
            }
        }

        // Preset In to Out Channel
        for (int i = 0; i < 17; i++) {
            iomap[i][0] = i;
        }

        // Preset Octave Transpose for Midi output channels
        for (int i = 0; i < 17; i++) {
            octxpose[i] = 0;
        }

        // Preset Keyboard Split for Midi output channels
        for (int i = 0; i < 17; i++) {
            splitout[i] = 0;
        }

        // Preset Output MIDI Module for Button Group output channels
        // Updated by Button Group Midi Module selections in Config
        for (int i = 0; i < 17; i++) {
            moduleout[i] = 1;
        }

        // Turn pass through off for all MIDI IN channels by default. Updated
        // By Button Group for all iput channels actively used. Avoids
        // passing input channels through that sounds and not used!
        for (int i = 0; i < 17; i++) {
            passthroughin[i] = false;

            // Exception: Pass Midi CTRL on channel 16 through
            passthroughin[16] = true;
        }

        // Pass by velocity on Note On,but can be defaulted to 
        // fixed value for e.g. Organs and Keyboards
        for (int i = 0; i < 17; i++) {
            velocityout[i] = true;
        }
    };

    // Array 17, so that we can keep mapping at the 1 -16 channels instead of zero based
    int iomap[17][5];
    int octxpose[17];
    int splitout[17];
    int moduleout[17];
    bool passthroughin[17];
    bool velocityout[17];

    // Remember the MidiView monitoring output channel for message duplication
    int midiviewidx = 255;

    //==============================================================================
    ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;

    juce_DeclareSingleton(MidiDevices, true)

private:
    // Default constructor made private
    MidiDevices() {
        juce::Logger::writeToLog("=S= MidiDevices(): Constructor " + std::to_string(zinstcntmididevices++));

        clearMidiIOMap();
    }
};
juce_ImplementSingleton(MidiDevices)


//==============================================================================
// Class: MidiIOMap - Array used for quick access to all Midi in->out Routings
//==============================================================================
struct MidiIOMap final : Component
{

    MidiIOMap() {
        // Preset Channel IO Map
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 5; j++) {
                iomap[i][j] = 0;
            }
        }

        // Midi Notes Octave Transpose Map by Midi Output Channel
        for (int i = 0; i < 16; i++) {
            octxposemap[i] = 0;
        }
    };

    ~MidiIOMap() {};

    int iomap[16][5];
    int octxposemap[16];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiIOMap)
};


//==============================================================================
// Class: InstrumentModules - Midi Instruments Devices Supported
//          To do: Store Instrument List in File for easy adds
//==============================================================================
static int zinstcntInstrumentModules = 0;
class InstrumentModules final : Component
{
public:
    ~InstrumentModules() {
        DBG("=S= InstrumentModules(): Destructor " + std::to_string(--zinstcntInstrumentModules));
    };

    bool createInstrumentModule(int moduleidx, String displayname,
        String subdirectory, String modulename, String moduleidstring,
        String voicename, int MSB, int LSB, int font, bool iszerobased,
        bool isrotary, int rotortype, int rotorcc, int rotoroff, int rotorslow, int rotorfast) {

        if ((moduleidx < 0) || (moduleidx >= instrumentmodules.size())) {
            return false;
        }

        instrumentmodules[moduleidx]->setDisplayName(displayname);
        instrumentmodules[moduleidx]->setSubDirectory(subdirectory);
        instrumentmodules[moduleidx]->setModuleFileName(modulename);
        instrumentmodules[moduleidx]->setModuleIdString(moduleidstring);

        instrumentmodules[moduleidx]->setDefVoiceName(voicename);
        instrumentmodules[moduleidx]->setDefMSB(MSB);
        instrumentmodules[moduleidx]->setDefLSB(LSB);
        instrumentmodules[moduleidx]->setDefFont(font);
        instrumentmodules[moduleidx]->setIsZeroBased(iszerobased);

        instrumentmodules[moduleidx]->setIsRotary(isrotary);
        instrumentmodules[moduleidx]->setRotorType(rotortype);
        instrumentmodules[moduleidx]->setRotorCC(rotorcc);
        instrumentmodules[moduleidx]->setRotorOff(rotoroff);
        instrumentmodules[moduleidx]->setRotorSlow(rotorslow);
        instrumentmodules[moduleidx]->setRotorFast(rotorfast);

        return true;
    };

    bool setInstrumentModule(int mmoduleidx) {

        moduleidx = mmoduleidx;

        // Set Instrument Module Static Vars
        vendorname = instrumentmodules[mmoduleidx]->getDisplayName();           //"Deebach Blackbox";
        instrumentdname = instrumentmodules[mmoduleidx]->getSubDirectory();     // "BlackBox";
        instrumentfname = instrumentmodules[mmoduleidx]->getModuleFileName();   //"maxplus.json";

        String sdefVoice = instrumentmodules[mmoduleidx]->getDefVoiceName();
        int sdefMSB = instrumentmodules[mmoduleidx]->getDefMSB();
        int sdefLSB = instrumentmodules[mmoduleidx]->getDefLSB();
        int sdefFont = instrumentmodules[mmoduleidx]->getDefFont();
        iszerobased = instrumentmodules[mmoduleidx]->getIsZeroBased();

        isrotary = instrumentmodules[mmoduleidx]->getIsRotary();

        return true;
    }

    String getDisplayName(int moduleid) {
        return instrumentmodules[moduleid]->getDisplayName();
    }

    String getSubDirectory(int moduleid) {
        return instrumentmodules[moduleid]->getSubDirectory();
    }

    String getFileName(int moduleid) {
        return instrumentmodules[moduleid]->getModuleFileName();
    }

    String getModuleIdString(int moduleid) {
        return instrumentmodules[moduleid]->getModuleIdString();
    }

    String getDefVoiceName(int moduleid) {
        return instrumentmodules[moduleid]->getDefVoiceName();
    }

    int getDefMSB(int moduleid) {
        return instrumentmodules[moduleid]->getDefMSB();
    }

    int getDefLSB(int moduleid) {
        return instrumentmodules[moduleid]->getDefLSB();
    }

    int getDefFont(int moduleid) {
        return instrumentmodules[moduleid]->getDefFont();
    }

    int getNumModules() {
        return instrumentmodules.size();
    }

    bool isZeroBased(int moduleid) {
        return instrumentmodules[moduleid]->getIsZeroBased();
    }

    bool isZeroBased(String fname) {

        bool iszerobased = true;

        for (auto modulevar : instrumentmodules) {
            if (modulevar->getModuleFileName() == fname) {
                iszerobased = modulevar->getIsZeroBased();
                break;
            }
        }

        return iszerobased;
    }

    bool isRotary(int moduleid) {
        return instrumentmodules[moduleid]->getIsRotary();
    }

    int getRotorType(int moduleid) {
        return instrumentmodules[moduleid]->getRotorType();
    }

    int getRotorCC(int moduleid) {
        return instrumentmodules[moduleid]->getRotorCC();
    }

    int getRotorOff(int moduleid) {
        return instrumentmodules[moduleid]->getRotorOff();
    }

    int getRotorSlow(int moduleid) {
        return instrumentmodules[moduleid]->getRotorSlow();
    }

    int getRotorFast(int moduleid) {
        return instrumentmodules[moduleid]->getRotorFast();
    }

    //-------------------------------------------------------------------------
// Create Value Tree Module
//-------------------------------------------------------------------------
    ValueTree createVTModules()
    {
        static Identifier devicemodulesType("devicemodule");           // Pre-create an Identifier
        static Identifier defaultmoduleType("defaultmodule");

        // Configs ValueTree
        ValueTree vtmodules(devicemodulesType);

        // Preset the user selected default module
        vtmodules.setProperty(defaultmoduleType, moduleidx, nullptr);

        if (!vtmodules.isValid()) {
            juce::Logger::writeToLog("*** createVTModules(): An error occurred while creating final Modules ValueTree...");
            // To do: Add more error handling
        }

        return vtmodules;
    }

    //-------------------------------------------------------------------------
    // Save Modules Value Tree to Disk
    //-------------------------------------------------------------------------
    bool saveModules() {

        DBG("*** saveModules(): Saving Device Modules file to disk");

        // Prepare to save Instrument odules to Disk
        File outputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(configdir)
            .getChildFile(modulefname);
        if (outputFile.existsAsFile())
        {
            DBG("*** saveModules(): Modules file exists! Deleting to store new copy");
            outputFile.deleteFile();
        }
        FileOutputStream output(outputFile);

        vtmodules = createVTModules();

        // For testing
        //String vtxml = vtmodules.toXmlString();

        //void ValueTree::writeToStream(OutputStream & output)	const
        vtmodules.writeToStream(output);
        output.flush();
        if (output.getStatus().failed())
        {
            juce::Logger::writeToLog("*** saveModules(): Modules file save failed " + modulefname);
            // To do: Add more error handling
            return false;
        }
        juce::Logger::writeToLog("*** saveModules(): Modules file saved " + modulefname);

        // Roundtip Save/Reload and update vars
        //loadmodules();

        return true;
    }

    //-------------------------------------------------------------------------
    // Modules Load from known ValueTree
    //-------------------------------------------------------------------------
    bool loadVTModules()
    {

        static Identifier devicemodulesType("devicemodule");           // Pre-create an Identifier
        static Identifier defaultmoduleType("defaultmodule");

        if (!(vtmodules.isValid())) { //&& (vtmodules.getNumChildren() > 0))) {
            juce::Logger::writeToLog("*** loadVTModules(): An error occurred while reading temp Modules ValueTree...");

            // To do: Add more error handling
            return false;
        }

        // For testing
        //String vtxml = vtmodules.toXmlString();

        moduleidx = vtmodules.getProperty(defaultmoduleType);

        return true;
    }

    //-------------------------------------------------------------------------
    // Load Value Tree Modules from Disk
    //-------------------------------------------------------------------------
    bool loadModules() {
        juce::Logger::writeToLog("*** loadModules(): Loading Module file " + modulefname);

        // Reload Value tree to test
        // Prepare to read Configs Panel from Disk
        File inputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(configdir)
            .getChildFile(modulefname);
        if (!inputFile.existsAsFile())
        {
            juce::Logger::writeToLog("*** loadModules(): No Module file " + modulefname + " to load!");

            inputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                .getChildFile(organdir)
                .getChildFile(defmodulefname);
            juce::Logger::writeToLog("*** loadModules(): Loading file mastermodules.cfg instead");
        }

        FileInputStream input(inputFile);
        if (input.failedToOpen())
        {
            juce::Logger::writeToLog(" ***loadModuless(): Module file " + modulefname + " did not open! Need to recover from this...");

            return false;
        }
        else
        {
            static Identifier modulesType("modules");   // Pre-create an Identifier
            ValueTree temp_vtmodules(modulesType);

            // static ValueTree ValueTree::readFromStream(InputStream & input)
            temp_vtmodules = temp_vtmodules.readFromStream(input);

            // For testing
            //String xmlstring = temp_vtconfigs.toXmlString();

            if (!(temp_vtmodules.isValid())) //&& (temp_vtconfigs.getNumChildren() > 0)))
            {
                juce::Logger::writeToLog("*** loadModules(): Module file read into temp ValueTree failed! No change to Configs");

                // To do: Add some other error handling
                return false;
            }
            else
                vtmodules = temp_vtmodules;

            loadVTModules();
        }

        return true;
    }

    juce_DeclareSingleton(InstrumentModules, true)

private:

    // Private Instrument Module Class
    class InstrumentModule final {
    public:
        InstrumentModule() {};

        ~InstrumentModule() {};

        void setDisplayName(String mdisplayname) {
            displayname = mdisplayname;
        }

        String getDisplayName() {
            return displayname;
        }

        void setSubDirectory(String msubdirectory) {
            subdirectory = msubdirectory;
        }

        String getSubDirectory() {
            return subdirectory;
        }

        void setModuleFileName(String mmodulefilename) {
            modulefilename = mmodulefilename;
        }

        String getModuleFileName() {
            return modulefilename;
        }

        void setModuleIdString(String smoduleidstring) {
            moduleidstring = smoduleidstring;
        }

        String getModuleIdString() {
            return moduleidstring;
        }

        void setDefVoiceName(String mdefvoicename) {
            defvoicename = mdefvoicename;
        }

        String getDefVoiceName() {
            return defvoicename;
        }

        void setDefMSB(int mdefMSB) {
            defMSB = mdefMSB;
        }

        int getDefMSB() {
            return defMSB;
        }

        void setDefLSB(int mdefLSB) {
            defLSB = mdefLSB;
        }

        int getDefLSB() {
            return defLSB;
        }

        void setDefFont(int mdefFont) {
            defFont = mdefFont;
        }

        int getDefFont() {
            return defFont;
        }

        void setIsRotary(bool bisrotary) {
            isrotary = bisrotary;
        }

        bool getIsRotary() {
            return isrotary;
        }

        void setRotorType(int irotortype) {
            rotortype = irotortype;
        }

        int getRotorType() {
            return rotortype;
        }

        void setRotorCC(int irotorcc) {
            rotorcc = irotorcc;
        }

        int getRotorCC() {
            return rotorcc;
        }

        void setRotorOff(int irotoroff) {
            rotoroff = irotoroff;
        }

        int getRotorOff() {
            return rotoroff;
        }

        void setRotorSlow(int irotorslow) {
            rotorslow = irotorslow;
        }

        int getRotorSlow() {
            return rotorslow;
        }

        void setRotorFast(int irotorfast) {
            rotorfast = irotorfast;
        }

        int getRotorFast() {
            return rotorfast;
        }

        void setIsZeroBased(bool biszerobased) {
            iszero = biszerobased;
        }

        bool getIsZeroBased() {
            return iszero;
        }

    private:
        String displayname;
        String subdirectory;
        String modulefilename;
        String moduleidstring;

        String defvoicename;
        int defMSB;
        int defLSB;
        int defFont;
        bool iszero;

        bool isrotary;
        int rotortype;
        int rotorcc;
        int rotoroff;
        int rotorslow;
        int rotorfast;
    };

    // Singleton Constructor
    InstrumentModules() {
        juce::Logger::writeToLog("=S= InstrumentModules(): Constructor " + std::to_string(zinstcntInstrumentModules++));

        // Create and default Instruments. To be loaded from file in future
        instrumentmodules.add(new InstrumentModule());
        createInstrumentModule(0, "MidiGM", "MidiGM", "midigm.json", "MIDI",
            "Grand Piano", 0, 0, 1, true, 
            false, 0, 0, 0, 0, 0);

        instrumentmodules.add(new InstrumentModule());
        createInstrumentModule(1, "Deebach BlackBox", "BlackBox", "maxplus.json", "Blackbox",
            "Deebach Grand 1", 121, 8, 0, true, 
            true, 2, 0x74, 0, 0, 0);

        instrumentmodules.add(new InstrumentModule());
        createInstrumentModule(2, "Roland Integra7", "Integra7", "integra7.json", "INTEGRA",
            "Piano", 87, 64, 1, false, 
            true, 1, 0x01, 0, 0x10, 0x60);

        instrumentmodules.add(new InstrumentModule());
        createInstrumentModule(3, "Ketron SD2", "KetronSD2", "ketronsd2.json", "SD2",
            "Grand Piano", 0, 2, 0, true, 
            true, 1, 0x1E, 0, 0x40, 0X7F);

        instrumentmodules.add(new InstrumentModule());
        createInstrumentModule(4, "CustomGM", "Custom", "custom.json", "MIDI",
            "Grand Piano", 0, 0, 1, true, 
            false, 0, 0, 0, 0, 0);
    };

    OwnedArray<InstrumentModule> instrumentmodules;

    ValueTree vtmodules;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentModules)
};
juce_ImplementSingleton(InstrumentModules)

