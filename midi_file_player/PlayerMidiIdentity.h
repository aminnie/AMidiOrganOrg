#pragma once

#include <JuceHeader.h>

struct PlayerMidiIdentity
{
    juce::String midiKey;
    juce::String fileName;
    juce::String originalPath;
    int64 fileSizeBytes = 0;
    int64 lastModifiedMs = 0;
};

namespace PlayerMidiIdentityUtil
{
inline PlayerMidiIdentity buildIdentity(const juce::File& midiFile)
{
    PlayerMidiIdentity identity;
    identity.fileName = midiFile.getFileName();
    identity.originalPath = midiFile.getFullPathName();
    if (midiFile.existsAsFile())
    {
        identity.fileSizeBytes = midiFile.getSize();
        identity.lastModifiedMs = midiFile.getLastModificationTime().toMilliseconds();
    }

    // Phase-1 stable key: normalized path + size + mtime (hash can be added later).
    const auto normalizedPath = identity.originalPath.trim().toLowerCase();
    identity.midiKey = normalizedPath
        + "|"
        + juce::String(identity.fileSizeBytes)
        + "|"
        + juce::String(identity.lastModifiedMs);
    return identity;
}
}
