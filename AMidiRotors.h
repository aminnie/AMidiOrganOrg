/*
  ==============================================================================

    AMidiRotors.h
    Created: 14 Feb 2024 6:33:01pm
    Author:  a_min

  ==============================================================================
*/

#pragma once

//==============================================================================
// Class: Rotary
//==============================================================================
class Rotors : private ComponentListener
{
public:
    Rotors(Component& comp)
        : containerComponent(comp)
    {
        containerComponent.addComponentListener(this);
    }

    ~Rotors() override
    {
        containerComponent.removeComponentListener(this);
    }

    void setRotarySpeed(bool isFast)
    {
        speed = isFast;
    }

private:
    float speed;

    Thread::ThreadID threadId = {};

    Component& containerComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Rotors)
};


//==============================================================================
// Class: RotarySlowThread
//==============================================================================
class RotarySlowThread final : public Rotors,
    public Thread
{
public:
    RotarySlowThread(Component& containerComp, int midiout) : Rotors(containerComp),
        Thread("Rotary Upper Slow Thread"),
        startTime(juce::Time::getMillisecondCounterHiRes() * 0.001),
        mididevices(MidiDevices::getInstance()) // Creates the singleton if there isn't already one
    {
        midichannel = midiout;

        startThread();
    }

    ~RotarySlowThread() override
    {
        // allow the thread 2 seconds to stop cleanly - should be plenty of time.
        stopThread(2000);
    }

    // The code that runs this thread - we'll loop continuously,
    // threadShouldExit() returns true when the stopThread() method has been
    // called, so we should check it often, and exit as soon as it gets flagged.
    void run() override
    {
        // Ramp Rotary Up
        int i = 9;
        while (!threadShouldExit() && (i >= 2))
        {
            // Step the rotary speed change at 5 times per second
            wait(interval);

            // Send rotary change control value
            int controllerNumber = 74;    //
            juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
                midichannel,
                controllerNumber,
                rotarystep[i--]
            );
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }
    }

private:
    int interval = 200;
    int stepcount = 0;

    double startTime;
    std::unique_ptr<MidiDevices> mididevices;

    int midichannel = 0;

    // To do: Move Rotary Up and Down array Config page
    // To do: Consider adding similar for Brake On and Off
    int rotaryon[6] = { 0x63, 0x33, 0x62, 0x68, 0x06, 0x00 };
    int rotarydepth[6] = { 0x63, 0x33, 0x62, 0x6a, 0x06, 0x45 };
    int rotarystep[10] = { 0 * 6, 1 * 4, 2 * 5, 3 * 6, 4 * 6, 5 * 6, 6 * 7, 7 * 7, 8 * 7, 63 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotarySlowThread)
};


//==============================================================================
// Class: RotaryFastThread
//==============================================================================
class RotaryFastThread final : public Rotors,
    public Thread
{
public:
    RotaryFastThread(Component& containerComp, int midiout) : Rotors(containerComp),
        Thread("Rotary Upper Fast Thread"),
        startTime(juce::Time::getMillisecondCounterHiRes() * 0.001),
        mididevices(MidiDevices::getInstance()) // Creates the singleton if there isn't already one
    {
        midichannel = midiout;

        startThread();
    }

    ~RotaryFastThread() override
    {
        // allow the thread 2 seconds to stop cleanly - should be plenty of time.
        stopThread(2000);
    }

    // The code that runs this thread - we'll loop continuously,
    // threadShouldExit() returns true when the stopThread() method has been
    // called, so we should check it often, and exit as soon as it gets flagged.
    void run() override
    {
        // Ramp Rotary Up
        int i = 2;
        while (!threadShouldExit() && (i <= 9))
        {
            // Step the rotary speed change at 5 times per second
            wait(interval);

            // Send rotary change control value
            int controllerNumber = 74;    //
            juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(
                midichannel,
                controllerNumber,
                rotarystep[i++]
            );
            ccMessage.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            mididevices->sendToOutputs(ccMessage);
        }
    }

private:
    int interval = 200;
    int stepcount = 0;

    double startTime;
    std::unique_ptr<MidiDevices> mididevices;
    int midichannel = 0;

    int rotaryfast[6] = { 0x63, 0x33, 0x62, 0x69, 0x06, 0x00 };
    int rotarystep[10] = { 0 * 6, 1 * 4, 2 * 5, 3 * 6, 4 * 6, 5 * 6, 6 * 7, 7 * 7, 8 * 7, 63 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotaryFastThread)
};


