#pragma once

#include <JuceHeader.h>
#include <map>
#include <vector>
#include "AMidiUtils.h"
#include "PlayerSongProfile.h"

struct PlayerSongProfileIndexEntry
{
    juce::String profileId;
    juce::String displayName;
    juce::String midiKey;
    juce::String moduleDisplayName;
    juce::String profileFile;
    juce::String updatedUtc;
};

struct PlayerSongProfilesIndex
{
    int schemaVersion = 1;
    std::vector<PlayerSongProfileIndexEntry> entries;
    std::map<juce::String, juce::String> lastUsedByMidiKey;
};

namespace PlayerSongProfileStore
{
inline juce::File getProfilesDirectory()
{
    auto dir = getOrganUserDocumentsRoot()
        .getChildFile(getAppState().configdir)
        .getChildFile("player_profiles");
    if (!dir.isDirectory())
        dir.createDirectory();
    return dir;
}

inline juce::File getIndexFile()
{
    auto dir = getOrganUserDocumentsRoot().getChildFile(getAppState().configdir);
    if (!dir.isDirectory())
        dir.createDirectory();
    return dir.getChildFile("player_profiles_index.json");
}

inline juce::var toVar(const PlayerSongProfilesIndex& index)
{
    juce::DynamicObject::Ptr root(new juce::DynamicObject());
    root->setProperty("schemaVersion", juce::jmax(1, index.schemaVersion));

    juce::Array<juce::var> entries;
    for (const auto& entry : index.entries)
    {
        juce::DynamicObject::Ptr item(new juce::DynamicObject());
        item->setProperty("profileId", entry.profileId);
        item->setProperty("displayName", entry.displayName);
        item->setProperty("midiKey", entry.midiKey);
        item->setProperty("moduleDisplayName", entry.moduleDisplayName);
        item->setProperty("profileFile", entry.profileFile);
        item->setProperty("updatedUtc", entry.updatedUtc);
        entries.add(juce::var(item.get()));
    }
    root->setProperty("entries", entries);

    juce::DynamicObject::Ptr lastUsed(new juce::DynamicObject());
    for (const auto& kv : index.lastUsedByMidiKey)
        lastUsed->setProperty(kv.first, kv.second);
    root->setProperty("lastUsedByMidiKey", juce::var(lastUsed.get()));
    return juce::var(root.get());
}

inline PlayerSongProfilesIndex fromVar(const juce::var& source)
{
    PlayerSongProfilesIndex out;
    const auto* root = source.getDynamicObject();
    if (root == nullptr)
        return out;

    const auto schemaVersionVar = root->getProperty("schemaVersion");
    if (schemaVersionVar.isInt() || schemaVersionVar.isInt64() || schemaVersionVar.isDouble())
        out.schemaVersion = juce::jmax(1, static_cast<int>(schemaVersionVar));

    const auto entriesVar = root->getProperty("entries");
    if (const auto* entriesArray = entriesVar.getArray())
    {
        for (const auto& itemVar : *entriesArray)
        {
            const auto* item = itemVar.getDynamicObject();
            if (item == nullptr)
                continue;
            PlayerSongProfileIndexEntry entry;
            entry.profileId = item->getProperty("profileId").toString();
            entry.displayName = item->getProperty("displayName").toString();
            entry.midiKey = item->getProperty("midiKey").toString();
            entry.moduleDisplayName = item->getProperty("moduleDisplayName").toString();
            entry.profileFile = item->getProperty("profileFile").toString();
            entry.updatedUtc = item->getProperty("updatedUtc").toString();
            if (entry.profileId.isNotEmpty())
                out.entries.push_back(entry);
        }
    }

    const auto lastUsedVar = root->getProperty("lastUsedByMidiKey");
    if (const auto* lastUsed = lastUsedVar.getDynamicObject())
    {
        for (const auto& kv : lastUsed->getProperties())
        {
            const auto key = kv.name.toString();
            const auto value = kv.value.toString();
            if (key.isNotEmpty() && value.isNotEmpty())
                out.lastUsedByMidiKey[key] = value;
        }
    }
    return out;
}

inline bool loadIndex(PlayerSongProfilesIndex& outIndex, juce::String& error)
{
    error.clear();
    outIndex = {};

    const auto indexFile = getIndexFile();
    if (!indexFile.existsAsFile())
        return true;

    const auto parsed = juce::JSON::parse(indexFile);
    if (parsed.isVoid())
    {
        error = "Invalid player profile index JSON.";
        return false;
    }

    outIndex = fromVar(parsed);
    return true;
}

inline bool saveIndex(const PlayerSongProfilesIndex& index, juce::String& error)
{
    error.clear();
    const auto indexFile = getIndexFile();
    if (!indexFile.getParentDirectory().createDirectory())
    {
        error = "Could not create profile index directory.";
        return false;
    }
    if (!indexFile.replaceWithText(juce::JSON::toString(toVar(index), true)))
    {
        error = "Failed writing player profile index.";
        return false;
    }
    return true;
}

inline juce::File getProfileFileForId(const juce::String& profileId)
{
    return getProfilesDirectory().getChildFile(profileId + ".playerprofile.json");
}

inline bool saveProfile(const PlayerSongProfile& profile, juce::String& error)
{
    error.clear();
    if (profile.profileId.isEmpty())
    {
        error = "Profile id is required.";
        return false;
    }

    const auto profileFile = getProfileFileForId(profile.profileId);
    if (!profileFile.getParentDirectory().createDirectory())
    {
        error = "Could not create player profile directory.";
        return false;
    }
    if (!profileFile.replaceWithText(PlayerSongProfileCodec::toJsonString(profile)))
    {
        error = "Failed writing player profile.";
        return false;
    }
    return true;
}

inline bool loadProfileById(const juce::String& profileId, PlayerSongProfile& outProfile, juce::String& error)
{
    error.clear();
    outProfile = {};
    const auto profileFile = getProfileFileForId(profileId);
    if (!profileFile.existsAsFile())
    {
        error = "Profile file does not exist.";
        return false;
    }
    return PlayerSongProfileCodec::fromJsonString(profileFile.loadFileAsString(), outProfile, error);
}

inline void upsertIndexEntry(PlayerSongProfilesIndex& index, const PlayerSongProfile& profile)
{
    PlayerSongProfileIndexEntry entry;
    entry.profileId = profile.profileId;
    entry.displayName = profile.displayName;
    entry.midiKey = profile.midiIdentity.midiKey;
    entry.moduleDisplayName = profile.moduleDisplayName;
    entry.profileFile = getProfileFileForId(profile.profileId).getFileName();
    entry.updatedUtc = profile.updatedUtc;

    bool replaced = false;
    for (auto& existing : index.entries)
    {
        if (existing.profileId == entry.profileId)
        {
            existing = entry;
            replaced = true;
            break;
        }
    }
    if (!replaced)
        index.entries.push_back(entry);
}

inline std::vector<PlayerSongProfileIndexEntry> getProfilesForMidiKey(const PlayerSongProfilesIndex& index,
                                                                      const juce::String& midiKey)
{
    std::vector<PlayerSongProfileIndexEntry> result;
    for (const auto& entry : index.entries)
    {
        if (entry.midiKey == midiKey)
            result.push_back(entry);
    }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b)
        {
            return a.updatedUtc > b.updatedUtc;
        });
    return result;
}

inline juce::String getLastUsedProfileIdForMidiKey(const PlayerSongProfilesIndex& index, const juce::String& midiKey)
{
    const auto it = index.lastUsedByMidiKey.find(midiKey);
    return it != index.lastUsedByMidiKey.end() ? it->second : juce::String {};
}

inline void setLastUsedProfileIdForMidiKey(PlayerSongProfilesIndex& index,
                                           const juce::String& midiKey,
                                           const juce::String& profileId)
{
    if (midiKey.isEmpty() || profileId.isEmpty())
        return;
    index.lastUsedByMidiKey[midiKey] = profileId;
}
}
