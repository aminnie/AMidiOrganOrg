#pragma once

#include <JuceHeader.h>
#include "AMidiUtils.h"
#include <array>

struct MidiFilePlayerSettings
{
    bool autoLoadLastMidiOnStartup = true;
    bool enableProgramChangeRemap = true;
    std::array<bool, 17> mutedChannels {};
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

    static juce::File getLastMidiFilePathFile()
    {
        return getConfigDirectory().getChildFile("last_midi_file.txt");
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
        autoLoadLastMidiOnStartup = true;
        enableProgramChangeRemap = true;
        mutedChannels.fill(false);

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

        const auto autoLoadValue = object->getProperty("autoLoadLastMidiOnStartup");
        if (autoLoadValue.isBool())
            autoLoadLastMidiOnStartup = static_cast<bool>(autoLoadValue);

        const auto remapValue = object->getProperty("enableProgramChangeRemap");
        if (remapValue.isBool())
            enableProgramChangeRemap = static_cast<bool>(remapValue);

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

        settingsObject->setProperty("autoLoadLastMidiOnStartup", autoLoadLastMidiOnStartup);
        settingsObject->setProperty("enableProgramChangeRemap", enableProgramChangeRemap);

        juce::Array<juce::var> mutedArray;
        for (int channel = 1; channel <= 16; ++channel)
            mutedArray.add(mutedChannels[static_cast<size_t>(channel)]);
        settingsObject->setProperty("mutedChannels", mutedArray);

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

    juce::String loadLastMidiPath() const
    {
        const auto pathFile = getLastMidiFilePathFile();
        if (!pathFile.existsAsFile())
            return {};
        return pathFile.loadFileAsString().trim();
    }

    void saveLastMidiPath(const juce::File& midiFile) const
    {
        const auto pathFile = getLastMidiFilePathFile();
        pathFile.getParentDirectory().createDirectory();
        pathFile.replaceWithText(midiFile.getFullPathName());
    }

    void clearLastMidiPath() const
    {
        const auto pathFile = getLastMidiFilePathFile();
        if (pathFile.existsAsFile())
            pathFile.deleteFile();
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

        const auto autoLoadValue = object->getProperty("autoLoadLastMidiOnStartup");
        if (autoLoadValue.isBool())
            autoLoadLastMidiOnStartup = static_cast<bool>(autoLoadValue);

        const auto remapValue = object->getProperty("enableProgramChangeRemap");
        if (remapValue.isBool())
            enableProgramChangeRemap = static_cast<bool>(remapValue);

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
    }
};
