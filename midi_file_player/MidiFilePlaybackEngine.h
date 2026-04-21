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
        double sourceTimeSeconds = 0.0;
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

                const double sourceTime = msg.getTimeStamp();
                events.push_back({ sourceTime, sourceTime, msg });
            }
        }

        sortEventsByPlaybackTime();

        return !events.empty();
    }

    void clear()
    {
        stop();
        events.clear();
    }

    void resetPlaybackTimesFromSource()
    {
        for (auto& event : events)
            event.timeSeconds = event.sourceTimeSeconds;
        sortEventsByPlaybackTime();
    }

    void applyPlaybackTimeRemap(const std::function<double(double sourceSec)>& remap)
    {
        if (remap == nullptr)
        {
            resetPlaybackTimesFromSource();
            return;
        }

        for (auto& event : events)
            event.timeSeconds = remap(event.sourceTimeSeconds);

        sortEventsByPlaybackTime();
    }

    void start(double nowMs)
    {
        startFromPlaybackTime(nowMs, 0.0);
    }

    void startFromPlaybackTime(double nowMs, double playbackSec)
    {
        if (events.empty())
        {
            stop();
            return;
        }

        isPlaying = true;
        playbackStartMs = nowMs;
        seekToPlaybackTime(playbackSec);
    }

    void pause(double nowMs)
    {
        if (!isPlaying)
            return;

        const auto elapsedSinceStart = juce::jmax(0.0, (nowMs - playbackStartMs) / 1000.0);
        playbackOffsetSeconds += elapsedSinceStart;
        isPlaying = false;
        playbackStartMs = 0.0;
    }

    bool resume(double nowMs)
    {
        if (events.empty() || nextEventIndex >= static_cast<int>(events.size()))
            return false;

        isPlaying = true;
        playbackStartMs = nowMs;
        return true;
    }

    void stop()
    {
        isPlaying = false;
        nextEventIndex = 0;
        playbackStartMs = 0.0;
        playbackOffsetSeconds = 0.0;
    }

    void seekToPlaybackTime(double playbackSec)
    {
        if (events.empty())
        {
            nextEventIndex = 0;
            playbackOffsetSeconds = 0.0;
            return;
        }

        const double targetSec = juce::jmax(0.0, playbackSec);
        const double thresholdSec = juce::jmax(0.0, targetSec - 1.0e-6);
        const auto it = std::lower_bound(events.begin(), events.end(), thresholdSec,
            [](const ScheduledEvent& event, double value)
            {
                return event.timeSeconds < value;
            });

        nextEventIndex = static_cast<int>(std::distance(events.begin(), it));
        playbackOffsetSeconds = targetSec;
    }

    ProcessResult processUntil(double nowMs, const std::function<void(const juce::MidiMessage&)>& sink)
    {
        ProcessResult result;
        if (!isPlaying || sink == nullptr)
            return result;

        const auto elapsedSeconds = playbackOffsetSeconds + (nowMs - playbackStartMs) / 1000.0;
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
    bool hasPendingEvents() const { return nextEventIndex < static_cast<int>(events.size()); }
    int getEventCount() const { return static_cast<int>(events.size()); }
    const std::vector<ScheduledEvent>& getEvents() const { return events; }

private:
    void sortEventsByPlaybackTime()
    {
        std::sort(events.begin(), events.end(),
                  [](const ScheduledEvent& a, const ScheduledEvent& b)
                  {
                      return a.timeSeconds < b.timeSeconds;
                  });
    }

    std::vector<ScheduledEvent> events;
    bool isPlaying = false;
    int nextEventIndex = 0;
    double playbackStartMs = 0.0;
    double playbackOffsetSeconds = 0.0;
};
