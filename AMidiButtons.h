/*
  ==============================================================================

    AMidiButtons.h
    Created: 14 Feb 2024 6:22:15pm
    Author:  a_min

  ==============================================================================
*/

#pragma once


//==============================================================================
// Class: VoiceButton - Create TextButton type that stores the MidiInstrument
//==============================================================================
static int zinstcntvoicebutton = 0;
class VoiceButton : public TextButton
{
public:
    VoiceButton()
    {
        //DBG("=== VoiceButton(): Constructor " + std::to_string(zinstcntvoicebutton++));
    }

    VoiceButton(String svoice)
    {
        instrument.setVoice(svoice);
    }

    ~VoiceButton() {
        //DBG("=== VoiceButton(): Destructor " + std::to_string(--zinstcntvoicebutton));
    }

    bool setInstrument(Instrument instr)
    {
        instrument = instr;

        return true;
    }

    Instrument getInstrument()
    {
        return instrument;
    }

    // Used when EffectsPage need to update changed effects in place on the panel (not copy of)
    Instrument* getInstrumentPtr()
    {
        return &instrument;
    }

    int getButtonGroupId()
    {
        return buttongroupid;
    }

    void setButtonGroupId(int btngroupid)
    {
        buttongroupid = btngroupid;
    }

    int getButtonId()
    {
        return buttonid;
    }

    void setButtonId(int btnid)
    {
        buttonid = btnid;
    }

    void setPanelButtonIdx(int panelbtnidx)
    {
        panelbuttonidx = panelbtnidx;
    }

    int getPanelButtonIdx()
    {
        return panelbuttonidx;
    }

    // Link between Buttons on tghe Display Component and Instrument Panes
    VoiceButton* getDisplayButtonPtr()
    {
        return displaybuttonptr;
    }

    void setDisplayButtonPtr(VoiceButton* dspbtnptr)
    {
        displaybuttonptr = dspbtnptr;
    }

private:
    Instrument instrument;

    int buttongroupid;
    int buttonid;
    int panelbuttonidx;
    Component::SafePointer<VoiceButton>displaybuttonptr;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceButton)
};


//==============================================================================
// Class: PresetButton - Button Group of Preset Buttons 
//==============================================================================
static int zinstcntpresetbutton = 0;
class PresetButton : public TextButton
{
public:
    PresetButton()
    {
        DBG("=== PresetButton(): Constructor " + std::to_string(zinstcntpresetbutton++));
    }

    ~PresetButton() {
        DBG("=== PresetButton(): Destructor " + std::to_string(--zinstcntpresetbutton));
    }

    PresetButton(String svoice)
    {
        setButtonText(svoice);
    }

    int getButtonId()
    {
        return buttonid;
    }

    void setButtonId(int btnid)
    {
        buttonid = btnid;
    }

    // Link between Buttons on the Display Component and Instrument Panels
    PresetButton* getPresetButtonPtr()
    {
        return presetbuttonptr;
    }

    void setPresetButtonPtr(PresetButton* presetbtnptr)
    {
        presetbuttonptr = presetbtnptr;
    }

private:
    int buttonid;
    Component::SafePointer<PresetButton>presetbuttonptr;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetButton)
};


//==============================================================================
// Class: MuteButton - Button Group Mute Buttons 
//==============================================================================
static int zinstcntmutebutton = 0;
class MuteButton : public TextButton
{
public:
    MuteButton()
    {
        DBG("=== MuteButton(): Constructor " + std::to_string(zinstcntmutebutton++));
    }

    ~MuteButton() {
        DBG("=== MuteButton(): Destructor " + std::to_string(--zinstcntmutebutton));
    }

    MuteButton(String svoice)
    {
        setButtonText(svoice);
    }

    int getButtonGroupId()
    {
        return buttongroupid;
    }

    void setButtonGroupId(int btngroupid)
    {
        buttongroupid = btngroupid;
    }

    int getButtonId()
    {
        return buttonid;
    }

    void setButtonId(int btnid)
    {
        buttonid = btnid;
    }

    // Link between Buttons on the Display Component and Instrument Panels
    MuteButton* getMuteButtonPtr()
    {
        return mutebuttonptr;
    }

    void setMuteButtonPtr(MuteButton* mutebtnptr)
    {
        mutebuttonptr = mutebtnptr;
    }

private:
    int buttongroupid;
    int buttonid;
    Component::SafePointer<MuteButton>mutebuttonptr;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuteButton)
};


//==============================================================================
// Class: CommandButton - Create TextButton type that stores the MidiInstrument
//==============================================================================
class CommandButton : public TextButton
{
public:
    CommandButton() {}

    ~CommandButton() {}

private:

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandButton)
};

//==============================================================================
// Class: ArrowCommandButton - Command-style button with thick white arrow
//==============================================================================
class ArrowCommandButton : public CommandButton
{
public:
    enum class Direction
    {
        up,
        down
    };

    explicit ArrowCommandButton(Direction dir) : direction(dir)
    {
        setButtonText("");
        setClickingTogglesState(true);
    }

    ~ArrowCommandButton() override = default;

    void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        const bool isEnabledNow = isEnabled();
        const bool isPressedOrToggled = shouldDrawButtonAsDown || getToggleState();
        const auto background = isPressedOrToggled ? findColour(TextButton::buttonOnColourId)
                                                   : findColour(TextButton::buttonColourId);
        const float corner = 4.4f;

        const auto borderColour = isPressedOrToggled ? Colours::black.withAlpha(0.70f)
                                                     : Colours::black.withAlpha(0.55f);
        const auto highlightColour = isPressedOrToggled ? Colours::white.withAlpha(0.04f)
                                                        : Colours::white.withAlpha(0.10f);

        g.setColour(isEnabledNow ? background : background.withAlpha(0.80f));
        g.fillRoundedRectangle(bounds, corner);

        // Subtle top sheen to better match the look of neighboring organ-style controls.
        auto topHalf = bounds;
        topHalf.setHeight(bounds.getHeight() * 0.45f);
        g.setColour(highlightColour);
        g.fillRoundedRectangle(topHalf, corner);

        if (isEnabledNow && shouldDrawButtonAsHighlighted && !isPressedOrToggled)
        {
            g.setColour(Colours::white.withAlpha(0.08f));
            g.fillRoundedRectangle(bounds, corner);
        }

        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds, corner, 1.1f);

        const float verticalNudge = isPressedOrToggled ? 0.8f : 0.0f;
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY() + verticalNudge;
        const float halfWidth = jmin(bounds.getWidth(), bounds.getHeight()) * 0.23f;
        const float halfHeight = jmin(bounds.getWidth(), bounds.getHeight()) * 0.20f;

        Path arrowPath;
        if (direction == Direction::up)
        {
            arrowPath.startNewSubPath(cx - halfWidth, cy + halfHeight);
            arrowPath.lineTo(cx, cy - halfHeight);
            arrowPath.lineTo(cx + halfWidth, cy + halfHeight);
        }
        else
        {
            arrowPath.startNewSubPath(cx - halfWidth, cy - halfHeight);
            arrowPath.lineTo(cx, cy + halfHeight);
            arrowPath.lineTo(cx + halfWidth, cy - halfHeight);
        }

        const auto arrowColour = isEnabledNow ? Colours::white
                                              : Colours::lightgrey.withAlpha(0.55f);
        g.setColour(arrowColour);
        g.strokePath(arrowPath, PathStrokeType(4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }

private:
    Direction direction;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrowCommandButton)
};




