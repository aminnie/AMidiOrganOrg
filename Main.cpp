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

        ensureUserDataFolderSeeded();

        mainWindow.reset (new MainWindow ("        ", new AMidiControl, *this));
    }

    void shutdown() override
    { 
        mainWindow = nullptr;

        // Tear down logger explicitly to avoid shutdown-order issues.
        Logger::setCurrentLogger(nullptr);
        delete flogger;
        flogger = nullptr;
    }

    void systemRequestedQuit() override
    {
       #if JUCE_WINDOWS && JUCE_DEBUG
        // Work around a persistent Debug CRT heap assertion on shutdown path.
        juce::Process::terminate();
       #else
        quit();
       #endif
    }

    // https://forum.juce.com/t/single-instance-only-standalone/47351/2
    bool moreThanOneInstanceAllowed() override 
    {
        return false;
    }

private:
    static void copyMissingDirectoryContents(const juce::File& sourceDir, const juce::File& targetDir)
    {
        if (!sourceDir.exists() || !sourceDir.isDirectory())
            return;

        if (!targetDir.exists() && !targetDir.createDirectory())
        {
            juce::Logger::writeToLog("*** Startup seed: Failed to create directory " + targetDir.getFullPathName());
            return;
        }

        for (const auto& entry : juce::RangedDirectoryIterator(sourceDir, false, "*", juce::File::findFilesAndDirectories))
        {
            const auto src = entry.getFile();
            const auto dst = targetDir.getChildFile(src.getFileName());

            if (src.isDirectory())
            {
                copyMissingDirectoryContents(src, dst);
            }
            else if (src.existsAsFile() && !dst.existsAsFile())
            {
                if (!src.copyFileTo(dst))
                    juce::Logger::writeToLog("*** Startup seed: Failed to copy file " + src.getFullPathName());
            }
        }
    }

    static void syncManagedInstrumentJsonFiles(const juce::File& sourceInstrumentDir, const juce::File& targetInstrumentDir)
    {
        if (!sourceInstrumentDir.exists() || !sourceInstrumentDir.isDirectory())
            return;

        if (!targetInstrumentDir.exists() && !targetInstrumentDir.createDirectory())
        {
            juce::Logger::writeToLog("*** Startup seed: Failed to create instrument directory " + targetInstrumentDir.getFullPathName());
            return;
        }

        const juce::StringArray managedInstrumentFiles{
            "midigm.json",
            "maxplus.json",
            "integra7.json",
            "at900mi.json",
            "ketronevm.json"
        };

        for (const auto& fileName : managedInstrumentFiles)
        {
            const auto sourceFile = sourceInstrumentDir.getChildFile(fileName);
            const auto targetFile = targetInstrumentDir.getChildFile(fileName);

            if (!sourceFile.existsAsFile())
            {
                juce::Logger::writeToLog("*** Startup seed: Managed instrument source missing " + sourceFile.getFullPathName());
                continue;
            }

            // Replace with current shipped catalog on each startup for managed files.
            if (targetFile.existsAsFile() && !targetFile.deleteFile())
            {
                juce::Logger::writeToLog("*** Startup seed: Failed to replace managed instrument file " + targetFile.getFullPathName());
                continue;
            }

            if (!sourceFile.copyFileTo(targetFile))
                juce::Logger::writeToLog("*** Startup seed: Failed to sync managed instrument file " + sourceFile.getFullPathName());
        }
    }

    static void syncManagedConfigFiles(const juce::File& sourceConfigDir, const juce::File& targetConfigDir)
    {
        if (!sourceConfigDir.exists() || !sourceConfigDir.isDirectory())
            return;

        if (!targetConfigDir.exists() && !targetConfigDir.createDirectory())
        {
            juce::Logger::writeToLog("*** Startup seed: Failed to create config directory " + targetConfigDir.getFullPathName());
            return;
        }

        const juce::StringArray managedConfigFiles{
            "instrument_modules.json"
        };

        for (const auto& fileName : managedConfigFiles)
        {
            const auto sourceFile = sourceConfigDir.getChildFile(fileName);
            const auto targetFile = targetConfigDir.getChildFile(fileName);

            if (!sourceFile.existsAsFile())
            {
                juce::Logger::writeToLog("*** Startup seed: Managed config source missing " + sourceFile.getFullPathName());
                continue;
            }

            // Replace with current shipped config on each startup for managed files.
            if (targetFile.existsAsFile() && !targetFile.deleteFile())
            {
                juce::Logger::writeToLog("*** Startup seed: Failed to replace managed config file " + targetFile.getFullPathName());
                continue;
            }

            if (!sourceFile.copyFileTo(targetFile))
                juce::Logger::writeToLog("*** Startup seed: Failed to sync managed config file " + sourceFile.getFullPathName());
        }
    }

    static juce::File findDocsSeedDirectory()
    {
        auto docsIn = [](const juce::File& base)
            {
                return base.getChildFile("docs");
            };
        auto docsInBundleResources = [](const juce::File& base)
            {
                return base.getChildFile("Resources").getChildFile("docs");
            };

        // 1) Prefer working-directory docs (typical dev run).
        auto docsDir = docsIn(juce::File::getCurrentWorkingDirectory());
        if (docsDir.exists() && docsDir.isDirectory())
            return docsDir;

        // 2) Walk up from executable location (packaged/dev launch fallback).
        auto probe = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
        for (int depth = 0; depth < 6 && probe.exists(); ++depth)
        {
            docsDir = docsIn(probe);
            if (docsDir.exists() && docsDir.isDirectory())
                return docsDir;

            // Support macOS app bundles where docs are packaged under
            // Contents/Resources/docs.
            docsDir = docsInBundleResources(probe);
            if (docsDir.exists() && docsDir.isDirectory())
                return docsDir;

            auto parent = probe.getParentDirectory();
            if (parent == probe)
                break;
            probe = parent;
        }

        return {};
    }

    static void copyDirectoryContents(const juce::File& sourceDir, const juce::File& targetDir)
    {
        for (const auto& entry : juce::RangedDirectoryIterator(sourceDir, false, "*", juce::File::findFilesAndDirectories))
        {
            const auto src = entry.getFile();
            const auto dst = targetDir.getChildFile(src.getFileName());

            if (src.isDirectory())
            {
                if (!dst.exists() && !dst.createDirectory())
                    juce::Logger::writeToLog("*** Startup seed: Failed to create directory " + dst.getFullPathName());

                if (!src.copyDirectoryTo(dst))
                    juce::Logger::writeToLog("*** Startup seed: Failed to copy directory " + src.getFullPathName());
            }
            else if (src.existsAsFile())
            {
                if (!src.copyFileTo(dst))
                    juce::Logger::writeToLog("*** Startup seed: Failed to copy file " + src.getFullPathName());
            }
        }
    }

    void ensureUserDataFolderSeeded()
    {
        const auto userDataDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile(organdir);

        const bool isFirstStart = !userDataDir.exists();

        if (isFirstStart && !userDataDir.createDirectory())
        {
            juce::Logger::writeToLog("*** Startup seed: Failed to create user data folder " + userDataDir.getFullPathName());
            return;
        }

        const auto docsSeed = findDocsSeedDirectory();
        if (!docsSeed.exists() || !docsSeed.isDirectory())
        {
            juce::Logger::writeToLog("*** Startup seed: docs directory not found; skipping initial copy");
            return;
        }

        // First run: bootstrap all shipped docs/assets into user workspace.
        if (isFirstStart)
            copyDirectoryContents(docsSeed, userDataDir);

        // Every startup: ensure configs folder/files exist in the user workspace.
        const auto sourceConfigDir = docsSeed.getChildFile(configdir);
        const auto targetConfigDir = userDataDir.getChildFile(configdir);
        copyMissingDirectoryContents(sourceConfigDir, targetConfigDir);
        syncManagedConfigFiles(sourceConfigDir, targetConfigDir);

        // Every startup: ensure instruments folder/files exist in the user workspace (JSON catalogs).
        const auto sourceInstrumentDir = docsSeed.getChildFile(instrumentdir);
        const auto targetInstrumentDir = userDataDir.getChildFile(instrumentdir);
        copyMissingDirectoryContents(sourceInstrumentDir, targetInstrumentDir);
        syncManagedInstrumentJsonFiles(sourceInstrumentDir, targetInstrumentDir);

        // Every startup: ensure panels folder (.pnl) exists in the user workspace.
        const auto sourcePanelDir = docsSeed.getChildFile(paneldir);
        const auto targetPanelDir = userDataDir.getChildFile(paneldir);
        copyMissingDirectoryContents(sourcePanelDir, targetPanelDir);
    }

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
            const auto profile = resolveUiProfile(getAppState().uiProfileId);
            setResizable (false, false);
            setResizeLimits (1000, 300, profile.baseWidth, profile.baseHeight);
            setSize(profile.baseWidth, profile.baseHeight);
            centreWithSize (profile.baseWidth, profile.baseHeight);
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
