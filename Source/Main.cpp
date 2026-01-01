/*
    Kousaten Mixer - Main Entry Point
    MADZINE 2024
*/

#include <JuceHeader.h>
#include "MainComponent.h"

class KousatenMixerApplication : public juce::JUCEApplication
{
public:
    KousatenMixerApplication() {}

    const juce::String getApplicationName() override
    {
        return JUCE_APPLICATION_NAME_STRING;
    }

    const juce::String getApplicationVersion() override
    {
        return JUCE_APPLICATION_VERSION_STRING;
    }

    bool moreThanOneInstanceAllowed() override
    {
        return false;
    }

    void initialise(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                             juce::Colour(0xff0e0c0c),  // Dark warm black
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

            #if JUCE_IOS || JUCE_ANDROID
                setFullScreen(true);
            #else
                setResizable(true, true);
                centreWithSize(getWidth(), getHeight());
            #endif

            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(KousatenMixerApplication)
