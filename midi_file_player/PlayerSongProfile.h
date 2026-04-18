#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include "PlayerMidiIdentity.h"

struct PlayerChannelProfile
{
    int midiChannel = 1;
    bool configured = false;
    juce::String voice;
    juce::String category;
    int msb = 0;
    int lsb = 0;
    int program = 0;
    int vol = 100;
    int exp = 127;
    int rev = 20;
    int cho = 10;
    int mod = 0;
    int tim = 0;
    int atk = 0;
    int rel = 0;
    int bri = 30;
    int pan = 64;
};

struct PlayerSongProfile
{
    static constexpr int schemaVersionCurrent = 1;

    int schemaVersion = schemaVersionCurrent;
    juce::String profileId;
    juce::String displayName;
    juce::String createdUtc;
    juce::String updatedUtc;
    juce::String notes;

    PlayerMidiIdentity midiIdentity;

    int moduleIdx = 0;
    juce::String moduleDisplayName;
    juce::String moduleCatalogFile;

    bool enableProgramChangeRemap = true;
    bool enablePlayerStripCcScaling = false;
    int soloChannel = 0;
    std::array<bool, 17> mutedChannels {};

    std::array<PlayerChannelProfile, 16> channels {};

    int selectedChannelIdx = -1;
    juce::String lastEditedTab;
};

namespace PlayerSongProfileCodec
{
inline int clampMidi7(int value)
{
    return juce::jlimit(0, 127, value);
}

inline juce::String utcNowIso8601()
{
    return juce::Time::getCurrentTime().toISO8601(true);
}

inline juce::String encodeMutedChannels(const std::array<bool, 17>& mutedChannels)
{
    juce::String encoded;
    encoded.preallocateBytes(16);
    for (int ch = 1; ch <= 16; ++ch)
        encoded << (mutedChannels[(size_t) ch] ? '1' : '0');
    return encoded;
}

inline void decodeMutedChannels(const juce::String& encoded, std::array<bool, 17>& mutedChannels)
{
    mutedChannels.fill(false);
    for (int i = 0; i < encoded.length() && i < 16; ++i)
        mutedChannels[(size_t) (i + 1)] = encoded[i] == '1';
}

inline juce::var valueTreeToVar(const juce::ValueTree& tree)
{
    juce::DynamicObject::Ptr out(new juce::DynamicObject());
    out->setProperty("type", tree.getType().toString());

    juce::DynamicObject::Ptr props(new juce::DynamicObject());
    for (int i = 0; i < tree.getNumProperties(); ++i)
    {
        const auto name = tree.getPropertyName(i);
        props->setProperty(name.toString(), tree.getProperty(name));
    }
    out->setProperty("properties", juce::var(props.get()));

    juce::Array<juce::var> children;
    for (int i = 0; i < tree.getNumChildren(); ++i)
        children.add(valueTreeToVar(tree.getChild(i)));
    out->setProperty("children", children);

    return juce::var(out.get());
}

inline juce::ValueTree valueTreeFromVar(const juce::var& source)
{
    const auto* object = source.getDynamicObject();
    if (object == nullptr)
        return {};

    const auto typeName = object->getProperty("type").toString().trim();
    if (typeName.isEmpty())
        return {};

    juce::ValueTree tree(typeName);

    const auto propsVar = object->getProperty("properties");
    if (const auto* propsObj = propsVar.getDynamicObject())
    {
        for (const auto& entry : propsObj->getProperties())
            tree.setProperty(juce::Identifier(entry.name.toString()), entry.value, nullptr);
    }

    const auto childrenVar = object->getProperty("children");
    if (childrenVar.isArray())
    {
        const auto* children = childrenVar.getArray();
        if (children != nullptr)
        {
            for (const auto& item : *children)
            {
                auto childTree = valueTreeFromVar(item);
                if (childTree.isValid())
                    tree.addChild(childTree, -1, nullptr);
            }
        }
    }

    return tree;
}

inline juce::ValueTree toValueTree(const PlayerSongProfile& profile)
{
    juce::ValueTree root("playerSongProfile");
    root.setProperty("schemaVersion", juce::jmax(1, profile.schemaVersion), nullptr);
    root.setProperty("profileId", profile.profileId, nullptr);
    root.setProperty("displayName", profile.displayName, nullptr);
    root.setProperty("createdUtc", profile.createdUtc, nullptr);
    root.setProperty("updatedUtc", profile.updatedUtc, nullptr);
    root.setProperty("notes", profile.notes, nullptr);

    juce::ValueTree midiRef("midiRef");
    midiRef.setProperty("midiKey", profile.midiIdentity.midiKey, nullptr);
    midiRef.setProperty("originalPath", profile.midiIdentity.originalPath, nullptr);
    midiRef.setProperty("fileName", profile.midiIdentity.fileName, nullptr);
    midiRef.setProperty("fileSizeBytes", profile.midiIdentity.fileSizeBytes, nullptr);
    midiRef.setProperty("lastModifiedMs", profile.midiIdentity.lastModifiedMs, nullptr);
    root.addChild(midiRef, -1, nullptr);

    juce::ValueTree moduleRef("moduleRef");
    moduleRef.setProperty("moduleIdx", profile.moduleIdx, nullptr);
    moduleRef.setProperty("moduleDisplayName", profile.moduleDisplayName, nullptr);
    moduleRef.setProperty("moduleCatalogFile", profile.moduleCatalogFile, nullptr);
    root.addChild(moduleRef, -1, nullptr);

    juce::ValueTree flags("playerFlags");
    flags.setProperty("enableProgramChangeRemap", profile.enableProgramChangeRemap, nullptr);
    flags.setProperty("enablePlayerStripCcScaling", profile.enablePlayerStripCcScaling, nullptr);
    flags.setProperty("soloChannel", juce::jlimit(0, 16, profile.soloChannel), nullptr);
    flags.setProperty("mutedChannels", encodeMutedChannels(profile.mutedChannels), nullptr);
    root.addChild(flags, -1, nullptr);

    juce::ValueTree channelsNode("channels");
    for (int idx = 0; idx < 16; ++idx)
    {
        const auto& channel = profile.channels[(size_t) idx];
        juce::ValueTree channelNode("channel");
        channelNode.setProperty("midiChannel", juce::jlimit(1, 16, channel.midiChannel), nullptr);
        channelNode.setProperty("configured", channel.configured, nullptr);
        channelNode.setProperty("voice", channel.voice, nullptr);
        channelNode.setProperty("category", channel.category, nullptr);
        channelNode.setProperty("msb", clampMidi7(channel.msb), nullptr);
        channelNode.setProperty("lsb", clampMidi7(channel.lsb), nullptr);
        channelNode.setProperty("program", clampMidi7(channel.program), nullptr);
        channelNode.setProperty("vol", clampMidi7(channel.vol), nullptr);
        channelNode.setProperty("exp", clampMidi7(channel.exp), nullptr);
        channelNode.setProperty("rev", clampMidi7(channel.rev), nullptr);
        channelNode.setProperty("cho", clampMidi7(channel.cho), nullptr);
        channelNode.setProperty("mod", clampMidi7(channel.mod), nullptr);
        channelNode.setProperty("tim", clampMidi7(channel.tim), nullptr);
        channelNode.setProperty("atk", clampMidi7(channel.atk), nullptr);
        channelNode.setProperty("rel", clampMidi7(channel.rel), nullptr);
        channelNode.setProperty("bri", clampMidi7(channel.bri), nullptr);
        channelNode.setProperty("pan", clampMidi7(channel.pan), nullptr);
        channelsNode.addChild(channelNode, -1, nullptr);
    }
    root.addChild(channelsNode, -1, nullptr);

    juce::ValueTree uiState("uiState");
    uiState.setProperty("selectedChannelIdx", profile.selectedChannelIdx, nullptr);
    uiState.setProperty("lastEditedTab", profile.lastEditedTab, nullptr);
    root.addChild(uiState, -1, nullptr);
    return root;
}

inline bool readBool(const juce::ValueTree& node, const juce::Identifier& key, bool fallback)
{
    if (!node.hasProperty(key))
        return fallback;
    const auto v = node.getProperty(key);
    return v.isBool() ? static_cast<bool>(v) : fallback;
}

inline int readInt(const juce::ValueTree& node, const juce::Identifier& key, int fallback)
{
    if (!node.hasProperty(key))
        return fallback;
    const auto v = node.getProperty(key);
    if (v.isInt() || v.isInt64() || v.isDouble())
        return static_cast<int>(v);
    return fallback;
}

inline bool fromValueTree(const juce::ValueTree& root, PlayerSongProfile& outProfile, juce::String& error)
{
    error.clear();
    if (!root.isValid() || root.getType() != juce::Identifier("playerSongProfile"))
    {
        error = "Profile root must be playerSongProfile.";
        return false;
    }

    PlayerSongProfile profile;
    profile.schemaVersion = juce::jmax(1, readInt(root, "schemaVersion", PlayerSongProfile::schemaVersionCurrent));
    profile.profileId = root.getProperty("profileId").toString().trim();
    profile.displayName = root.getProperty("displayName").toString();
    profile.createdUtc = root.getProperty("createdUtc").toString();
    profile.updatedUtc = root.getProperty("updatedUtc").toString();
    profile.notes = root.getProperty("notes").toString();

    if (profile.profileId.isEmpty())
    {
        error = "Profile is missing profileId.";
        return false;
    }

    if (auto midiRef = root.getChildWithName("midiRef"); midiRef.isValid())
    {
        profile.midiIdentity.midiKey = midiRef.getProperty("midiKey").toString();
        profile.midiIdentity.originalPath = midiRef.getProperty("originalPath").toString();
        profile.midiIdentity.fileName = midiRef.getProperty("fileName").toString();
        profile.midiIdentity.fileSizeBytes = (int64) readInt(midiRef, "fileSizeBytes", 0);
        profile.midiIdentity.lastModifiedMs = (int64) readInt(midiRef, "lastModifiedMs", 0);
    }

    if (auto moduleRef = root.getChildWithName("moduleRef"); moduleRef.isValid())
    {
        profile.moduleIdx = juce::jmax(0, readInt(moduleRef, "moduleIdx", 0));
        profile.moduleDisplayName = moduleRef.getProperty("moduleDisplayName").toString();
        profile.moduleCatalogFile = moduleRef.getProperty("moduleCatalogFile").toString();
    }

    if (auto flags = root.getChildWithName("playerFlags"); flags.isValid())
    {
        profile.enableProgramChangeRemap = readBool(flags, "enableProgramChangeRemap", true);
        profile.enablePlayerStripCcScaling = readBool(flags, "enablePlayerStripCcScaling", false);
        profile.soloChannel = juce::jlimit(0, 16, readInt(flags, "soloChannel", 0));
        decodeMutedChannels(flags.getProperty("mutedChannels").toString(), profile.mutedChannels);
    }

    if (auto channelsNode = root.getChildWithName("channels"); channelsNode.isValid())
    {
        int loaded = 0;
        for (int i = 0; i < channelsNode.getNumChildren() && loaded < 16; ++i)
        {
            auto channelNode = channelsNode.getChild(i);
            if (channelNode.getType() != juce::Identifier("channel"))
                continue;

            auto& c = profile.channels[(size_t) loaded];
            c.midiChannel = juce::jlimit(1, 16, readInt(channelNode, "midiChannel", loaded + 1));
            c.configured = readBool(channelNode, "configured", false);
            c.voice = channelNode.getProperty("voice").toString();
            c.category = channelNode.getProperty("category").toString();
            c.msb = clampMidi7(readInt(channelNode, "msb", 0));
            c.lsb = clampMidi7(readInt(channelNode, "lsb", 0));
            c.program = clampMidi7(readInt(channelNode, "program", 0));
            c.vol = clampMidi7(readInt(channelNode, "vol", 100));
            c.exp = clampMidi7(readInt(channelNode, "exp", 127));
            c.rev = clampMidi7(readInt(channelNode, "rev", 20));
            c.cho = clampMidi7(readInt(channelNode, "cho", 10));
            c.mod = clampMidi7(readInt(channelNode, "mod", 0));
            c.tim = clampMidi7(readInt(channelNode, "tim", 0));
            c.atk = clampMidi7(readInt(channelNode, "atk", 0));
            c.rel = clampMidi7(readInt(channelNode, "rel", 0));
            c.bri = clampMidi7(readInt(channelNode, "bri", 30));
            c.pan = clampMidi7(readInt(channelNode, "pan", 64));
            ++loaded;
        }
    }

    if (auto uiState = root.getChildWithName("uiState"); uiState.isValid())
    {
        profile.selectedChannelIdx = juce::jlimit(-1, 15, readInt(uiState, "selectedChannelIdx", -1));
        profile.lastEditedTab = uiState.getProperty("lastEditedTab").toString();
    }

    outProfile = profile;
    return true;
}

inline juce::String toJsonString(const PlayerSongProfile& profile)
{
    const auto root = toValueTree(profile);
    return juce::JSON::toString(valueTreeToVar(root), true);
}

inline bool fromJsonString(const juce::String& jsonText, PlayerSongProfile& outProfile, juce::String& error)
{
    error.clear();
    const auto parsed = juce::JSON::parse(jsonText);
    if (parsed.isVoid())
    {
        error = "Invalid profile JSON.";
        return false;
    }

    const auto tree = valueTreeFromVar(parsed);
    if (!tree.isValid())
    {
        error = "Profile JSON tree is invalid.";
        return false;
    }
    return fromValueTree(tree, outProfile, error);
}
}
