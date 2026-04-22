/*
  Manage saved Player (song) profiles: list, rename, delete, clean stale index rows.
  Non-modal document window; uses PlayerSongProfileStore.
*/

#pragma once

#include "PlayerSongProfileStore.h"
#include <JuceHeader.h>
#include <functional>

/** In-window content: list + actions. */
class PlayerProfilesManageContent final : public juce::Component,
                                            private juce::ListBoxModel
{
public:
    PlayerProfilesManageContent (const std::function<void()>& requestClose,
                                 PlayerSongProfilesIndex& index,
                                 juce::String& outActiveProfileId,
                                 juce::String& outActiveProfileDisplay,
                                 juce::String& outActiveProfileCreated,
                                 const std::function<void()>& onIndexMutated) :
        closeRequest (requestClose),
        profileIndex (index),
        activeId (outActiveProfileId),
        activeDisplay (outActiveProfileDisplay),
        activeCreated (outActiveProfileCreated),
        onMutated (onIndexMutated)
    {
        list.setModel (this);
        list.setRowHeight (26);
        list.setMultipleSelectionEnabled (false);
        addAndMakeVisible (list);
        for (juce::TextButton* b : { &renameButton, &deleteButton, &staleButton, &closeButton })
        {
            addAndMakeVisible (b);
        }
        // White on light default button is invisible; use a dark face so the labels read clearly.
        const auto renameDeleteText = juce::Colours::white;
        const auto renameDeleteFace = juce::Colours::darkgrey;
        const auto renameDeleteFaceOn = juce::Colours::darkgrey.brighter (0.12f);
        renameButton.setColour (juce::TextButton::textColourOffId, renameDeleteText);
        renameButton.setColour (juce::TextButton::textColourOnId, renameDeleteText);
        renameButton.setColour (juce::TextButton::buttonColourId, renameDeleteFace);
        renameButton.setColour (juce::TextButton::buttonOnColourId, renameDeleteFaceOn);
        deleteButton.setColour (juce::TextButton::textColourOffId, renameDeleteText);
        deleteButton.setColour (juce::TextButton::textColourOnId, renameDeleteText);
        deleteButton.setColour (juce::TextButton::buttonColourId, renameDeleteFace);
        deleteButton.setColour (juce::TextButton::buttonOnColourId, renameDeleteFaceOn);
        renameButton.onClick = [this] { doRename (-1); };
        deleteButton.onClick  = [this] { doDelete(); };
        staleButton.onClick   = [this] { doRemoveStale(); };
        closeButton.onClick   = [this] { if (closeRequest) closeRequest(); };
        setSize (600, 420);
        refreshFromIndex();
    }

    void refreshFromIndex()
    {
        rowEntries = profileIndex.entries;
        std::sort (rowEntries.begin(), rowEntries.end(), [](const auto& a, const auto& b)
            { return a.updatedUtc > b.updatedUtc; });
        list.updateContent();
        list.deselectAllRows();
        updateButtonState();
    }

private:
    int getNumRows() override { return (int) rowEntries.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool) override
    {
        if (! juce::isPositiveAndBelow (row, (int) rowEntries.size()))
            return;
        const auto& e = rowEntries[(size_t) row];
        juce::String line = e.displayName;
        if (line.isEmpty())
            line = e.profileId;
        line << "  |  " << (e.midiKey.isNotEmpty() ? e.midiKey : juce::String ("(no key)"));
        if (e.updatedUtc.isNotEmpty())
            line << "  |  " << e.updatedUtc;
        g.setFont (juce::Font (juce::FontOptions().withHeight (15.0f)));
        g.setColour (findColour (juce::ListBox::textColourId, true));
        g.drawFittedText (line, 6, 0, width - 8, height, juce::Justification::centredLeft, 1);
    }

    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override
    {
        if (juce::isPositiveAndBelow (row, (int) rowEntries.size()))
            doRename (row);
    }

    void selectedRowsChanged (int) override { updateButtonState(); }

    int getValidSelectedIndex() const
    {
        const int n = (int) rowEntries.size();
        if (n == 0)
            return -1;
        return list.getSelectedRow();
    }

    void updateButtonState()
    {
        const bool sel = getValidSelectedIndex() >= 0;
        renameButton.setEnabled (sel);
        deleteButton.setEnabled (sel);
    }

    void doRename (int rowIfKnown)
    {
        int r = rowIfKnown;
        if (r < 0)
            r = getValidSelectedIndex();
        if (! juce::isPositiveAndBelow (r, (int) rowEntries.size()))
        {
            juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Rename",
                "Select a profile to rename.");
            return;
        }
        const juce::String profileId = rowEntries[(size_t) r].profileId;
        juce::String startName = rowEntries[(size_t) r].displayName;
        if (startName.isEmpty())
            startName = profileId;
        juce::AlertWindow w ("Rename profile", "New display name:", juce::AlertWindow::NoIcon, this);
        w.addTextEditor ("n", startName, "Name");
        w.addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w.addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        if (w.runModalLoop() != 1)
            return;
        const juce::String newName = w.getTextEditorContents ("n");
        juce::String err;
        if (PlayerSongProfileStore::setProfileDisplayName (profileId, newName, profileIndex, err))
        {
            if (profileId == activeId)
                activeDisplay = newName.trim();
            onMutated();
            refreshFromIndex();
        }
        else
        {
            juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Rename failed", err);
        }
    }

    void doDelete()
    {
        const int r = getValidSelectedIndex();
        if (! juce::isPositiveAndBelow (r, (int) rowEntries.size()))
            return;
        const juce::String& profileId = rowEntries[(size_t) r].profileId;
        juce::String show = rowEntries[(size_t) r].displayName;
        if (show.isEmpty())
            show = profileId;
        if (! juce::AlertWindow::showOkCancelBox (juce::AlertWindow::WarningIcon, "Delete profile",
                "Delete this profile from disk? This cannot be undone.\n\n" + show,
                "Delete", "Cancel", this, nullptr))
            return;
        juce::String err;
        if (PlayerSongProfileStore::deleteProfileById (profileId, profileIndex, err))
        {
            if (profileId == activeId)
            {
                activeId.clear();
                activeDisplay.clear();
                activeCreated.clear();
            }
            onMutated();
            refreshFromIndex();
        }
        else
        {
            juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Delete failed", err);
        }
    }

    void doRemoveStale()
    {
        juce::String err;
        const int n = PlayerSongProfileStore::removeStaleIndexEntriesForMissingProfileFiles (profileIndex, err);
        if (! err.isEmpty())
        {
            juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Cleanup failed", err);
            return;
        }
        onMutated();
        refreshFromIndex();
        if (n > 0)
            juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Cleanup",
                "Removed " + juce::String (n) + " stale index " + (n == 1 ? "row." : "rows."));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);
        const int buttonH = 30;
        auto bottom = area.removeFromBottom (buttonH + 8);
        bottom.removeFromTop (4);
        const int pad = 6;
        auto row = bottom.removeFromTop (buttonH);
        closeButton.setBounds (row.removeFromRight (100));
        row.removeFromRight (pad);
        deleteButton.setBounds (row.removeFromRight (100));
        row.removeFromRight (pad);
        renameButton.setBounds (row.removeFromRight (100));
        row.removeFromRight (pad);
        staleButton.setBounds (row.removeFromLeft (juce::jmin (280, juce::jmax(0, row.getWidth() - pad))));
        list.setBounds (area);
    }

    std::function<void()> closeRequest;
    juce::TextButton renameButton { "Rename" };
    juce::TextButton deleteButton { "Delete" };
    juce::TextButton staleButton { "Remove missing files from list" };
    juce::TextButton closeButton { "Close" };
    juce::ListBox list { "PlayerProfiles" };

    PlayerSongProfilesIndex& profileIndex;
    juce::String& activeId;
    juce::String& activeDisplay;
    juce::String& activeCreated;
    std::function<void()> onMutated;
    std::vector<PlayerSongProfileIndexEntry> rowEntries;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerProfilesManageContent)
};

class PlayerProfilesManagerWindow final : public juce::DocumentWindow
{
public:
    PlayerProfilesManagerWindow (const juce::String& name,
                                 PlayerSongProfilesIndex& index,
                                 juce::String& outActiveId,
                                 juce::String& outActiveDisplay,
                                 juce::String& outActiveCreated,
                                 const std::function<void()>& onMutated) :
        DocumentWindow (name, juce::Colours::lightgrey, DocumentWindow::closeButton)
    {
        setContentOwned (new PlayerProfilesManageContent (
            [this] { closeButtonPressed(); },
            index,
            outActiveId,
            outActiveDisplay,
            outActiveCreated,
            onMutated), true);
        setResizable (true, true);
        setUsingNativeTitleBar (true);
        setResizeLimits (480, 320, 2000, 2000);
        setSize (600, 420);
        centreWithSize (getWidth(), getHeight());
    }

    void closeButtonPressed() override { delete this; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerProfilesManagerWindow)
};
