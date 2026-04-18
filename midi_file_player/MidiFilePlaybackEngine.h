#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <functional>
#include <vector>

class MidiFilePlaybackEngine
{
public:
    struct ScheduledEvent
    {
        double timeSeconds = 0.0;
        juce::MidiMessage message;
    };

    struct ProcessResult
    {
        int emittedEventCount = 0;
        bool reachedEndOfStream = false;
    };

    bool loadFromFile(const juce::File& file, juce::String& error)
    {
        stop();
        events.clear();
        error.clear();

        juce::FileInputStream stream(file);
        if (!stream.openedOk())
        {
            error = "Failed to open file: " + file.getFullPathName();
            return false;
        }

        juce::MidiFile midiFile;
        if (!midiFile.readFrom(stream))
        {
            error = "Failed to parse MIDI: " + file.getFileName();
            return false;
        }

        midiFile.convertTimestampTicksToSeconds();

        for (int track = 0; track < midiFile.getNumTracks(); ++track)
        {
            const auto* sequence = midiFile.getTrack(track);
            if (sequence == nullptr)
                continue;

            for (int eventIdx = 0; eventIdx < sequence->getNumEvents(); ++eventIdx)
            {
                const auto* eventHolder = sequence->getEventPointer(eventIdx);
                if (eventHolder == nullptr)
                    continue;

                const auto& msg = eventHolder->message;
                if (msg.isMetaEvent() || msg.isEndOfTrackMetaEvent())
                    continue;

                events.push_back({ msg.getTimeStamp(), msg });
            }
        }

        std::sort(events.begin(), events.end(),
                  [](const ScheduledEvent& a, const ScheduledEvent& b)
                  {
                      return a.timeSeconds < b.timeSeconds;
                  });

        return !events.empty();
    }

    void clear()
    {
        stop();
        events.clear();
    }

    void start(double nowMs)
    {
        if (events.empty())
        {
            stop();
            return;
        }

        isPlaying = true;
        nextEventIndex = 0;
        playbackStartMs = nowMs;
    }

    void stop()
    {
        isPlaying = false;
        nextEventIndex = 0;
        playbackStartMs = 0.0;
    }

    ProcessResult processUntil(double nowMs, const std::function<void(const juce::MidiMessage&)>& sink)
    {
        ProcessResult result;
        if (!isPlaying || sink == nullptr)
            return result;

        const auto elapsedSeconds = (nowMs - playbackStartMs) / 1000.0;
        while (nextEventIndex < static_cast<int>(events.size())
               && events[static_cast<size_t>(nextEventIndex)].timeSeconds <= elapsedSeconds)
        {
            sink(events[static_cast<size_t>(nextEventIndex)].message);
            ++nextEventIndex;
            ++result.emittedEventCount;
        }

        if (nextEventIndex >= static_cast<int>(events.size()))
        {
            isPlaying = false;
            result.reachedEndOfStream = true;
        }

        return result;
    }

    bool hasEvents() const { return !events.empty(); }
    bool getIsPlaying() const { return isPlaying; }
    int getEventCount() const { return static_cast<int>(events.size()); }
    const std::vector<ScheduledEvent>& getEvents() const { return events; }

private:
    std::vector<ScheduledEvent> events;
    bool isPlaying = false;
    int nextEventIndex = 0;
    double playbackStartMs = 0.0;
};
