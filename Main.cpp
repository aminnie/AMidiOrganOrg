/*
  ==============================================================================

    This file contains the startup code for the JUCE AMidiOrgan.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "BinaryData.h"
#include "AMidiControl.h"

class Application : public juce::JUCEApplication
{
public:
    //==============================================================================
    Application() = default;

    const juce::String getApplicationName() override       { return "AMidiOrgan"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }

    void initialise (const juce::String&) override
    {

        // Create JUCE Logger file for Windows in directory equivalent
        // to "C:\Users\a_min\AppData\Roaming\amidiorgan"
        flogger = FileLogger::createDateStampedLogger(
            "amidiorgan",                           //const String & logFileSubDirectoryName,
            "amidilogger",                          //const String & logFileNameRoot,
            "log",                                  //const String & logFileNameSuffix,
            "AMidiLogger Open"                      //const String & welcomeMessage
        );
        Logger::setCurrentLogger(flogger);

        // Get user home directory
        //File fileToSave = File::getCurrentWorkingDirectory().getChildFile(instrumentdname);
        File filespecialloc = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(organdir);

        AMidiTrayIcon();

        mainWindow.reset (new MainWindow ("        ", new AMidiControl, *this));
    }

    void shutdown() override
    { 
        mainWindow = nullptr; 
    }

    // https://forum.juce.com/t/single-instance-only-standalone/47351/2
    bool moreThanOneInstanceAllowed() override 
    {
        return false;
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name, juce::Component* c, JUCEApplication& a)
            : DocumentWindow (name, 
                juce::Desktop::getInstance()
                        .getDefaultLookAndFeel()
                        .findColour (ResizableWindow::backgroundColourId),
                                        juce::DocumentWindow::closeButton),
                app (a)
        {
            setUsingNativeTitleBar(false);
            setTitleBarHeight(6);          // 320 - 3x2 - 8

            setContentOwned (c, true);

           #if JUCE_ANDROID || JUCE_IOS
            setFullScreen (true);
           #else
            setResizable (false, false);
            setResizeLimits (1000, 300, 1480, 320);
            centreWithSize (getWidth(), getHeight());
            setDraggable(true);
           #endif

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            app.systemRequestedQuit();
        }

    private:
        JUCEApplication& app;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION (Application)
