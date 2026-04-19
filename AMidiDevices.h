/*
  ==============================================================================

    AMidiDevicesh.h
    Created: 14 Feb 2024 6:57:27pm
    Author:  a_min

  ==============================================================================
*/

#pragma once
#include <functional>
#include <map>


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

inline StringArray normalizeModuleMatchStrings(const StringArray& rawMatchers, const String& legacyMatcher = {})
{
    StringArray normalized;

    auto addMatcher = [&normalized](String matcher)
    {
        matcher = matcher.trim();
        if (matcher.isEmpty())
            return;

        bool alreadyPresent = false;
        for (const auto& existingMatcher : normalized)
        {
            if (existingMatcher.equalsIgnoreCase(matcher))
            {
                alreadyPresent = true;
                break;
            }
        }

        if (!alreadyPresent)
            normalized.add(matcher);
    };

    for (const auto& matcher : rawMatchers)
        addMatcher(matcher);

    addMatcher(legacyMatcher);
    return normalized;
}

inline bool doesModuleMatcherMatchDeviceName(const String& matcher, const String& deviceName)
{
    const String normalizedMatcher = matcher.trim().toLowerCase();
    const String normalizedDeviceName = deviceName.trim().toLowerCase();

    if (normalizedMatcher.isEmpty() || normalizedDeviceName.isEmpty())
        return false;

    // Avoid broad generic "midi" substring matches (e.g. "Integra ... MIDI 1")
    // which can incorrectly map unrelated modules.
    if (normalizedMatcher == "midi")
        return normalizedDeviceName.startsWith("midi") || normalizedDeviceName == "midi";

    return normalizedDeviceName.contains(normalizedMatcher);
}

inline bool doesAnyModuleMatcherMatchDeviceName(const StringArray& rawMatchers, const String& deviceName)
{
    const StringArray matchers = normalizeModuleMatchStrings(rawMatchers);
    for (const auto& matcher : matchers)
    {
        if (doesModuleMatcherMatchDeviceName(matcher, deviceName))
            return true;
    }

    return false;
}



//==============================================================================
// Class: MidiDevices - Singleton
// 
// Usage Example
// MySingleton* m = MySingleton::getInstance(); // creates the singleton if there isn't already one.
// ...
// MySingleton::deleteInstance(); // safely deletes the singleton (if it's been created).
//==============================================================================
class InstrumentModules;

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

    bool openDevice(bool isInput, int index)
    {
        auto& list = isInput ? midiInputs : midiOutputs;
        if (index < 0 || index >= list.size())
            return false;

        if (isInput)
        {
            jassert(midiInputs[index]->inDevice.get() == nullptr);
            midiInputs[index]->inDevice = MidiInput::openDevice(midiInputs[index]->deviceInfo.identifier, this);

            if (midiInputs[index]->inDevice.get() == nullptr)
            {
                DBG("*** MidiDemo::openDevice: open input device for index = " << index << " failed!");
                return false;
            }

            midiInputs[index]->inDevice->start();
            return true;
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
                return false;
            }
            else {
                juce::Logger::writeToLog("*** AMidiDevices: Discovered device " + midiOutputs[index]->deviceInfo.name);
                return true;
            }
        }
    }

    bool closeDevice(bool isInput, int index)
    {
        auto& list = isInput ? midiInputs : midiOutputs;
        if (index < 0 || index >= list.size())
            return false;

        list[index]->stopAndReset();
        return true;
    }

    bool isValidMidiChannel(int channel) const noexcept
    {
        return channel >= 1 && channel <= 16;
    }

    int getTransposedNoteForOutput(int inputNote, int midioutchan) const noexcept
    {
        if (!isValidMidiChannel(midioutchan))
            return juce::jlimit(0, 127, inputNote);

        int octtranspose = octxpose[midioutchan];
        if (octtranspose < -3 || octtranspose > 3)
            octtranspose = 0;

        return juce::jlimit(0, 127, inputNote + octtranspose * 12);
    }

    bool shouldRouteLayeredNote(int inchan, int note, int outchan) const noexcept
    {
        if (!isValidMidiChannel(inchan) || !isValidMidiChannel(outchan))
            return false;

        if (inchan == lowerchan && splitout[lowersolo] != 0)
        {
            // Above (or equal) split routes only to solo channel.
            if (note >= splitout[lowersolo] && outchan != lowersolo)
                return false;

            // Below split excludes solo channel.
            if (note < splitout[lowersolo] && outchan == lowersolo)
                return false;
        }

        if (inchan == upperchan && splitout[uppersolo] != 0)
        {
            // Above (or equal) split routes only to solo channel.
            if (note >= splitout[uppersolo] && outchan != uppersolo)
                return false;

            // Below split excludes solo channel.
            if (note < splitout[uppersolo] && outchan == uppersolo)
                return false;
        }

        return true;
    }

    void setOutputChannelMuted(int outchan, bool muted) noexcept
    {
        if (!isValidMidiChannel(outchan))
            return;
        outputChannelMuted[outchan] = muted;
    }

    bool isOutputChannelMuted(int outchan) const noexcept
    {
        if (!isValidMidiChannel(outchan))
            return false;
        return outputChannelMuted[outchan];
    }

    void clearSysExThroughRoutes()
    {
        const juce::SpinLock::ScopedLockType routeLock(sysexThroughRoutesLock);
        sysexInputToOutputIndices.clear();
    }

    void addSysExThroughRouteForInputIdentifier(const juce::String& inputIdentifier, const juce::Array<int>& outputDeviceIndices)
    {
        const juce::String normalizedInput = inputIdentifier.trim().toLowerCase();
        if (normalizedInput.isEmpty())
            return;

        const juce::SpinLock::ScopedLockType routeLock(sysexThroughRoutesLock);
        auto& outputs = sysexInputToOutputIndices[normalizedInput];
        for (int i = 0; i < outputDeviceIndices.size(); ++i)
        {
            const int outIdx = outputDeviceIndices.getUnchecked(i);
            if (outIdx >= 0 && outIdx < midiOutputs.size())
                outputs.addIfNotAlreadyThere(outIdx);
        }
    }

    bool routeIncomingSysExForInputIdentifier(const juce::String& inputIdentifier, const MidiMessage& message)
    {
        return routeIncomingSysExForInputIdentifierInternal(inputIdentifier, message);
    }

    //-------------------------------------------------------------------------
    // Process Inbound Midi Messages and Send to Output
    //-------------------------------------------------------------------------
    void handleIncomingMidiMessage(MidiInput* sourceInput, const MidiMessage& message) override
    {
        std::function<void(const MidiMessage&, const juce::String&)> incomingMonitorHookCopy;
        {
            const juce::SpinLock::ScopedLockType hookLock(incomingMonitorHookLock);
            incomingMonitorHookCopy = incomingMonitorHook;
        }
        if (incomingMonitorHookCopy)
            incomingMonitorHookCopy(message, resolveInputLabelForSource(sourceInput));

        //--- To do: Should we be adding messages to buffer rather than straight out to midiOutput
        int inchan = message.getChannel();

        if (message.isSysEx())
        {
            routeIncomingSysExForInputIdentifierInternal(resolveInputIdentifierForSource(sourceInput), message);
            return;
        }

        // Non-channel messages are currently dropped after monitor capture.
        if (!isValidMidiChannel(inchan))
            return;

        // Config-global Program Change trigger for "Next preset" (consumed when matched).
        if (message.isProgramChange()
            && inchan == juce::jlimit(1, 16, getAppState().presetMidiPcInputChannel)
            && message.getProgramChangeNumber() == juce::jlimit(0, 127, getAppState().presetMidiPcValue))
        {
            auto triggerCopy = presetNextTriggerHook;
            if (triggerCopy)
            {
                auto* messageManager = juce::MessageManager::getInstanceWithoutCreating();
                if (messageManager == nullptr || juce::MessageManager::existsAndIsCurrentThread())
                    triggerCopy();
                else
                    juce::MessageManager::callAsync([triggerCopy]()
                        {
                            triggerCopy();
                        });
            }
            return;
        }

        // Check to see if Midi passthrough is blocked for this channel and igore all messages if true
        if (passthroughin[inchan] == false)
            return;

        if (message.isController() && inchan == 16)
        {
            routeGlobalCcThroughForChannel16(message);
            return;
        }

        // Route non-note channelized messages only to configured output channels.
        if (!(message.isNoteOn()) && !(message.isNoteOff()))
        {
            bool routed = false;
            for (int i = 0; i < 5; ++i)
            {
                const int outchan = iomap[inchan][i];
                if (outchan == 0)
                    break;
                if (!isValidMidiChannel(outchan))
                    continue;

                auto routedMessage = message;
                routedMessage.setChannel(outchan);
                sendToOutputs(routedMessage);
                routed = true;
            }

            if (routed)
                DBG("*** handleIncomingMidiMessage(): Routed non-note message from " + std::to_string(inchan));
            return;
        }

        // Route MIDI In Note Messages to the 1) designated output by rewriting and message and 2) 
        // fanning it out per the Midi IO routing map when the messages is a Note On and Off
        // https://cpp.hotexamples.com/examples/juce/MidiMessage/getChannel/cpp-midimessage-getchannel-method-examples.html

        // Rewrite and reroute Note On and Off messages only to achieve layering on Organ, Orhestral, Symphonic and Solo channes
        int i = 0;
        while (i < 5) {     // Up to four layers supported in addition to original Midi in channel
            if (iomap[inchan][i] == 0) break;

            const int note = message.getNoteNumber();
            const int outchan = iomap[inchan][i];

            // Hard mute: suppress all note traffic while output channel is muted.
            if (message.isNoteOnOrOff() && isOutputChannelMuted(outchan))
            {
                i++;
                continue;
            }

            if (!shouldRouteLayeredNote(inchan, note, outchan))
            {
                i++;
                continue;
            }

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

        if (!isValidMidiChannel(midioutchan))
            return false;

        // Get the Input Note and Octave transpose if required for Output Channel
        // Ignore transposes larger than -3 and +3.
        int outchan = midioutchan;
        auto note = getTransposedNoteForOutput(message.getNoteNumber(), outchan);
        juce::uint8 velocity = 0;
        if (message.getRawDataSize() >= 3)
            velocity = message.getRawData()[2];

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
    void sendToOutputs(const MidiMessage& msg, bool bypassOutputChannelMuteGate = false)
    {
        const int outchan = msg.getChannel();

        // Hard mute gate for any direct channelized note emissions.
        if (!bypassOutputChannelMuteGate
            && msg.isNoteOnOrOff()
            && isValidMidiChannel(outchan)
            && isOutputChannelMuted(outchan))
            return;

        if (testSendHook)
            testSendHook(msg);

        std::function<void(const MidiMessage&, const juce::String&)> monitorHookCopy;
        {
            const juce::SpinLock::ScopedLockType hookLock(outgoingMonitorHookLock);
            monitorHookCopy = outgoingMonitorHook;
        }

        // Route MIDI message to all mapped output modules for this channel.
        bool hasMappedRoute = false;
        if (outchan > 0 && outchan < 17)
        {
            hasMappedRoute = sendToMappedOutputsForRouteChannel(msg, outchan, monitorHookCopy, true);
        }

        // Keep monitor visibility for unmapped channel traffic.
        if (!hasMappedRoute && monitorHookCopy)
            monitorHookCopy(msg, {});

        // If MidiView connected duplicate message for display in monitor
        if (midiviewidx < midiOutputs.size()) {
            if (midiOutputs[midiviewidx]->outDevice.get() != nullptr)
                midiOutputs[midiviewidx]->outDevice->sendMessageNow(msg);
        }
    }

    bool routeGlobalCcThroughForChannel16(const MidiMessage& message)
    {
        bool routed = false;
        std::function<void(const MidiMessage&, const juce::String&)> monitorHookCopy;
        {
            const juce::SpinLock::ScopedLockType hookLock(outgoingMonitorHookLock);
            monitorHookCopy = outgoingMonitorHook;
        }

        for (int groupIndex = 0; groupIndex < numberbuttongroups; ++groupIndex)
        {
            if (!globalCcThroughEnabledByGroup[groupIndex])
                continue;

            const int routeChannel = globalCcThroughOutputChannelByGroup[groupIndex];
            if (!isValidMidiChannel(routeChannel))
                continue;

            const bool sent = sendToMappedOutputsForRouteChannel(message, routeChannel, monitorHookCopy, false);
            routed = routed || sent;

            DBG("*** routeGlobalCcThroughForChannel16(): Routed CC to group " << groupIndex << " via midiout " << routeChannel);
        }

        if (!routed && monitorHookCopy)
            monitorHookCopy(message, {});

        if (midiviewidx < midiOutputs.size()) {
            if (midiOutputs[midiviewidx]->outDevice.get() != nullptr)
                midiOutputs[midiviewidx]->outDevice->sendMessageNow(message);
        }

        return routed;
    }

    void setOutgoingMidiMonitor(std::function<void(const MidiMessage&, const juce::String&)> hook)
    {
        const juce::SpinLock::ScopedLockType hookLock(outgoingMonitorHookLock);
        outgoingMonitorHook = std::move(hook);
    }

    void setIncomingMidiMonitor(std::function<void(const MidiMessage&, const juce::String&)> hook)
    {
        const juce::SpinLock::ScopedLockType hookLock(incomingMonitorHookLock);
        incomingMonitorHook = std::move(hook);
    }

    void setPresetNextProgramChangeTrigger(std::function<void()> hook)
    {
        presetNextTriggerHook = std::move(hook);
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
    // Default to no routes; Config writes allowed mappings explicitly.
    void clearMidiIOMap()
    {
        for (int i = 0; i < 17; i++) {
            for (int j = 0; j < 5; j++) {
                iomap[i][j] = 0;
            }
        }

        // Preset Octave Transpose for Midi output channels
        for (int i = 0; i < 17; i++) {
            octxpose[i] = 0;
        }

        // Preset Keyboard Split for Midi output channels
        for (int i = 0; i < 17; i++) {
            splitout[i] = 0;
        }

        // Preset Output MIDI Module(s) for Button Group output channels.
        // Empty means "no configured module route".
        for (int i = 0; i < 17; i++) {
            moduleout[i].clearQuick();
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

        // Output-channel hard mute state defaults to unmuted.
        for (int i = 0; i < 17; i++) {
            outputChannelMuted[i] = false;
        }

        for (int i = 0; i < numberbuttongroups; ++i) {
            globalCcThroughEnabledByGroup[i] = false;
            globalCcThroughOutputChannelByGroup[i] = 0;
        }
    };

    // Array 17, so that we can keep mapping at the 1 -16 channels instead of zero based
    int iomap[17][5];
    int octxpose[17];
    int splitout[17];
    juce::Array<int> moduleout[17];
    bool passthroughin[17];
    bool velocityout[17];
    bool outputChannelMuted[17];
    bool globalCcThroughEnabledByGroup[numberbuttongroups];
    int globalCcThroughOutputChannelByGroup[numberbuttongroups];

    // Remember the MidiView monitoring output channel for message duplication
    int midiviewidx = 255;

    // Test hook for observing emitted output messages without hardware.
    std::function<void(const MidiMessage&)> testSendHook;
    std::function<void()> presetNextTriggerHook;
    juce::SpinLock incomingMonitorHookLock;
    std::function<void(const MidiMessage&, const juce::String&)> incomingMonitorHook;
    juce::SpinLock outgoingMonitorHookLock;
    std::function<void(const MidiMessage&, const juce::String&)> outgoingMonitorHook;
    juce::SpinLock sysexThroughRoutesLock;
    std::map<juce::String, juce::Array<int>> sysexInputToOutputIndices;

    //==============================================================================
    ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;

    juce_DeclareSingleton(MidiDevices, true)

private:
    bool sendToMappedOutputsForRouteChannel(const MidiMessage& msg,
                                            int routeChannel,
                                            const std::function<void(const MidiMessage&, const juce::String&)>& monitorHookCopy,
                                            bool logOnSuccess)
    {
        if (!isValidMidiChannel(routeChannel))
            return false;

        bool sentToOutput = false;
        const auto& mappedModules = moduleout[routeChannel];
        for (int i = 0; i < mappedModules.size(); ++i)
        {
            const int outmodidx = mappedModules.getUnchecked(i);
            juce::String routedModuleName;
            if (outmodidx >= 0 && outmodidx < midiOutputs.size())
                routedModuleName = midiOutputs[outmodidx]->deviceInfo.name;

            if (monitorHookCopy)
                monitorHookCopy(msg, routedModuleName);

            if (outmodidx >= 0 && outmodidx < midiOutputs.size())
            {
                sentToOutput = true;
                if (midiOutputs[outmodidx]->outDevice.get() != nullptr)
                {
                    midiOutputs[outmodidx]->outDevice->sendMessageNow(msg);
                    if (logOnSuccess)
                        DBG("*** sendToOutputs " << routeChannel << " to module " << outmodidx << " sent!");
                }
                else if (logOnSuccess)
                {
                    DBG("*** sendToOutputs " << routeChannel << " to module " << outmodidx << " failed!");
                }
            }
        }

        return sentToOutput;
    }

    bool routeIncomingSysExForInputIdentifierInternal(const juce::String& inputIdentifier, const MidiMessage& message)
    {
        const juce::String normalizedInput = inputIdentifier.trim().toLowerCase();
        if (normalizedInput.isEmpty())
            return false;

        juce::Array<int> routedOutputIndices;
        {
            const juce::SpinLock::ScopedLockType routeLock(sysexThroughRoutesLock);
            const auto it = sysexInputToOutputIndices.find(normalizedInput);
            if (it == sysexInputToOutputIndices.end())
                return false;
            routedOutputIndices = it->second;
        }

        if (routedOutputIndices.isEmpty())
            return false;

        if (testSendHook)
            testSendHook(message);

        std::function<void(const MidiMessage&, const juce::String&)> monitorHookCopy;
        {
            const juce::SpinLock::ScopedLockType hookLock(outgoingMonitorHookLock);
            monitorHookCopy = outgoingMonitorHook;
        }

        bool sentToAtLeastOneOutput = false;
        for (int i = 0; i < routedOutputIndices.size(); ++i)
        {
            const int outmodidx = routedOutputIndices.getUnchecked(i);
            if (outmodidx < 0 || outmodidx >= midiOutputs.size())
                continue;

            juce::String routedModuleName = midiOutputs[outmodidx]->deviceInfo.name;
            if (monitorHookCopy)
                monitorHookCopy(message, routedModuleName);

            if (midiOutputs[outmodidx]->outDevice.get() != nullptr)
            {
                midiOutputs[outmodidx]->outDevice->sendMessageNow(message);
                sentToAtLeastOneOutput = true;
            }
        }

        if (midiviewidx < midiOutputs.size() && midiOutputs[midiviewidx]->outDevice.get() != nullptr)
            midiOutputs[midiviewidx]->outDevice->sendMessageNow(message);

        return sentToAtLeastOneOutput || testSendHook != nullptr;
    }

    juce::String resolveInputLabelForSource(MidiInput* sourceInput) const
    {
        if (sourceInput == nullptr)
            return "<unknown-input>";

        for (int i = 0; i < midiInputs.size(); ++i)
        {
            if (midiInputs[i] != nullptr && midiInputs[i]->inDevice.get() == sourceInput)
            {
                if (midiInputs[i]->deviceInfo.name.isNotEmpty())
                    return midiInputs[i]->deviceInfo.name;
                if (midiInputs[i]->deviceInfo.identifier.isNotEmpty())
                    return midiInputs[i]->deviceInfo.identifier;
                break;
            }
        }

        return "<unknown-input>";
    }

    juce::String resolveInputIdentifierForSource(MidiInput* sourceInput) const
    {
        if (sourceInput == nullptr)
            return {};

        for (int i = 0; i < midiInputs.size(); ++i)
        {
            if (midiInputs[i] != nullptr && midiInputs[i]->inDevice.get() == sourceInput)
                return midiInputs[i]->deviceInfo.identifier;
        }

        return {};
    }

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

    struct ModuleDefinition
    {
        int moduleIndex = -1;
        String displayName;
        String subDirectory;
        String moduleFileName;
        String moduleIdString;
        String defaultVoiceName;
        int defaultMSB = 0;
        int defaultLSB = 0;
        int defaultFont = 0;
        bool isZeroBased = true;
        bool isRotary = false;
        int rotorType = 0;
        int rotorCC = 0;
        int rotorOff = 0;
        int rotorSlow = 0;
        int rotorFast = 0;
        StringArray moduleMatchStrings;
    };

    bool createInstrumentModule(int moduleIndex, String displayname,
        String subdirectory, String modulename, String moduleidstring,
        String voicename, int MSB, int LSB, int font, bool isZeroBased,
        bool isRotary, int rotortype, int rotorcc, int rotoroff, int rotorslow, int rotorfast,
        const StringArray& moduleMatchStrings = {}) {

        if ((moduleIndex < 0) || (moduleIndex >= instrumentmodules.size())) {
            return false;
        }

        instrumentmodules[moduleIndex]->setDisplayName(displayname);
        instrumentmodules[moduleIndex]->setSubDirectory(subdirectory);
        instrumentmodules[moduleIndex]->setModuleFileName(modulename);
        instrumentmodules[moduleIndex]->setModuleMatchStrings(moduleMatchStrings, moduleidstring);

        instrumentmodules[moduleIndex]->setDefVoiceName(voicename);
        instrumentmodules[moduleIndex]->setDefMSB(MSB);
        instrumentmodules[moduleIndex]->setDefLSB(LSB);
        instrumentmodules[moduleIndex]->setDefFont(font);
        instrumentmodules[moduleIndex]->setIsZeroBased(isZeroBased);

        instrumentmodules[moduleIndex]->setIsRotary(isRotary);
        instrumentmodules[moduleIndex]->setRotorType(rotortype);
        instrumentmodules[moduleIndex]->setRotorCC(rotorcc);
        instrumentmodules[moduleIndex]->setRotorOff(rotoroff);
        instrumentmodules[moduleIndex]->setRotorSlow(rotorslow);
        instrumentmodules[moduleIndex]->setRotorFast(rotorfast);

        return true;
    };

    bool setInstrumentModule(int mmoduleidx) {
        if ((mmoduleidx < 0) || (mmoduleidx >= instrumentmodules.size()))
            return false;

        appState.moduleidx = mmoduleidx;

        // Set Instrument module state.
        appState.vendorname = instrumentmodules[mmoduleidx]->getDisplayName();
        appState.instrumentdname = instrumentmodules[mmoduleidx]->getSubDirectory();
        appState.instrumentfname = instrumentmodules[mmoduleidx]->getModuleFileName();

        appState.sdefVoice = instrumentmodules[mmoduleidx]->getDefVoiceName();
        appState.sdefMSB = instrumentmodules[mmoduleidx]->getDefMSB();
        appState.sdefLSB = instrumentmodules[mmoduleidx]->getDefLSB();
        appState.sdefFont = instrumentmodules[mmoduleidx]->getDefFont();
        appState.iszerobased = instrumentmodules[mmoduleidx]->getIsZeroBased();

        appState.isrotary = instrumentmodules[mmoduleidx]->getIsRotary();

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

    StringArray getModuleMatchStrings(int moduleid) {
        return instrumentmodules[moduleid]->getModuleMatchStrings();
    }

    bool deviceNameMatchesModule(int moduleid, const String& deviceName) {
        return doesAnyModuleMatcherMatchDeviceName(instrumentmodules[moduleid]->getModuleMatchStrings(), deviceName);
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

        bool zeroBased = true;

        for (auto modulevar : instrumentmodules) {
            if (modulevar->getModuleFileName() == fname) {
                zeroBased = modulevar->getIsZeroBased();
                break;
            }
        }

        return zeroBased;
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
        ValueTree modulesTree(devicemodulesType);

        // Preset the user selected default module
        modulesTree.setProperty(defaultmoduleType, appState.moduleidx, nullptr);

        if (!modulesTree.isValid()) {
            juce::Logger::writeToLog("*** createVTModules(): An error occurred while creating final Modules ValueTree...");
            // To do: Add more error handling
        }

        return modulesTree;
    }

    //-------------------------------------------------------------------------
    // Save Modules Value Tree to Disk
    //-------------------------------------------------------------------------
    bool saveModules() {

        DBG("*** saveModules(): Saving Device Modules file to disk");

        // Prepare to save Instrument odules to Disk
        File outputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(appState.configdir)
            .getChildFile(appState.modulefname);
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
            juce::Logger::writeToLog("*** saveModules(): Modules file save failed " + appState.modulefname);
            // To do: Add more error handling
            return false;
        }
        juce::Logger::writeToLog("*** saveModules(): Modules file saved " + appState.modulefname);

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

        const int loadedModuleIdx = (int) vtmodules.getProperty(defaultmoduleType);
        appState.moduleidx = instrumentmodules.size() > 0
            ? juce::jlimit(0, instrumentmodules.size() - 1, loadedModuleIdx)
            : 0;

        return true;
    }

    //-------------------------------------------------------------------------
    // Load Value Tree Modules from Disk
    //-------------------------------------------------------------------------
    bool loadModules() {
        juce::Logger::writeToLog("*** loadModules(): Loading Module file " + appState.modulefname);

        // Reload Value tree to test
        // Prepare to read Configs Panel from Disk
        File inputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(appState.configdir)
            .getChildFile(appState.modulefname);
        if (!inputFile.existsAsFile())
        {
            juce::Logger::writeToLog("*** loadModules(): No Module file " + appState.modulefname + " to load!");

            inputFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
                .getChildFile(organdir)
                .getChildFile(defmodulefname);
            juce::Logger::writeToLog("*** loadModules(): Loading file mastermodules.cfg instead");
        }

        FileInputStream input(inputFile);
        if (input.failedToOpen())
        {
            juce::Logger::writeToLog(" ***loadModuless(): Module file " + appState.modulefname + " did not open! Need to recover from this...");

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
    static constexpr const char* moduleCatalogFileName = "instrument_modules.json";

    static juce::Array<ModuleDefinition> getBuiltInModuleDefinitions()
    {
        auto defs = juce::Array<ModuleDefinition>{
            ModuleDefinition{ 0, "MidiGM", "MidiGM", "midigm.json", "MIDI", "Grand Piano", 0, 0, 1, true, false, 0, 0, 0, 0, 0 },
            ModuleDefinition{ 1, "Deebach BlackBox", "BlackBox", "maxplus.json", "Blackbox", "Deebach Grand 1", 121, 8, 0, true, true, 2, 0x74, 0, 20, 63 },
            ModuleDefinition{ 2, "Roland Integra-7", "Integra-7", "integra7.json", "INTEGRA-7", "Piano", 87, 64, 1, false, true, 1, 0x01, 0, 0x10, 0x60 },
            ModuleDefinition{ 3, "Roland AT900", "AT900MI", "at900mi.json", "AT900MI", "Grand Piano1", 121, 0, 0, true, false, 0, 0, 0, 0, 0 },
            ModuleDefinition{ 4, "MidiView", "MidiView", "midigm.json", "MIDIVIEW", "Grand Piano", 0, 0, 1, true, false, 0, 0, 0, 0, 0 },
            ModuleDefinition{ 5, "Ketron EVM", "KetronEVM", "ketronevm.json", "EVM", "Piano", 0, 0, 0, true, true, 1, 0x1E, 0, 0x40, 0x7F }
        };

        defs.getReference(2).moduleMatchStrings.add("INTEGRA-7");
        defs.getReference(4).moduleMatchStrings.addArray(juce::StringArray{ "MIDIVIEW", "MIDI VIEW", "MIDIVIEW PORT", "MIDIVIEW IN", "MIDIVIEW OUT" });
        defs.getReference(5).moduleMatchStrings.add("MIDI GADGET");
        return defs;
    }

    static bool loadModuleDefinitionsFromJson(const juce::File& catalogFile, juce::Array<ModuleDefinition>& outDefs)
    {
        outDefs.clear();

        if (!catalogFile.existsAsFile())
            return false;

        const auto parsed = juce::JSON::parse(catalogFile.loadFileAsString());
        if (parsed.isVoid())
            return false;

        const auto* root = parsed.getDynamicObject();
        if (root == nullptr)
            return false;

        const auto modulesVar = root->getProperty("modules");
        if (!modulesVar.isArray())
            return false;

        for (const auto& moduleVar : *modulesVar.getArray())
        {
            const auto* obj = moduleVar.getDynamicObject();
            if (obj == nullptr)
                continue;

            ModuleDefinition def;
            def.moduleIndex = obj->hasProperty("moduleIndex") ? (int) obj->getProperty("moduleIndex") : -1;
            def.displayName = obj->getProperty("displayName").toString();
            def.subDirectory = obj->getProperty("subDirectory").toString();
            def.moduleFileName = obj->getProperty("moduleFileName").toString();
            def.moduleIdString = obj->getProperty("moduleIdString").toString();
            const auto matchStringsVar = obj->getProperty("matchStrings");
            if (matchStringsVar.isArray())
            {
                for (const auto& matchVar : *matchStringsVar.getArray())
                    def.moduleMatchStrings.add(matchVar.toString());
            }

            def.defaultVoiceName = obj->getProperty("defaultVoiceName").toString();
            def.defaultMSB = (int) obj->getProperty("defaultMSB");
            def.defaultLSB = (int) obj->getProperty("defaultLSB");
            def.defaultFont = (int) obj->getProperty("defaultFont");
            def.isZeroBased = (bool) obj->getProperty("isZeroBased");
            def.isRotary = (bool) obj->getProperty("isRotary");
            def.rotorType = (int) obj->getProperty("rotorType");
            def.rotorCC = (int) obj->getProperty("rotorCC");
            def.rotorOff = (int) obj->getProperty("rotorOff");
            def.rotorSlow = (int) obj->getProperty("rotorSlow");
            def.rotorFast = (int) obj->getProperty("rotorFast");

            if (def.displayName.isNotEmpty() && def.moduleFileName.isNotEmpty())
                outDefs.add(def);
        }

        struct ModuleDefSorter
        {
            int compareElements(const ModuleDefinition& a, const ModuleDefinition& b) const
            {
                const bool aIndexed = a.moduleIndex >= 0;
                const bool bIndexed = b.moduleIndex >= 0;

                if (aIndexed && bIndexed)
                {
                    if (a.moduleIndex < b.moduleIndex) return -1;
                    if (a.moduleIndex > b.moduleIndex) return 1;
                    return 0;
                }

                if (aIndexed && !bIndexed) return -1;
                if (!aIndexed && bIndexed) return 1;
                return 0;
            }
        };

        ModuleDefSorter sorter;
        outDefs.sort(sorter, true);

        return outDefs.size() > 0;
    }

    void addInstrumentModule(const ModuleDefinition& def)
    {
        instrumentmodules.add(new InstrumentModule());
        const int idx = instrumentmodules.size() - 1;
        createInstrumentModule(idx,
            def.displayName,
            def.subDirectory,
            def.moduleFileName,
            def.moduleIdString,
            def.defaultVoiceName,
            def.defaultMSB,
            def.defaultLSB,
            def.defaultFont,
            def.isZeroBased,
            def.isRotary,
            def.rotorType,
            def.rotorCC,
            def.rotorOff,
            def.rotorSlow,
            def.rotorFast,
            def.moduleMatchStrings);
    }


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

        void setModuleMatchStrings(const StringArray& matchers, const String& legacyMatcher = {}) {
            modulematchstrings = normalizeModuleMatchStrings(matchers, legacyMatcher);
        }

        String getModuleIdString() {
            return modulematchstrings.isEmpty() ? String{} : modulematchstrings[0];
        }

        StringArray getModuleMatchStrings() {
            return modulematchstrings;
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
        StringArray modulematchstrings;

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

    AppState& appState;

    // Singleton Constructor
    InstrumentModules() : appState(getAppState()) {
        juce::Logger::writeToLog("=S= InstrumentModules(): Constructor " + std::to_string(zinstcntInstrumentModules++));
        const auto catalogFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(appState.configdir)
            .getChildFile(moduleCatalogFileName);

        juce::Array<ModuleDefinition> definitions;
        if (!loadModuleDefinitionsFromJson(catalogFile, definitions))
        {
            juce::Logger::writeToLog("*** InstrumentModules(): Failed to load module catalog JSON. Falling back to built-in defaults.");
            definitions = getBuiltInModuleDefinitions();
        }

        for (const auto& def : definitions)
            addInstrumentModule(def);
    };

    OwnedArray<InstrumentModule> instrumentmodules;

    ValueTree vtmodules;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentModules)
};
juce_ImplementSingleton(InstrumentModules)

