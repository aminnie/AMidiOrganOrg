#pragma once

#include <JuceHeader.h>
#include "AMidiUtils.h"
#include <array>

struct MidiFilePlayerSettings
{
    bool enableProgramChangeRemap = true;
    bool enablePlayerStripCcScaling = false;
    std::array<bool, 17> mutedChannels {};
    int soloChannel = 0;
    juce::String selectedOutputIdentifier;
    juce::String selectedInputIdentifier;

    static juce::File getConfigDirectory()
    {
        return getOrganUserDocumentsRoot().getChildFile(getAppState().configdir);
    }

    static juce::File getSettingsJsonFile()
    {
        return getConfigDirectory().getChildFile("midi_file_settings.json");
    }

    static juce::File getSelectedOutputFile()
    {
        return getConfigDirectory().getChildFile("selected_midi_output.txt");
    }

    static juce::File getSelectedInputFile()
    {
        return getConfigDirectory().getChildFile("selected_midi_input.txt");
    }

    bool load(juce::String& error)
    {
        error.clear();
        enableProgramChangeRemap = true;
        enablePlayerStripCcScaling = false;
        mutedChannels.fill(false);
        soloChannel = 0;

        const auto settingsFile = getSettingsJsonFile();
        if (!settingsFile.existsAsFile())
        {
            loadLegacyFallback();
            loadPersistedDeviceSelection();
            return true;
        }

        const auto parsed = juce::JSON::parse(settingsFile);
        const auto* object = parsed.getDynamicObject();
        if (object == nullptr)
        {
            error = "Invalid JSON in MIDI file settings.";
            return false;
        }

        const auto remapValue = object->getProperty("enableProgramChangeRemap");
        if (remapValue.isBool())
            enableProgramChangeRemap = static_cast<bool>(remapValue);

        const auto playerCcScalingValue = object->getProperty("enablePlayerStripCcScaling");
        if (playerCcScalingValue.isBool())
            enablePlayerStripCcScaling = static_cast<bool>(playerCcScalingValue);

        const auto mutedChannelsVar = object->getProperty("mutedChannels");
        if (auto* mutedArray = mutedChannelsVar.getArray())
        {
            for (int i = 0; i < mutedArray->size() && i < 16; ++i)
            {
                const auto entry = mutedArray->getReference(i);
                if (entry.isBool())
                    mutedChannels[static_cast<size_t>(i + 1)] = static_cast<bool>(entry);
            }
        }

        const auto soloChannelVar = object->getProperty("soloChannel");
        if (soloChannelVar.isInt() || soloChannelVar.isInt64() || soloChannelVar.isDouble())
            soloChannel = juce::jlimit(0, 16, static_cast<int>(soloChannelVar));

        loadPersistedDeviceSelection();
        return true;
    }

    bool save(juce::String& error) const
    {
        error.clear();
        juce::var settingsVar(new juce::DynamicObject());
        auto* settingsObject = settingsVar.getDynamicObject();
        if (settingsObject == nullptr)
        {
            error = "Failed to construct MIDI file settings object.";
            return false;
        }

        settingsObject->setProperty("enableProgramChangeRemap", enableProgramChangeRemap);
        settingsObject->setProperty("enablePlayerStripCcScaling", enablePlayerStripCcScaling);

        juce::Array<juce::var> mutedArray;
        for (int channel = 1; channel <= 16; ++channel)
            mutedArray.add(mutedChannels[static_cast<size_t>(channel)]);
        settingsObject->setProperty("mutedChannels", mutedArray);
        settingsObject->setProperty("soloChannel", juce::jlimit(0, 16, soloChannel));

        const auto settingsFile = getSettingsJsonFile();
        settingsFile.getParentDirectory().createDirectory();
        if (!settingsFile.replaceWithText(juce::JSON::toString(settingsVar, true)))
        {
            error = "Failed to write MIDI file settings.";
            return false;
        }

        savePersistedDeviceSelection();
        return true;
    }

private:
    void loadPersistedDeviceSelection()
    {
        const auto outputFile = getSelectedOutputFile();
        if (outputFile.existsAsFile())
            selectedOutputIdentifier = outputFile.loadFileAsString().trim();

        const auto inputFile = getSelectedInputFile();
        if (inputFile.existsAsFile())
            selectedInputIdentifier = inputFile.loadFileAsString().trim();
    }

    void savePersistedDeviceSelection() const
    {
        const auto outputFile = getSelectedOutputFile();
        outputFile.getParentDirectory().createDirectory();
        outputFile.replaceWithText(selectedOutputIdentifier);

        const auto inputFile = getSelectedInputFile();
        inputFile.getParentDirectory().createDirectory();
        inputFile.replaceWithText(selectedInputIdentifier);
    }

    void loadLegacyFallback()
    {
        const auto legacySettingsFile = getConfigDirectory().getChildFile("settings.json");
        if (!legacySettingsFile.existsAsFile())
            return;

        const auto parsed = juce::JSON::parse(legacySettingsFile);
        const auto* object = parsed.getDynamicObject();
        if (object == nullptr)
            return;

        const auto remapValue = object->getProperty("enableProgramChangeRemap");
        if (remapValue.isBool())
            enableProgramChangeRemap = static_cast<bool>(remapValue);

        const auto playerCcScalingValue = object->getProperty("enablePlayerStripCcScaling");
        if (playerCcScalingValue.isBool())
            enablePlayerStripCcScaling = static_cast<bool>(playerCcScalingValue);

        const auto mutedChannelsVar = object->getProperty("mutedChannels");
        if (auto* mutedArray = mutedChannelsVar.getArray())
        {
            for (int i = 0; i < mutedArray->size() && i < 16; ++i)
            {
                const auto entry = mutedArray->getReference(i);
                if (entry.isBool())
                    mutedChannels[static_cast<size_t>(i + 1)] = static_cast<bool>(entry);
            }
        }

        const auto soloChannelVar = object->getProperty("soloChannel");
        if (soloChannelVar.isInt() || soloChannelVar.isInt64() || soloChannelVar.isDouble())
            soloChannel = juce::jlimit(0, 16, static_cast<int>(soloChannelVar));
    }
};
