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
        addAndMakeVisible (filterLabel);
        addAndMakeVisible (filterEditor);
        addAndMakeVisible (list);
        for (juce::TextButton* b : { &renameButton, &deleteButton, &staleButton, &closeButton })
        {
            addAndMakeVisible (b);
        }
        filterLabel.setText ("Filter:", juce::dontSendNotification);
        filterLabel.setJustificationType (juce::Justification::centredLeft);
        filterEditor.setMultiLine (false);
        filterEditor.setReturnKeyStartsNewLine (false);
        filterEditor.setTextToShowWhenEmpty ("Search profiles...", juce::Colours::grey);
        filterEditor.onTextChange = [this] { applyFilter(); };
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
        sortedAllEntries = profileIndex.entries;
        std::sort (sortedAllEntries.begin(), sortedAllEntries.end(), [](const auto& a, const auto& b)
            { return a.updatedUtc > b.updatedUtc; });
        applyFilter();
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
        if (e.moduleDisplayName.isNotEmpty())
            line << " [" << e.moduleDisplayName << "]";
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
        const int selected = list.getSelectedRow();
        return juce::isPositiveAndBelow (selected, n) ? selected : -1;
    }

    bool entryMatchesFilter (const PlayerSongProfileIndexEntry& entry, const juce::String& needle) const
    {
        if (needle.isEmpty())
            return true;

        if (entry.displayName.containsIgnoreCase (needle)) return true;
        if (entry.profileId.containsIgnoreCase (needle)) return true;
        if (entry.moduleDisplayName.containsIgnoreCase (needle)) return true;
        if (entry.midiKey.containsIgnoreCase (needle)) return true;
        if (entry.updatedUtc.containsIgnoreCase (needle)) return true;
        if (entry.profileFile.containsIgnoreCase (needle)) return true;
        if (entry.profileFile.isNotEmpty()
            && juce::File (entry.profileFile).getFileName().containsIgnoreCase (needle)) return true;

        return false;
    }

    void applyFilter()
    {
        juce::String selectedProfileId;
        const int previousSelection = getValidSelectedIndex();
        if (juce::isPositiveAndBelow (previousSelection, (int) rowEntries.size()))
            selectedProfileId = rowEntries[(size_t) previousSelection].profileId;

        rowEntries.clear();
        const juce::String needle = filterEditor.getText().trim();
        for (const auto& entry : sortedAllEntries)
        {
            if (entryMatchesFilter (entry, needle))
                rowEntries.push_back (entry);
        }

        list.updateContent();

        int selectedAfterFilter = -1;
        if (selectedProfileId.isNotEmpty())
        {
            for (int i = 0; i < (int) rowEntries.size(); ++i)
            {
                if (rowEntries[(size_t) i].profileId == selectedProfileId)
                {
                    selectedAfterFilter = i;
                    break;
                }
            }
        }

        if (selectedAfterFilter >= 0)
            list.selectRow (selectedAfterFilter);
        else
            list.deselectAllRows();

        updateButtonState();
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
        auto* w = new juce::AlertWindow ("Rename profile", "New display name:", juce::AlertWindow::NoIcon, this);
        w->addTextEditor ("n", startName, "Name");
        w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        auto safeThis = juce::Component::SafePointer<PlayerProfilesManageContent> (this);
        w->enterModalState (true,
            juce::ModalCallbackFunction::create ([safeThis, profileId, w] (int result)
            {
                if (safeThis == nullptr || result != 1)
                    return;

                const juce::String newName = w->getTextEditorContents ("n");
                juce::String err;
                if (PlayerSongProfileStore::setProfileDisplayName (profileId, newName, safeThis->profileIndex, err))
                {
                    if (profileId == safeThis->activeId)
                        safeThis->activeDisplay = newName.trim();
                    safeThis->onMutated();
                    safeThis->refreshFromIndex();
                }
                else
                {
                    juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Rename failed", err);
                }
            }),
            true);
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
        const int filterH = 26;
        auto filterRow = area.removeFromTop (filterH);
        filterLabel.setBounds (filterRow.removeFromLeft (56));
        filterEditor.setBounds (filterRow.reduced (2, 0));
        area.removeFromTop (6);

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
    juce::Label filterLabel { "profilesFilterLabel", "Filter:" };
    juce::TextEditor filterEditor;
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
    std::vector<PlayerSongProfileIndexEntry> sortedAllEntries;
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

/** In-window content: filtered profile list + load action for Player->Load MIDI Profile. */
class PlayerProfilesLoadContent final : public juce::Component,
                                         private juce::ListBoxModel
{
public:
    PlayerProfilesLoadContent (const std::function<void()>& requestClose,
                               const PlayerSongProfilesIndex& index,
                               const juce::String& activeProfileId,
                               const std::function<void(const juce::String&)>& onLoadRequested) :
        closeRequest (requestClose),
        profileIndex (index),
        activeId (activeProfileId),
        onLoad (onLoadRequested)
    {
        list.setModel (this);
        list.setRowHeight (26);
        list.setMultipleSelectionEnabled (false);
        addAndMakeVisible (filterLabel);
        addAndMakeVisible (filterEditor);
        addAndMakeVisible (list);
        addAndMakeVisible (loadButton);
        addAndMakeVisible (closeButton);
        filterLabel.setText ("Filter:", juce::dontSendNotification);
        filterLabel.setJustificationType (juce::Justification::centredLeft);
        filterEditor.setMultiLine (false);
        filterEditor.setReturnKeyStartsNewLine (false);
        filterEditor.setTextToShowWhenEmpty ("Search profiles...", juce::Colours::grey);
        filterEditor.onTextChange = [this] { applyFilter(); };
        loadButton.onClick = [this] { doLoadSelected(); };
        closeButton.onClick = [this] { if (closeRequest) closeRequest(); };
        setSize (600, 420);
        refreshFromIndex();
    }

    void refreshFromIndex()
    {
        sortedAllEntries = profileIndex.entries;
        std::sort (sortedAllEntries.begin(), sortedAllEntries.end(), [](const auto& a, const auto& b)
            { return a.updatedUtc > b.updatedUtc; });
        applyFilter();
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
        if (e.moduleDisplayName.isNotEmpty())
            line << " [" << e.moduleDisplayName << "]";
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
            doLoadSelected();
    }

    void selectedRowsChanged (int) override { updateButtonState(); }

    int getValidSelectedIndex() const
    {
        const int n = (int) rowEntries.size();
        if (n == 0)
            return -1;
        const int selected = list.getSelectedRow();
        return juce::isPositiveAndBelow (selected, n) ? selected : -1;
    }

    bool entryMatchesFilter (const PlayerSongProfileIndexEntry& entry, const juce::String& needle) const
    {
        if (needle.isEmpty())
            return true;

        if (entry.displayName.containsIgnoreCase (needle)) return true;
        if (entry.profileId.containsIgnoreCase (needle)) return true;
        if (entry.moduleDisplayName.containsIgnoreCase (needle)) return true;
        if (entry.midiKey.containsIgnoreCase (needle)) return true;
        if (entry.updatedUtc.containsIgnoreCase (needle)) return true;
        if (entry.profileFile.containsIgnoreCase (needle)) return true;
        if (entry.profileFile.isNotEmpty()
            && juce::File (entry.profileFile).getFileName().containsIgnoreCase (needle)) return true;

        return false;
    }

    void applyFilter()
    {
        juce::String selectedProfileId;
        const int previousSelection = getValidSelectedIndex();
        if (juce::isPositiveAndBelow (previousSelection, (int) rowEntries.size()))
            selectedProfileId = rowEntries[(size_t) previousSelection].profileId;

        rowEntries.clear();
        const juce::String needle = filterEditor.getText().trim();
        for (const auto& entry : sortedAllEntries)
        {
            if (entryMatchesFilter (entry, needle))
                rowEntries.push_back (entry);
        }

        list.updateContent();

        int selectedAfterFilter = -1;
        if (selectedProfileId.isNotEmpty())
        {
            for (int i = 0; i < (int) rowEntries.size(); ++i)
            {
                if (rowEntries[(size_t) i].profileId == selectedProfileId)
                {
                    selectedAfterFilter = i;
                    break;
                }
            }
        }
        else if (activeId.isNotEmpty())
        {
            for (int i = 0; i < (int) rowEntries.size(); ++i)
            {
                if (rowEntries[(size_t) i].profileId == activeId)
                {
                    selectedAfterFilter = i;
                    break;
                }
            }
        }

        if (selectedAfterFilter >= 0)
            list.selectRow (selectedAfterFilter);
        else
            list.deselectAllRows();

        updateButtonState();
    }

    void updateButtonState()
    {
        loadButton.setEnabled (getValidSelectedIndex() >= 0);
    }

    void doLoadSelected()
    {
        const int r = getValidSelectedIndex();
        if (! juce::isPositiveAndBelow (r, (int) rowEntries.size()))
            return;

        if (onLoad != nullptr)
            onLoad (rowEntries[(size_t) r].profileId);
        if (closeRequest != nullptr)
            closeRequest();
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);
        const int filterH = 26;
        auto filterRow = area.removeFromTop (filterH);
        filterLabel.setBounds (filterRow.removeFromLeft (56));
        filterEditor.setBounds (filterRow.reduced (2, 0));
        area.removeFromTop (6);

        const int buttonH = 30;
        auto bottom = area.removeFromBottom (buttonH + 8);
        bottom.removeFromTop (4);
        const int pad = 6;
        auto row = bottom.removeFromTop (buttonH);
        closeButton.setBounds (row.removeFromRight (100));
        row.removeFromRight (pad);
        loadButton.setBounds (row.removeFromRight (100));
        list.setBounds (area);
    }

    std::function<void()> closeRequest;
    const PlayerSongProfilesIndex& profileIndex;
    juce::String activeId;
    std::function<void(const juce::String&)> onLoad;
    juce::Label filterLabel { "profilesFilterLabel", "Filter:" };
    juce::TextEditor filterEditor;
    juce::TextButton loadButton { "Load" };
    juce::TextButton closeButton { "Close" };
    juce::ListBox list { "PlayerProfilesLoad" };
    std::vector<PlayerSongProfileIndexEntry> sortedAllEntries;
    std::vector<PlayerSongProfileIndexEntry> rowEntries;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerProfilesLoadContent)
};

class PlayerProfilesLoadWindow final : public juce::DocumentWindow
{
public:
    PlayerProfilesLoadWindow (const juce::String& name,
                              const PlayerSongProfilesIndex& index,
                              const juce::String& activeProfileId,
                              const std::function<void(const juce::String&)>& onLoadRequested) :
        DocumentWindow (name, juce::Colours::lightgrey, DocumentWindow::closeButton)
    {
        setContentOwned (new PlayerProfilesLoadContent (
            [this] { closeButtonPressed(); },
            index,
            activeProfileId,
            onLoadRequested), true);
        setResizable (true, true);
        setUsingNativeTitleBar (true);
        setResizeLimits (480, 320, 2000, 2000);
        setSize (600, 420);
        centreWithSize (getWidth(), getHeight());
    }

    void closeButtonPressed() override { delete this; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerProfilesLoadWindow)
};
