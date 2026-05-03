/*
==============================================================================
 AMidiOrgan — Hotkey bindings editor (JSON persistence under configs/).
==============================================================================
*/

#pragma once

#include "AMidiUtils.h"

#include <vector>

inline constexpr const char* kHotkeysUiProfileFontScaleProperty = "uiProfileFontScale";

class HotkeysProfileLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override
    {
        auto f = juce::LookAndFeel_V4::getTextButtonFont(button, buttonHeight);
        return f.withHeight(f.getHeight() * getComponentScale(button));
    }

    juce::Font getComboBoxFont(juce::ComboBox& box) override
    {
        auto f = juce::LookAndFeel_V4::getComboBoxFont(box);
        return f.withHeight(f.getHeight() * getComponentScale(box));
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        auto f = juce::LookAndFeel_V4::getLabelFont(label);
        return f.withHeight(f.getHeight() * getComponentScale(label));
    }

    void drawGroupComponentOutline(juce::Graphics& g, int width, int height, const juce::String& text,
                                   const juce::Justification& position, juce::GroupComponent& group) override
    {
        const float textH = juce::jlimit(10.0f, 72.0f, 15.0f * getComponentScale(group));
        const float indent = 3.0f;
        const float textEdgeGap = 4.0f;
        auto cs = 5.0f;

        juce::Font f(juce::FontOptions().withHeight(textH));
        juce::Path p;
        auto x = indent;
        auto y = f.getAscent() - 3.0f;
        auto w = juce::jmax(0.0f, (float)width - x * 2.0f);
        auto h = juce::jmax(0.0f, (float)height - y - indent);
        cs = juce::jmin(cs, w * 0.5f, h * 0.5f);
        auto cs2 = 2.0f * cs;
        auto textW = text.isEmpty() ? 0.0f
                                    : juce::jlimit(0.0f,
                                                   juce::jmax(0.0f, w - cs2 - textEdgeGap * 2),
                                                   (float)juce::GlyphArrangement::getStringWidthInt(f, text) + textEdgeGap * 2.0f);
        auto textX = cs + textEdgeGap;
        if (position.testFlags(juce::Justification::horizontallyCentred))
            textX = cs + (w - cs2 - textW) * 0.5f;
        else if (position.testFlags(juce::Justification::right))
            textX = w - cs - textW - textEdgeGap;

        p.startNewSubPath(x + textX + textW, y);
        p.lineTo(x + w - cs, y);
        p.addArc(x + w - cs2, y, cs2, cs2, 0, juce::MathConstants<float>::halfPi);
        p.lineTo(x + w, y + h - cs);
        p.addArc(x + w - cs2, y + h - cs2, cs2, cs2, juce::MathConstants<float>::halfPi, juce::MathConstants<float>::pi);
        p.lineTo(x + cs, y + h);
        p.addArc(x, y + h - cs2, cs2, cs2, juce::MathConstants<float>::pi, juce::MathConstants<float>::pi * 1.5f);
        p.lineTo(x, y + cs);
        p.addArc(x, y, cs2, cs2, juce::MathConstants<float>::pi * 1.5f, juce::MathConstants<float>::twoPi);
        p.lineTo(x + textX, y);

        const auto alpha = group.isEnabled() ? 1.0f : 0.5f;
        g.setColour(group.findColour(juce::GroupComponent::outlineColourId).withMultipliedAlpha(alpha));
        g.strokePath(p, juce::PathStrokeType(2.0f));
        g.setColour(group.findColour(juce::GroupComponent::textColourId).withMultipliedAlpha(alpha));
        g.setFont(f);
        g.drawText(text, juce::roundToInt(x + textX), 0, juce::roundToInt(textW), juce::roundToInt(textH),
                   juce::Justification::centred, true);
    }

private:
    static float getComponentScale(const juce::Component& c)
    {
        const auto v = c.getProperties().getWithDefault(kHotkeysUiProfileFontScaleProperty, 1.0);
        if (!(v.isInt() || v.isInt64() || v.isDouble()))
            return 1.0f;
        return juce::jlimit(0.5f, 4.0f, static_cast<float>((double) v));
    }
};

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
        "Player tab",
        "Sounds tab",
        "Effects tab",
        "Monitor tab",
        "Manual preset",
        "Preset 1",
        "Preset 2",
        "Preset 3",
        "Preset 4",
        "Preset 5",
        "Preset 6",
        "Next preset",
        "Upper rotary Fast/Slow",
        "Upper rotary Brake",
        "Lower rotary Fast/Slow",
        "Lower rotary Brake",
        "Player Start/Stop"
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
        setLookAndFeel(&uiProfileLookAndFeel);
        working = keyTarget.getHotkeyBindings();
        snapshot = working;

        const juce::Colour tabBg = findColour(juce::ResizableWindow::backgroundColourId);

        mainGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colours::yellow.darker(0.45f));
        mainGroup.setComponentID("hk.group");
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
            lab->setComponentID("hk.label." + juce::String(i));
            lab->setBounds(0, 0, labelW, rowH - 2);
            content.addAndMakeVisible(lab);
            labels.push_back(std::unique_ptr<juce::Label>(lab));

            auto* cb = new juce::ComboBox();
            hotkeyUiDetail::populateHotkeyComboBox(*cb);
            hotkeyUiDetail::styleComboLikeConfig(*cb, tabBg);
            cb->setComponentID("hk.combo." + juce::String(i));
            cb->setBounds(0, 0, comboW, rowH - 2);
            cb->onChange = [this] { updateSaveButtonState(); };
            content.addAndMakeVisible(cb);
            combos.push_back(std::unique_ptr<juce::ComboBox>(cb));
        }

        content.setSize(320, 320);

        syncCombosFromWorking();

        addAndMakeVisible(saveButton);
        saveButton.setButtonText("Save");
        saveButton.setComponentID("hk.save");
        hotkeyUiDetail::styleTextButtonLikeConfig(saveButton);
        saveButton.onClick = [this] { onSaveClicked(); };

        addAndMakeVisible(cancelButton);
        cancelButton.setButtonText("Cancel");
        cancelButton.setComponentID("hk.cancel");
        hotkeyUiDetail::styleTextButtonLikeConfig(cancelButton);
        cancelButton.onClick = [this] { onCancelClicked(); };

        // Same pattern as ConfigPage: Save disabled until edits; re-disabled after successful save.
        updateSaveButtonState();

        // Match tab pages that use `findColour(ResizableWindow::backgroundColourId)` (see MenuTabs::addTab).
        setOpaque(true);
        applyCurrentUiProfile();
    }

    ~HotkeysPage() override
    {
        setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(scaleX(8));
        const int buttonH = scaleY(30);
        const int buttonW = scaleX(80);
        const int gapAboveButtons = scaleY(12);   // space between group bottom outline and Save/Cancel row
        const int paddingBelowButtons = scaleY(8); // margin under the button row within the tab

        auto bottom = r.removeFromBottom(buttonH + paddingBelowButtons);
        r.removeFromBottom(gapAboveButtons);
        cancelButton.setBounds(bottom.removeFromRight(buttonW).withHeight(buttonH));
        bottom.removeFromRight(scaleX(8));
        saveButton.setBounds(bottom.removeFromRight(buttonW).withHeight(buttonH));

        mainGroup.setBounds(r);

        const int titleInset = scaleY(28);
        const int sidePad = scaleX(10);
        viewport.setBounds(sidePad,
                           titleInset,
                           juce::jmax(0, mainGroup.getWidth() - 2 * sidePad),
                           juce::jmax(0, mainGroup.getHeight() - titleInset - sidePad));

        layoutHotkeyGrid();
    }

    void applyCurrentUiProfile()
    {
        currentProfile = resolveUiProfile(getAppState().uiProfileId);
        applyScale(mainGroup, currentProfile.groupTitleFontScale);
        applyScale(saveButton, currentProfile.buttonFontScale);
        applyScale(cancelButton, currentProfile.buttonFontScale);
        const float hotkeyLabelScale = (currentProfile.id == "2560x720") ? 1.12f : currentProfile.labelFontScale;
        for (size_t i = 0; i < labels.size(); ++i)
            if (labels[i] != nullptr)
                applyScale(*labels[i], hotkeyLabelScale);
        for (size_t i = 0; i < combos.size(); ++i)
            if (combos[i] != nullptr)
                applyScale(*combos[i], currentProfile.comboFontScale);
        resized();
    }

    void visibilityChanged() override
    {
        if (isShowing())
        {
            applyCurrentUiProfile();
            working = keyTarget.getHotkeyBindings();
            snapshot = working;
            syncCombosFromWorking();
            updateSaveButtonState();
        }
    }

private:
    juce::ApplicationCommandManager& commandManager;
    KeyPressTarget& keyTarget;

    juce::GroupComponent mainGroup { "hotkeysGroup", "Keyboard Shortcuts" };
    juce::Viewport viewport;
    HotkeyScrollContent content;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;

    juce::TextButton saveButton;
    juce::TextButton cancelButton;

    std::unique_ptr<juce::BubbleMessageComponent> bubbleMessage;

    HotkeyBindings working;
    HotkeyBindings snapshot;
    UiProfileDefinition currentProfile = resolveUiProfile(getAppState().uiProfileId);
    HotkeysProfileLookAndFeel uiProfileLookAndFeel;

    int scaleX(int v) const
    {
        return juce::jmax(1, juce::roundToInt(v * juce::jlimit(0.5f, 4.0f, currentProfile.xScale)));
    }

    int scaleY(int v) const
    {
        return juce::jmax(1, juce::roundToInt(v * juce::jlimit(0.5f, 4.0f, currentProfile.yScale)));
    }

    void applyScale(juce::Component& c, float fallbackScale)
    {
        float scale = fallbackScale;
        if (const auto specific = getUiFontScaleOverride(currentProfile, c.getComponentID()))
            scale = *specific;
        c.getProperties().set(kHotkeysUiProfileFontScaleProperty, juce::jlimit(0.5f, 4.0f, scale));
    }

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
        const int titleInset = scaleY(28);
        const int sidePad = scaleX(10);
        const int rowH = scaleY(28);
        const int labelW = scaleX(hotkeyUiDetail::kHotkeyLabelWidth);
        const int comboW = scaleX(100);
        const int pad = scaleX(8);
        const int colGap = scaleX(36); // horizontal gap between columns (combo of col N → label of col N+1)
        const int labelToComboGap = scaleX(4); // tighter than outer pad — keeps label + dropdown visually paired
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
