#pragma once

#include <juce_gui_extra/juce_gui_extra.h>


#include "cppDmx/DmxEngine.h"
#include "cppDmx/IDmxDriver.h"
#include "cppDmx/Drivers/Art-Net/ArtNetDiscovery.h"

/** The whole controller UI in one component:

      - The driver selector chooses Art-Net or USB Pro (Enttec DMX USB Pro).
      - For Art-Net, "Detect" enumerates reachable destinations into the
        dropdown. For USB Pro there's no discovery yet, so a COM port is
        typed in directly.
      - The universe selector chooses the flat, 0-based universe to transmit on
        (with a live decode into Art-Net Net / SubUni, matching ArtNetOutput;
        USB Pro ignores the decode since a widget only ever has one universe).
      - "Start sending" builds the selected driver and hands it the DmxEngine,
        then runs a slow RGB sweep on the chosen universe so a monitor visibly
        lights up — confirming the destination and universe are correct end
        to end.

    The component owns the DmxEngine; everything below DmxEngine is untouched. */
struct DiscoveredOutput
{
    std::string description;   // what the user reads in the dropdown
    std::string host;          // unicast or broadcast target
    int         port = 6454;   // Art-Net default
};

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void refreshOutputs();
    void onOutputsDiscovered (std::vector<DiscoveredOutput> found, int nodeCount);
    void toggleSending();
    void updateUniverseDecode();
    void updateDriverTypeUI();
    void timerCallback() override;

    cppDmx::DmxEngine engine;
    std::unique_ptr<cppDmx::IDmxDriver> outputDriver;
    std::vector<DiscoveredOutput> outputs;

    juce::Label       titleLabel;

    juce::Label       driverTypeLabel { {}, "Driver" };
    juce::ComboBox    driverTypeBox;

    juce::Label       outputLabel { {}, "Output" };
    juce::ComboBox    outputBox;
    juce::TextButton  detectButton { "Detect" };

    juce::Label       comPortLabel { {}, "COM Port" };
    juce::TextEditor  comPortEditor;

    juce::Label       universeLabel { {}, "Universe" };
    juce::Slider      universeSlider { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::Label       universeDecodeLabel;

    juce::TextButton  startButton { "Start sending" };
    juce::Label       statusLabel;

    bool sending  = false;
    int  sweepStep = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
