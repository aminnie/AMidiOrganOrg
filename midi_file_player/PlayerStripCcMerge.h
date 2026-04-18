#pragma once

#include <JuceHeader.h>
#include "AMidiInstruments.h"
#include "AMidiUtils.h"

namespace PlayerStripCcMerge
{
inline int mergeUnipolarCc7Bit(int fileValue, int stripValue)
{
    const auto clampedFile = juce::jlimit(0, 127, fileValue);
    const auto clampedStrip = juce::jlimit(0, 127, stripValue);
    const auto merged = juce::roundToInt((static_cast<double>(clampedFile) * static_cast<double>(clampedStrip)) / 127.0);
    return juce::jlimit(0, 127, merged);
}

inline bool shouldMergeControllerNumber(int controllerNumber)
{
    switch (controllerNumber)
    {
    case CCMod:
    case CCVol:
    case CCExp:
    case CCTim:
    case CCRel:
    case CCAtk:
    case CCBri:
    case CCRev:
    case CCCho:
        return true;
    default:
        return false;
    }
}

inline int getStripControllerValue(Instrument& instrument, int controllerNumber)
{
    switch (controllerNumber)
    {
    case CCMod: return instrument.getMod();
    case CCVol: return instrument.getVol();
    case CCExp: return instrument.getExp();
    case CCTim: return instrument.getTim();
    case CCRel: return instrument.getRel();
    case CCAtk: return instrument.getAtk();
    case CCBri: return instrument.getBri();
    case CCRev: return instrument.getRev();
    case CCCho: return instrument.getCho();
    default: return 127;
    }
}

inline bool mergeControllerWithStripIfApplicable(const juce::MidiMessage& incoming,
                                                 Instrument& stripInstrument,
                                                 juce::MidiMessage& mergedMessage)
{
    if (!incoming.isController())
        return false;

    const auto controllerNumber = incoming.getControllerNumber();
    if (!shouldMergeControllerNumber(controllerNumber))
        return false;

    const auto mergedValue = mergeUnipolarCc7Bit(incoming.getControllerValue(),
                                                 getStripControllerValue(stripInstrument, controllerNumber));
    mergedMessage = juce::MidiMessage::controllerEvent(incoming.getChannel(), controllerNumber, mergedValue);
    mergedMessage.setTimeStamp(incoming.getTimeStamp());
    return true;
}
}
