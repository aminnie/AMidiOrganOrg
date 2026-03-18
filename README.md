AMidiOrgan Features

## Build

### Prerequisites
- CMake 3.22 or newer
- JUCE source checkout (for example: `C:/JUCE` on Windows)

### Windows (Visual Studio generator)
From the repository root:

```powershell
cmake -S . -B build -DJUCE_ROOT="C:/JUCE"
cmake --build build --config Debug --target AMidiOrgan
```

Output executable:
- `build/AMidiOrgan_artefacts/Debug/AMidiOrgan.exe`

### Run (Windows)
From the repository root:

```powershell
# Run Debug build in a new process
Start-Process "build/AMidiOrgan_artefacts/Debug/AMidiOrgan.exe"

# Optional: run in the current terminal (foreground)
& "build/AMidiOrgan_artefacts/Debug/AMidiOrgan.exe"
```

To run Release:

```powershell
cmake --build build --config Release --target AMidiOrgan
Start-Process "build/AMidiOrgan_artefacts/Release/AMidiOrgan.exe"
```

### Test (Windows)
From the repository root:

```powershell
# Build the test executable
cmake --build build --config Debug --target AMidiOrganTests

# Run tests
ctest --test-dir build -C Debug --output-on-failure
```

### macOS (Xcode generator)
From the repository root:

```bash
cmake -S . -B build-mac -G Xcode -DJUCE_ROOT="/path/to/JUCE"
cmake --build build-mac --config Debug --target AMidiOrgan
```

Typical output app bundle:
- `build-mac/Debug/AMidiOrgan.app`

### Notes
- UI images are packaged from `assets/*.png` via `juce_add_binary_data(...)` in `CMakeLists.txt`.

### Asset Naming Contract
The following asset filenames are referenced by code through `BinaryData` symbols and should remain stable unless code and CMake are updated together:
- `assets/keyboard.png`
- `assets/icons8arrowdown32.png`
- `assets/icons8arrowup32.png`
- `assets/icons8arrowdown32click.png`
- `assets/icons8arrowup32click.png`

New:
- MIDI Hardeware and Software Sound Mdules: 
	Deebach BlackBox, Roland Integra7, Ketron SD2, Midi GM. 
	Please conact developer for more to be added.

1. Voice Buttons
1.1 MIDI Instrument Sound from loaded Device (MSB, LSB, Soundfont).
1.2 Ten MIDI Effects (VOL, EXP, REV, CHO, MOD, TIM, ATK, REL, BRI, PAN) applied to Button Group MIDI Out channel.
1.3. MIDI Sound settings are unique per Voice Button and any/all effects may be changed from one Button to another.

2. Voice Button Groups
2.1 A number of Voice Buttons in a logical grouping.
2.2.1 Logical groupings can be changed by the user in the Config page.
2.2 Voice Button Groups also stores:
2.2.1 The MIDI IN and OUT settings for this group, configurable by the user via Config Page
2.2.2 Octave shift settings for this group.
2.2.3 Solo Keyboard Split:
2.2.3.1 Solo Button Group can be used to host typical Solo instruments. However the Solo split section remains polyphonic.
2.2.3.2 One Solo Split for the Upper and Lower Keyboards.
2.3 Every Voice Button Group has a Volume slider with level Up and Down Buttons.
2.3.1 Voice Button value is preset by the value on the active Voice Button enabling adjutments equalization in realtime.
2.3.2 Changes to the Button Group volume is not stored back on the active Voice Button Instrument Volume effect.
2.4. Voice Button Groups Mute and Unmute:
2.4.1 Muting is used to enable Voice Layering by turning the sound volume off for the associated Button Group.
2.4.2 Muting a Voice Button Group disables the Volume Slider and Up/Down Buttons and sets Volume for the MIDI Out channel to zero.
2.4.2.1 MIDI Notes from the Button Group MIDI Input channel are always transmited to the MIDI Output channel for a Button Group.
2.4.2.2 This enables dynamic layering of Input Keyboard notes to Voice Button Group MIDI output channels.
2.5 Output Note layering:
2.5.1 All MIDI messages in the first Buttton Group MIDI IN channel is forwarded directly to its MIDI Out Channel. Default same MIDI IN as OUT.
2.5.2. Layering to Button Group 2, 3, and 4 forwards MIDI Note On/Off Messages only using the active Voice Button, its sound and effects.
2.6 To do: Need to adjust Bass and Drums Keyboard Panel to allow all messages on first Drum Button Group as well.

3. Instrument Panel
3.1 System supports one Instrument Panel that contains:
3.1.1 Three Keyboard Panels: Upper, Lower and Bass&Drums.
3.2 Instrument Panel is named and saved to and loaded from disk in the application working directory.
3.3 User is able to create instrument panels for specific organ types/styles or by song.
3.4. Instrument Panel contains up to 96 Voice Buttons organized in 12 Buttons Groups over 3 Keyboard Panels.
3.5 Instrument Panel saves all 96 Voice Button values, 12 Button Group details, as well as 7 x Preset configurations to .pnl file for reload.
3.6 Instrument Panel Button Groups are color coded to assist identifying the Keyboard Panel you are working with.
3.7 To do: Make the Voice Button Group sizing/nmbers configurable. This was done for the first Voice Group to support default Organ Bass.

4. Presets
4.1. Total for 7 Presets supported:
4.1.1 Manual Preset as the default when the system starts up.
4.1.2 Six programmable Presets.
4.2. Every Preset stores the active/selected Voice Button in every Button Voice Button Group for all 3 Keyboard Panels.
4.3. A reference to the Voice Button is stored and reflecs the currently selected Sound and Effects.
4.4 The Preset SET button is used to program (update) a selected Preset, including Manual.
4.4.1 To update a Preset, ensure you select the preferred Preset to preset active Buttons, and update active Buttons as needed. 
4.4.2 All Voices on all Panels reflect your preference. 
4.4.3 Click Preset Set button and proceed to click Preset Button to update.
4.5. The Preset button group is shared by all three Keyboard Panels. 
4.5.1 Selections on any one of the three Keyboard Panels saves all three Panels into the Preset.
4.6. To save the Preset selections, save the Instrument Panel.

5. Rotary Button Group
5.2 Available on the Upper and Lower Keyboard Panels.
5.3 Controls Rotor Speed (Fast/Slow) and Brake On/Off.
5.3.1 Ramp up and down speed based on an internal list played at 100ms intervals
5.4 To do: Needs more work limited syncronizing voice startup fast/slow speed.

6. Keyboard Panel
6.1. Every Keyboard Panel contains four Voice Button Groups.
6.2. Seven Presets are supported by a Keyboard Panel.
6.3. Upper and Lower Panel Rotary function is associated with Button Group 1 (Organ by default).

7. Sounds Edit Page
7.1 Any support Instrument Midi Sound can be selected and programmed into a Voice Button on any Keyboard Panel.
7.2 Ensure intended Voice Button selected before clicking on Sounds button in Keyboard Panel. Page displays Voice Button details.
7.3 Once picked from pop-up menu, this voice sound is MIDI sent on the Button Group MIDI Out channel for demo play.
7.4 Click on the To Upper/To Lower/To Bass button to save new Sound to the Voice Button and return to the Keyboard Panel.
7.5. To do: Assess whether we continue to use JUCE popup menu for this function. Works better with a mouse than touch.

8. Effects Edit Page
8.1 Any of the supported MIDI Effects can be selected and programmed into a Voice Button on any Keyboard Panel
8.2 Ensure intended Voice Button is selected before clicking on Effects button in Keyboard Panel. Page displays Voice Buttons details.
8.3 Changes to any of the 10 MIDI Effects are applied in realtime to the Button Group MIDI Out channel.
8.4 Click on the To Upper/To Lower/To Bass button to save changed Effects values on Voice Button and return to the Keyboard Panel.
8.5 MIDI Effects applies to the Button Group MIDI channel:
8.5.1 Every Voice Button and associated sound has its own Effects that is supplied to the Button Group MIDI Out channel when Voice selected.
8.5.2 Establish if we need global Effect settings in addition to Button specific similar an organ - e.g. Volume or Brilliance.
8.6 To do: Consider reading current MIDI Device Effects values directly and use as defaults for Effect values.

9. Config Page
9.1 Configure Button Group parameters:
9.1.2 Button Group Name (1 of 12).
9.1.3 Button Group MIDI IN and Out Channel.
9.1.4 Button Group Octave Shift (+3 to -3).
9.1.5 Solo Keyboard Split for Upper and Lower Keyboard Panels. 
9.2 MIDI Reset Button resets all MIDI Controller on each of 16 channels using standard MIDI command. 
9.3 MIDI Pass Through check: If off, any channel that is not configured as input in a Button Group is blocked and not forwarded to output.
9.3.1. Useful where Organ Solo channel is output on seperate channel from Uppper/Lower and interferes with kKeyboard Split in this application.
9.3.2. If check is off, list to block is recalculated and applied on startup or Config save.
9.4 Config Page applies to Organ application and is independent of the Instrument Panel loaded.
9.5 To do: Consider if configs should be moved into Panel save and by Instrument Panel file, rather than global.
9.6 To do: Add feature to load different instrument JSON file.

10. MIDI Start Page
10.1 MIDI Input and Output Devices are dynamially loaded and made available for selected upon connect or disconnect.
10.2 Multiple MIDI Input Keyboards and Output Devices are supported.
10.2.1 Only one MIDI Sound Device (e.g. Deebach MaxPlus) is support.
10.3 Button Group MIDI In/Out Channels should be configured to listen and output to the preferred devices by MIDI channel.
10.4 In addition to output note layering by Button Group, MIDI input is also layered onto each selected MIDI OUT device.
10.5 Instrument HW/SW Sound Module Button: Displaus the currently selected Midi Sound Module name 
10.5.1 Click button produces to list of Sound Modules currently supported:
10.5.1.1 Deebach BlackBox, Roland Integra7, Ketron SD2, MIDIGM, Custom MIDI
10.5.2 A new module can be added be replace the .JSON sound file in the aaplication Custom directory. 
10.5.3 Check JSON validity using to ensure instrument loads. E.g.: https://jsonlint.com/
10.6 To do: Test Bluetooth connectivity (standard JUCE framework support).

11 General
11.1 Built using the JUCE C++ framework.
11.1.1 Application can be compiled for different operating systems and devices as it is using JUCE and C++.
12.1.2 Initial executable for Windows based systems
11.3 System supports a touch panel and using mouse or keyboard to navigate is optional.

12. Device Support
12.1 MIDI Keyboards
12.1.1 Every Button Group supports a MIDI keyboard, and/or inputs using common MIDI IN
12.1.2 For Solo input from dedicated MIDI keyboard, set split value to 0 for MIDI OUT 
12.2 Sound Modules
12.2.1 Uses custom Sound Module Device file with a simple JSON based structure converted from the original device instrument file.
12.2.2 Supports output to any Hardeware or Software Module that presents itself as a MIDI device after Sound Module file has been added.
12.3 Displays
12.3.1 Current display optimized for a Waveshare 11.9 Capacitive Touch Screen (1480 by 320 pixles)
12.3.1.1 https://www.waveshare.com/11.9inch-hdmi-lcd.htm
12.3.2 Applcation autocenters on systems with a typical HD 15.6" display.
12.3.3 Please contact developer for other display suggestions/requests.

13. Up Next:
13.1 Use JUCE to compile and test controller with Raspberry PI or Latte Panda SBC
13.1.1  Intention is to be able to build a cost effective and standalone controller and display that does not need PC or similar
13.2 Add optional HW support for selected buttons and sliders

 Contact Detaails:
 - Anton Minnie @ a_minnie@hotmail.com.
