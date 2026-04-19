/*
  ==============================================================================

    AMidiInstruments.h
    Created: 14 Feb 2024 7:31:54pm
    Author:  a_min

  ==============================================================================
*/

#pragma once


//==============================================================================
// Class: Instrument - Midi Data Object to be loaded onto Voice Panel Buttons
//==============================================================================
static int zinstcntInstrument = 0;
class Instrument final
{
public:
    Instrument() {
        //DBG("=== Instrument(): Constructor " + std::to_string(zinstcntInstrument++));
    };

    ~Instrument() {

        //DBG("=== Instrument(): Destructor " + std::to_string(--zinstcntInstrument));
    };

    String getVoice() {
        return svoice;
    }

    void setVoice(String val) {
        svoice = val;
    }

    String getCategory() {
        return category;
    }

    void setCategory(String val) {
        category = val;
    }

    int getMSB() {
        return sbank1;
    }

    void setMSB(int val) {
        val = check0to127(val);
        sbank1 = val;
    }

    int getLSB() {
        return sbank2;
    }

    void setLSB(int val) {
        val = check0to127(val);
        sbank2 = val;
    }

    int getFont() {
        return sfont;
    }

    void setFont(int val) {
        val = check0to127(val);
        sfont = val;
    }

    int getVol() {
        return vol;
    }

    void setVol(int val) {
        val = check0to127(val);
        vol = val;
    }

    int getExp() {
        return exp;
    }

    void setExp(int val) {
        val = check0to127(val);
        exp = val;
    }

    int getRev() {
        return rev;
    }

    void setRev(int val) {
        val = check0to127(val);
        rev = val;
    }
    int getCho() {
        return cho;
    }

    void setCho(int val) {
        val = check0to127(val);
        cho = val;
    }

    int getMod() {
        return mod;
    }

    void setMod(int val) {
        val = check0to127(val);
        mod = val;
    }

    int getTim() {
        return tim;
    }

    void setTim(int val) {
        val = check0to127(val);
        tim = val;
    }

    int getAtk() {
        return atk;
    }

    void setAtk(int val) {
        val = check0to127(val);
        atk = val;
    }

    int getRel() {
        return rel;
    }

    void setRel(int val) {
        val = check0to127(val);
        rel = val;
    }

    int getBri() {
        return bri;
    }

    void setBri(int val) {
        val = check0to127(val);
        bri = val;
    }

    int getPan() {
        return pan;
    }

    void setPan(int val) {
        val = check0to127(val);
        pan = val;
    }

    // Dirty is treated a bitmap to indicate which effects have been changed
    int getDirty() {
        return isdirty;
    }

    void setDirty(int val) {
        isdirty = isdirty | val;
    }

    void replaceDirtyMask(int val) {
        isdirty = val;
    }

    int getChannel()
    {
        return midichannel;
    }

    void setChannel(int val) {
        val = check1to16(val);
        midichannel = val;
    }

    // Used to simplify Panel save to disk and reload
    int getButtonIdx()
    {
        return buttonidx;
    }

    void setButtonIdx(int val) {
        buttonidx = val;
    }

private:
    String category = "Category";
    String svoice = "Test Grand1";

    int sbank1 = 121;
    int sbank2 = 8;
    int sfont = 0;

    int vol = 100;
    int exp = 127;
    int rev = 20;
    int cho = 10;
    int mod = 0;
    int tim = 0;
    int atk = 0;
    int rel = 0;
    int bri = 30;
    int pan = 64;

    int isdirty = 0;

    // To do: Move to the button as it is button specific
    int midichannel = 1;

    int buttonidx = 0;

    // To do: Fix Instrument and issues with the leak detector!
    //JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Instrument)
};


//==============================================================================
// Class: MidiInstruments - Singleton and instruments JSON object.
// CPP file that uses Singleton
// https://ccrma.stanford.edu/~jos/juce_modules/group__juce__core-memory.html#gacd79c49d860c4386b12231ca141a41b9
//==============================================================================
static int zinstcntmidiinstruments = 0;
class MidiInstruments final : public Component
{
public:

    bool loadMidiInstruments(const String& instrumentFileName)
    {
        String fname = instrumentFileName;

        // Sample JSON Instrument File
            //std::string jsonText = R"(
            //     {"Vendor":"Max-Plus Instruments", "Instruments" : [["A-Piano", [3, 0, 121, 8, "Deebach Grand1"], [3, 0, 121, 0, "Classic Grand1"], [3, 0, 121, 3, "Classic Grand2"], [3, 0, 121, 9, "Italian Grand1"], [3, 0, 121, 25, "Sharp Piano3"], [3, 0, 121, 4, "Upright Piano1"], [3, 0, 121, 5, "Upright Piano2"], [3, 0, 121, 6, "Upright Piano3"], [3, 0, 121, 22, "SwedishGrand1"], [3, 1, 121, 0, "Pop Grand1"], [3, 1, 121, 5, "Pop Grand2"], [3, 1, 121, 7, "Pop Grand3"], [3, 1, 121, 9, "Pop Grand4"]], ["E-Piano", [3, 64, 121, 88, "Suitcase"], [3, 65, 121, 88, "MK-1"], [3, 66, 121, 88, "Straight Rhodes"], [3, 4, 121, 26, "RidersTremoloEP"]], ["Organ", [3, 65, 121, 101, "Jimmy Jazz 3rdPc"], [3, 78, 121, 100, "Beta Perc Organ"], [3, 90, 121, 100, "SpecFullDrw F/S"]]] }
            //)";
            // Parse the JSON string into a var object.
            //juce::var jsonData = juce::JSON::parse(jsonText);

        juce::Logger::writeToLog("*** loadMidiInstruments(): Loading " + instrumentFileName + " instrument file");

        // Read the instruments JSON file from disk into a string
        juce::File instrumentFile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir)
            .getChildFile(appState.instrumentdir)
            .getChildFile(instrumentFileName);
        if (!instrumentFile.existsAsFile()) {
            juce::Logger::writeToLog("*** loadMidiInstruments(): Failed to load " + instrumentFileName + " instrument file");

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Instrument file not found!", instrumentFileName);
            return false;
        }

        auto instrumentText = instrumentFile.loadFileAsString();

        const juce::var parsed = juce::JSON::parse(instrumentText);
        if (parsed.isVoid() || parsed.isUndefined() || !parsed.isObject())
        {
            juce::Logger::writeToLog("*** loadMidiInstruments(): JSON parse failed or root is not an object: " + instrumentFileName);
            return false;
        }

        const juce::var instrumentsNode = parsed["Instruments"];
        if (!instrumentsNode.isArray())
        {
            juce::Logger::writeToLog("*** loadMidiInstruments(): Missing or invalid Instruments array: " + instrumentFileName);
            return false;
        }

        jsonInstrumentData = parsed;

        // Check if newly loaded Midi Instrument file is 0 or 1 based on the PC value
        // For example: Integra7 needs 1 subtracted from PC value before send
        // Track this value while the file is loaded for any new Voice selects
        // and Voice Button assignments. Not used during realtime voice selects 
        if (instrumentmodules->isZeroBased(fname))
            appState.iszerobased = true;
        else
            appState.iszerobased = false;

        return true;
    }

    String getVendor()
    {
        return jsonInstrumentData["Vendor"];
    }

    String getCategory(int cat) {
        if (cat < 0)
            return "None";

        const auto instruments = jsonInstrumentData["Instruments"];
        if (!instruments.isArray() || cat >= instruments.size())
            return "None";

        const auto category = instruments[cat];
        if (!category.isArray() || category.size() < 1)
            return "None";

        return category[0].toString();
    }

    int getCategoryCount() {
        const auto instruments = jsonInstrumentData["Instruments"];
        if (!instruments.isArray())
            return 0;

        return instruments.size();
    }

    String getVoice(int cat, int inst) {
        const auto row = getVoiceRowArray(cat, inst);
        if (!row.isArray())
            return "No Voice";

        return row[4].toString();
    }

    int getMSB(int cat, int inst) {
        const auto row = getVoiceRowArray(cat, inst);
        if (!row.isArray())
            return 0;

        return static_cast<int>(row[2]);
    }

    int getLSB(int cat, int inst) {
        const auto row = getVoiceRowArray(cat, inst);
        if (!row.isArray())
            return 0;

        return static_cast<int>(row[3]);
    }

    int getFont(int cat, int inst) {
        const auto row = getVoiceRowArray(cat, inst);
        if (!row.isArray())
            return 0;

        int insfont = static_cast<int>(row[1]);

        if ((!appState.iszerobased) && (inst > 0)) insfont = insfont - 1;

        return insfont;
    }

    int getCategoryVoiceCount(int cat) {
        if (cat < 0)
            return 0;

        const auto instruments = jsonInstrumentData["Instruments"];
        if (!instruments.isArray())
            return 0;

        const int categoryCount = instruments.size();
        if (cat >= categoryCount)
            return 0;

        const auto category = instruments[cat];
        if (!category.isArray())
            return 0;

        // Category entry layout is ["CategoryName", [voice1], [voice2], ...].
        const int voiceCount = category.size() - 1;
        return voiceCount > 0 ? voiceCount : 0;
    }

    struct VoiceMatch
    {
        int categoryIdx = -1;
        int voiceIdx = -1; // 1-based voice entry index within category
    };

    juce::Array<VoiceMatch> findVoicesContaining(const juce::String& searchTerm)
    {
        juce::Array<VoiceMatch> matches;

        const juce::String needle = searchTerm.trim().toLowerCase();
        if (needle.isEmpty())
            return matches;

        const int categoryCount = getCategoryCount();
        for (int categoryIdx = 0; categoryIdx < categoryCount; ++categoryIdx)
        {
            const int voiceCount = getCategoryVoiceCount(categoryIdx);
            for (int voiceIdx = 1; voiceIdx <= voiceCount; ++voiceIdx)
            {
                const auto voiceName = getVoice(categoryIdx, voiceIdx);
                if (voiceName.toLowerCase().contains(needle))
                    matches.add({ categoryIdx, voiceIdx });
            }
        }

        return matches;
    }

    ~MidiInstruments() {
        DBG("=S= MidiInstruments(): Destructor " + std::to_string(--zinstcntmidiinstruments));
    };

    juce_DeclareSingleton(MidiInstruments, true)

private:
    // Private constructor so only one object instance can be created.
    MidiInstruments() : instrumentmodules(InstrumentModules::getInstance()),
        appState(getAppState())
    {
        juce::Logger::writeToLog("=S= MidiInstruments(): Constructor " + std::to_string(zinstcntmidiinstruments++));
    }

    juce::var jsonInstrumentData;

    InstrumentModules* instrumentmodules = nullptr;
    AppState& appState;

    /** Category arrays are ["CategoryName", voiceRow1, voiceRow2, ...]; voice rows start at index 1. */
    juce::var getVoiceRowArray(int cat, int voiceIdx) const
    {
        if (cat < 0 || voiceIdx < 1)
            return {};

        const auto instruments = jsonInstrumentData["Instruments"];
        if (!instruments.isArray() || cat >= instruments.size())
            return {};

        const auto category = instruments[cat];
        if (!category.isArray() || voiceIdx >= category.size())
            return {};

        const auto row = category[voiceIdx];
        if (!row.isArray() || row.size() < 5)
            return {};

        return row;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiInstruments)
};
juce_ImplementSingleton(MidiInstruments)


