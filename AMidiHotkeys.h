/*
==============================================================================
 AMidiOrgan — Hotkey bindings editor (JSON persistence under configs/).
==============================================================================
*/

#pragma once

#include "AMidiUtils.h"

#include <vector>

//==============================================================================
inline juce::File getHotkeysFile()
{
    juce::File dir = getOrganUserDocumentsRoot().getChildFile(getAppState().configdir);
    if (!dir.isDirectory())
        dir.createDirectory();
    return dir.getChildFile("hotkeys.json");
}

inline bool parseHotkeyCharFromJson(const juce::String& s, std::optional<juce_wchar>& out)
{
    if (s.isEmpty())
    {
        out = std::nullopt;
        return true;
    }

    if (s.length() != 1)
        return false;

    const juce_wchar c = s[0];
    if (c >= L'0' && c <= L'9')
    {
        out = c;
        return true;
    }
    if (c >= L'a' && c <= L'z')
    {
        out = c;
        return true;
    }
    if (c >= L'A' && c <= L'Z')
    {
        out = (juce_wchar)(c + (L'a' - L'A'));
        return true;
    }

    return false;
}

inline bool hasDuplicateHotkeyAssignments(const HotkeyBindings& b) noexcept
{
    for (int i = 0; i < kNumHotkeyCommands; ++i)
    {
        if (!b.keys[(size_t) i].has_value())
            continue;

        const juce_wchar c = *b.keys[(size_t) i];

        for (int j = i + 1; j < kNumHotkeyCommands; ++j)
        {
            if (b.keys[(size_t) j].has_value() && *b.keys[(size_t) j] == c)
                return true;
        }
    }

    return false;
}

inline bool loadHotkeyBindingsFromFile(HotkeyBindings& out, juce::String& errorMsg)
{
    out = HotkeyBindings::withDefaults();

    const juce::File f = getHotkeysFile();
    if (!f.existsAsFile())
        return true;

    juce::FileInputStream in(f);
    if (!in.openedOk())
    {
        errorMsg = "Could not open hotkeys file.";
        return false;
    }

    const juce::String jsonText = in.readEntireStreamAsString();
    const juce::var parsed = juce::JSON::parse(jsonText);

    if (parsed.isVoid())
    {
        errorMsg = "Invalid JSON in hotkeys file.";
        return false;
    }

    if (auto* obj = parsed.getDynamicObject())
    {
        for (int i = 0; i < kNumHotkeyCommands; ++i)
        {
            const juce::String key = juce::String(static_cast<int>(kHotkeyCommandOrder[(size_t) i]));

            if (!obj->hasProperty(key))
                continue;

            const juce::var v = obj->getProperty(key);

            if (v.isVoid() || v.isUndefined())
            {
                out.keys[(size_t) i] = std::nullopt;
                continue;
            }

            const juce::String s = v.toString();

            if (s.isEmpty())
            {
                out.keys[(size_t) i] = std::nullopt;
                continue;
            }

            std::optional<juce_wchar> ch;
            if (!parseHotkeyCharFromJson(s, ch))
            {
                errorMsg = "Invalid hotkey value for command " + key + ".";
                return false;
            }

            out.keys[(size_t) i] = ch;
        }
    }

    return true;
}

inline bool saveHotkeyBindingsToFile(const HotkeyBindings& b, juce::String& errorMsg)
{
    const juce::File f = getHotkeysFile();

    if (!f.getParentDirectory().createDirectory())
    {
        errorMsg = "Could not create configs directory.";
        return false;
    }

    juce::DynamicObject::Ptr o = new juce::DynamicObject();

    for (int i = 0; i < kNumHotkeyCommands; ++i)
    {
        const juce::String key = juce::String(static_cast<int>(kHotkeyCommandOrder[(size_t) i]));

        if (b.keys[(size_t) i].has_value())
            o->setProperty(key, juce::String::charToString(*b.keys[(size_t) i]));
        else
            o->setProperty(key, juce::var());
    }

    const juce::var root(o.get());
    const juce::String txt = juce::JSON::toString(root, true);

    // FileOutputStream opens at EOF for existing files (append). Use replaceWithText so each save overwrites.
    if (!f.replaceWithText(txt))
    {
        errorMsg = "Could not write hotkeys file.";
        return false;
    }

    return true;
}

/**
 * Apply bindings to the live shortcut map.
 *
 * `KeyPressMappingSet::resetToDefaultMappings()` only replays the *cached*
 * `ApplicationCommandInfo` entries from the last registration — it does not
 * call `getCommandInfo()` again. After changing `KeyPressTarget::hotkeyBindings`,
 * we must re-register commands so defaults (and key mappings) match the new table.
 *
 * This app registers shortcut commands only on `KeyPressTarget` (see AMidiControl).
 */
inline void applyHotkeyBindingsToCommandManager(juce::ApplicationCommandManager& cm, KeyPressTarget& kt, const HotkeyBindings& b)
{
    kt.setHotkeyBindings(b);
    cm.clearCommands();
    cm.registerAllCommandsForTarget(&kt);
}

//==============================================================================
namespace hotkeyUiDetail
{
    inline std::optional<juce_wchar> comboIdToKey(int selectedId) noexcept
    {
        if (selectedId <= 1)
            return std::nullopt;

        if (selectedId >= 2 && selectedId <= 11)
            return (juce_wchar)(L'0' + (selectedId - 2));

        if (selectedId >= 12 && selectedId <= 37)
            return (juce_wchar)(L'a' + (selectedId - 12));

        return std::nullopt;
    }

    inline int keyToComboId(std::optional<juce_wchar> k) noexcept
    {
        if (!k.has_value())
            return 1;

        const juce_wchar c = *k;

        if (c >= L'0' && c <= L'9')
            return 2 + (int)(c - L'0');

        juce_wchar lc = c;

        if (c >= L'A' && c <= L'Z')
            lc = (juce_wchar)(c + (L'a' - L'A'));

        if (lc >= L'a' && lc <= L'z')
            return 12 + (int)(lc - L'a');

        return 1;
    }

    inline void populateHotkeyComboBox(juce::ComboBox& cb)
    {
        cb.clear(juce::dontSendNotification);
        cb.addItem("(None)", 1);

        int id = 2;

        for (juce_wchar c = L'0'; c <= L'9'; ++c)
            cb.addItem(juce::String::charToString(c), id++);

        for (juce_wchar c = L'A'; c <= L'Z'; ++c)
            cb.addItem(juce::String::charToString(c), id++);
    }

    static const char* hotkeyRowLabels[kNumHotkeyCommands] = {
        "Upper tab",
        "Lower tab",
        "Bass tab",
        "Sounds tab",
        "Effects tab",
        "Manual preset",
        "Preset 1",
        "Preset 2",
        "Preset 3",
        "Preset 4",
        "Preset 5",
        "Preset 6",
        "Upper rotary Fast/Slow",
        "Upper rotary Brake",
        "Lower rotary Fast/Slow",
        "Lower rotary Brake"
    };

    /** Label column width (~one-third narrower than 220px); shorter columns pull each dropdown closer to its label. */
    inline constexpr int kHotkeyLabelWidth = 220 - (220 / 3);

    /** Match ConfigPage `comboConfig` text/arrow colours; field fill uses `fieldBackground` (tab colour). */
    inline void styleComboLikeConfig(juce::ComboBox& cb, juce::Colour fieldBackground)
    {
        cb.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
        cb.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        cb.setColour(juce::TextButton::buttonColourId, juce::Colours::lightgrey);
        cb.setColour(juce::TextButton::buttonOnColourId, juce::Colours::antiquewhite);
        cb.setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);
        cb.setColour(juce::ComboBox::backgroundColourId, fieldBackground);
        cb.setEditableText(false);
        cb.setJustificationType(juce::Justification::centredLeft);
    }

    /** Match ConfigPage Load / Save / Save As TextButton colours. */
    inline void styleTextButtonLikeConfig(juce::TextButton& b)
    {
        b.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        b.setColour(juce::TextButton::buttonColourId, juce::Colours::black.darker());
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colours::black.brighter());
    }

    inline void styleLabelLikeConfig(juce::Label& lab)
    {
        lab.setColour(juce::Label::textColourId, juce::Colours::white);
        lab.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::plain)));
    }
} // namespace hotkeyUiDetail

/** Scroll content panel — same fill as the tab (`ResizableWindow::backgroundColourId`). */
struct HotkeyScrollContent final : public juce::Component
{
    void paint(juce::Graphics& g) override
    {
        g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
    }
};

//==============================================================================
class HotkeysPage final : public juce::Component
{
public:
    HotkeysPage(juce::ApplicationCommandManager& cmIn, KeyPressTarget& ktIn)
        : commandManager(cmIn)
        , keyTarget(ktIn)
    {
        working = keyTarget.getHotkeyBindings();
        snapshot = working;

        const juce::Colour tabBg = findColour(juce::ResizableWindow::backgroundColourId);

        mainGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colours::grey.darker());
        addAndMakeVisible(mainGroup);

        viewport.setScrollBarsShown(true, false);
        viewport.setViewedComponent(&content, false);
        mainGroup.addAndMakeVisible(viewport);

        content.setOpaque(true);

        const int rowH = 28;
        const int labelW = hotkeyUiDetail::kHotkeyLabelWidth;
        const int comboW = 100;

        for (int i = 0; i < kNumHotkeyCommands; ++i)
        {
            auto* lab = new juce::Label({}, hotkeyUiDetail::hotkeyRowLabels[i]);
            lab->setJustificationType(juce::Justification::centredLeft);
            hotkeyUiDetail::styleLabelLikeConfig(*lab);
            lab->setBounds(0, 0, labelW, rowH - 2);
            content.addAndMakeVisible(lab);
            labels.push_back(std::unique_ptr<juce::Label>(lab));

            auto* cb = new juce::ComboBox();
            hotkeyUiDetail::populateHotkeyComboBox(*cb);
            hotkeyUiDetail::styleComboLikeConfig(*cb, tabBg);
            cb->setBounds(0, 0, comboW, rowH - 2);
            cb->onChange = [this] { updateSaveButtonState(); };
            content.addAndMakeVisible(cb);
            combos.push_back(std::unique_ptr<juce::ComboBox>(cb));
        }

        content.setSize(320, 320);

        syncCombosFromWorking();

        addAndMakeVisible(saveButton);
        saveButton.setButtonText("Save");
        hotkeyUiDetail::styleTextButtonLikeConfig(saveButton);
        saveButton.onClick = [this] { onSaveClicked(); };

        addAndMakeVisible(cancelButton);
        cancelButton.setButtonText("Cancel");
        hotkeyUiDetail::styleTextButtonLikeConfig(cancelButton);
        cancelButton.onClick = [this] { onCancelClicked(); };

        // Same pattern as ConfigPage: Save disabled until edits; re-disabled after successful save.
        updateSaveButtonState();

        // Match tab pages that use `findColour(ResizableWindow::backgroundColourId)` (see MenuTabs::addTab).
        setOpaque(true);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(8);
        const int buttonH = 30;
        const int buttonW = 80;
        const int gapAboveButtons = 12;   // space between group bottom outline and Save/Cancel row
        const int paddingBelowButtons = 8; // margin under the button row within the tab

        auto bottom = r.removeFromBottom(buttonH + paddingBelowButtons);
        r.removeFromBottom(gapAboveButtons);
        cancelButton.setBounds(bottom.removeFromRight(buttonW).withHeight(buttonH));
        bottom.removeFromRight(8);
        saveButton.setBounds(bottom.removeFromRight(buttonW).withHeight(buttonH));

        mainGroup.setBounds(r);

        const int titleInset = 28;
        const int sidePad = 10;
        viewport.setBounds(sidePad,
                           titleInset,
                           juce::jmax(0, mainGroup.getWidth() - 2 * sidePad),
                           juce::jmax(0, mainGroup.getHeight() - titleInset - sidePad));

        layoutHotkeyGrid();
    }

    void visibilityChanged() override
    {
        if (isShowing())
        {
            working = keyTarget.getHotkeyBindings();
            snapshot = working;
            syncCombosFromWorking();
            updateSaveButtonState();
        }
    }

private:
    juce::ApplicationCommandManager& commandManager;
    KeyPressTarget& keyTarget;

    juce::GroupComponent mainGroup { "hotkeysGroup", "Keyboard shortcuts" };
    juce::Viewport viewport;
    HotkeyScrollContent content;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;

    juce::TextButton saveButton;
    juce::TextButton cancelButton;

    std::unique_ptr<juce::BubbleMessageComponent> bubbleMessage;

    HotkeyBindings working;
    HotkeyBindings snapshot;

    /** True when combo selections match `snapshot` (ConfigPage-style: Save enabled only when dirty). */
    bool currentCombosMatchSnapshot() const
    {
        for (int i = 0; i < kNumHotkeyCommands; ++i)
        {
            if (combos[(size_t) i] == nullptr)
                continue;

            const int selectedId = combos[(size_t) i]->getSelectedId();
            const int expectedId = hotkeyUiDetail::keyToComboId(snapshot.keys[(size_t) i]);

            if (selectedId != expectedId)
                return false;
        }

        return true;
    }

    void updateSaveButtonState()
    {
        const bool dirty = !currentCombosMatchSnapshot();
        saveButton.setEnabled(dirty);

        if (dirty)
        {
            // Match panel Save / Effects “To Upper” pending style (see MenuTabs::updatePanelSaveButtonsPendingStyle).
            saveButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            saveButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
            saveButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
            saveButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkred.brighter());
        }
        else
            hotkeyUiDetail::styleTextButtonLikeConfig(saveButton);
    }

    /** Column-major grid: fill top-to-bottom, then next column. Chooses the widest column count that still fits the viewport to reduce scrolling. */
    void layoutHotkeyGrid()
    {
        const int titleInset = 28;
        const int sidePad = 10;
        const int rowH = 28;
        const int labelW = hotkeyUiDetail::kHotkeyLabelWidth;
        const int comboW = 100;
        const int pad = 8;
        const int colGap = 36; // horizontal gap between columns (combo of col N → label of col N+1)
        const int labelToComboGap = 4; // tighter than outer pad — keeps label + dropdown visually paired
        const int minColW = pad + labelW + labelToComboGap + comboW + pad;

        const int vpW = juce::jmax(0, mainGroup.getWidth() - 2 * sidePad);
        const int vpH = juce::jmax(0, mainGroup.getHeight() - titleInset - sidePad);

        if (vpW <= 0 || vpH <= 0)
            return;

        int numCols = 1;

        for (int c = 1; c <= kNumHotkeyCommands; ++c)
        {
            const int r = (kNumHotkeyCommands + c - 1) / c;
            const int h = pad * 2 + r * rowH;
            const int w = pad * 2 + c * minColW + (c - 1) * colGap;

            if (h <= vpH && w <= vpW)
                numCols = c;
        }

        const int numRows = (kNumHotkeyCommands + numCols - 1) / numCols;

        for (int i = 0; i < kNumHotkeyCommands; ++i)
        {
            const int col = i / numRows;
            const int row = i % numRows;
            const int x = pad + col * (minColW + colGap);
            const int y = pad + row * rowH;

            if (labels[(size_t) i] != nullptr)
                labels[(size_t) i]->setBounds(x, y, labelW, rowH - 2);

            if (combos[(size_t) i] != nullptr)
                combos[(size_t) i]->setBounds(x + pad + labelW + labelToComboGap, y, comboW, rowH - 2);
        }

        const int contentW = pad * 2 + numCols * minColW + (numCols - 1) * colGap;
        const int contentH = pad * 2 + numRows * rowH;
        content.setSize(contentW, contentH);

        const bool needV = contentH > vpH;
        const bool needH = contentW > vpW;
        viewport.setScrollBarsShown(needV, needH);
    }

    void syncCombosFromWorking()
    {
        for (int i = 0; i < kNumHotkeyCommands; ++i)
            if (combos[(size_t) i] != nullptr)
                combos[(size_t) i]->setSelectedId(hotkeyUiDetail::keyToComboId(working.keys[(size_t) i]),
                                                  juce::dontSendNotification);
    }

    void captureWorkingFromCombos()
    {
        for (int i = 0; i < kNumHotkeyCommands; ++i)
            if (combos[(size_t) i] != nullptr)
                working.keys[(size_t) i] = hotkeyUiDetail::comboIdToKey(combos[(size_t) i]->getSelectedId());
    }

    void onSaveClicked()
    {
        captureWorkingFromCombos();

        if (hasDuplicateHotkeyAssignments(working))
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Duplicate hotkeys",
                "Two or more commands use the same key. Assign unique keys or use (None) before saving.",
                "OK");
            return;
        }

        juce::String err;

        if (!saveHotkeyBindingsToFile(working, err))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Save failed", err, "OK");
            return;
        }

        applyHotkeyBindingsToCommandManager(commandManager, keyTarget, working);
        snapshot = working;

        juce::Logger::writeToLog("HotkeysPage: hotkeys saved to " + getHotkeysFile().getFileName());
        BubbleMessage(saveButton, "Hotkeys saved", bubbleMessage);

        updateSaveButtonState();
    }

    void onCancelClicked()
    {
        working = snapshot;
        syncCombosFromWorking();
        updateSaveButtonState();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HotkeysPage)
};
