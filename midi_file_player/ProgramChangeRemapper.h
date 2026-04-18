#pragma once

#include <JuceHeader.h>
#include <unordered_map>

class ProgramChangeRemapper
{
public:
    struct Mapping
    {
        int outProgram = 0;
        int bankMsb = -1;
        int bankLsb = -1;
    };

    void clear()
    {
        mappings.clear();
    }

    void addMapping(int channel, int incomingProgram, Mapping mapping)
    {
        if (!isValidChannel(channel) || !isValidMidiValue(incomingProgram)
            || !isValidMidiValue(mapping.outProgram))
        {
            return;
        }

        if (mapping.bankMsb >= 0 && !isValidMidiValue(mapping.bankMsb))
            return;

        if (mapping.bankLsb >= 0 && !isValidMidiValue(mapping.bankLsb))
            return;

        mappings[makeKey(channel, incomingProgram)] = mapping;
    }

    bool loadFromJsonFile(const juce::File& file, juce::String& error)
    {
        clear();
        error.clear();

        if (!file.existsAsFile())
        {
            error = "Lookup file does not exist: " + file.getFullPathName();
            return false;
        }

        const auto parsed = juce::JSON::parse(file);
        if (parsed.isVoid())
        {
            error = "Invalid JSON in lookup file: " + file.getFileName();
            return false;
        }

        return loadFromJsonVar(parsed, error);
    }

    bool loadFromJsonVar(const juce::var& root, juce::String& error)
    {
        clear();
        error.clear();

        const auto* object = root.getDynamicObject();
        if (object == nullptr)
        {
            error = "JSON root must be an object.";
            return false;
        }

        const auto mappingsVar = object->getProperty("mappings");
        if (!mappingsVar.isArray())
        {
            error = "JSON must contain a 'mappings' array.";
            return false;
        }

        const auto* array = mappingsVar.getArray();
        if (array == nullptr)
        {
            error = "Unable to read 'mappings' array.";
            return false;
        }

        for (int i = 0; i < array->size(); ++i)
        {
            const auto item = array->getReference(i);
            const auto* entry = item.getDynamicObject();
            if (entry == nullptr)
            {
                error = "Mapping entry is not an object at index " + juce::String(i);
                clear();
                return false;
            }

            if (!entry->hasProperty("channel")
                || !entry->hasProperty("incomingProgram")
                || !entry->hasProperty("outProgram"))
            {
                error = "Mapping entry is missing required fields at index " + juce::String(i);
                clear();
                return false;
            }

            const int channel = static_cast<int>(entry->getProperty("channel"));
            const int incomingProgram = static_cast<int>(entry->getProperty("incomingProgram"));
            Mapping mapping;
            mapping.outProgram = static_cast<int>(entry->getProperty("outProgram"));

            if (entry->hasProperty("bankMsb"))
                mapping.bankMsb = static_cast<int>(entry->getProperty("bankMsb"));
            if (entry->hasProperty("bankLsb"))
                mapping.bankLsb = static_cast<int>(entry->getProperty("bankLsb"));

            if (!isValidChannel(channel)
                || !isValidMidiValue(incomingProgram)
                || !isValidMidiValue(mapping.outProgram)
                || (mapping.bankMsb >= 0 && !isValidMidiValue(mapping.bankMsb))
                || (mapping.bankLsb >= 0 && !isValidMidiValue(mapping.bankLsb)))
            {
                error = "Mapping entry has invalid MIDI value at index " + juce::String(i);
                clear();
                return false;
            }

            mappings[makeKey(channel, incomingProgram)] = mapping;
        }

        return true;
    }

    // Returns number of replacement messages written into outMessages (0 means no replacement).
    int buildReplacementMessages(const juce::MidiMessage& incoming,
                                 juce::MidiMessage* outMessages,
                                 int maxMessages) const
    {
        if (!incoming.isProgramChange() || outMessages == nullptr || maxMessages <= 0)
            return 0;

        const auto channel = incoming.getChannel();
        const auto program = incoming.getProgramChangeNumber();
        const auto key = makeKey(channel, program);

        const auto it = mappings.find(key);
        if (it == mappings.end())
            return 0;

        const auto& mapping = it->second;
        int count = 0;

        if (mapping.bankMsb >= 0 && count < maxMessages)
            outMessages[count++] = juce::MidiMessage::controllerEvent(channel, 0, mapping.bankMsb);

        if (mapping.bankLsb >= 0 && count < maxMessages)
            outMessages[count++] = juce::MidiMessage::controllerEvent(channel, 32, mapping.bankLsb);

        if (count < maxMessages)
            outMessages[count++] = juce::MidiMessage::programChange(channel, mapping.outProgram);

        return count;
    }

    int getMappingCount() const
    {
        return static_cast<int>(mappings.size());
    }

private:
    static bool isValidChannel(int channel)
    {
        return channel >= 1 && channel <= 16;
    }

    static bool isValidMidiValue(int value)
    {
        return value >= 0 && value <= 127;
    }

    static int makeKey(int channel, int incomingProgram)
    {
        return (channel << 8) | incomingProgram;
    }

    std::unordered_map<int, Mapping> mappings;
};
