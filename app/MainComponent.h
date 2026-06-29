#pragma once

#include <juce_gui_extra/juce_gui_extra.h>


#include "cppDmx/DmxEngine.h"
#include "cppDmx/Drivers/Art-Net/ArtNetDiscovery.h"

/** The whole controller UI in one component:

      - "Detect" enumerates reachable Art-Net destinations into the dropdown.
      - The universe selector chooses the flat, 0-based universe to transmit on
        (with a live decode into Art-Net Net / SubUni, matching ArtNetOutput).
      - "Start sending" hands a freshly-built ArtNetOutput to the DmxEngine and
        runs a slow RGB sweep on the chosen universe so a monitor visibly lights
        up — confirming the destination and universe are correct end to end.

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
    void timerCallback() override;

    cppDmx::DmxEngine engine;
    std::vector<DiscoveredOutput> outputs;

    juce::Label       titleLabel;

    juce::Label       outputLabel { {}, "Output" };
    juce::ComboBox    outputBox;
    juce::TextButton  detectButton { "Detect" };

    juce::Label       universeLabel { {}, "Universe" };
    juce::Slider      universeSlider { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::Label       universeDecodeLabel;

    juce::TextButton  startButton { "Start sending" };
    juce::Label       statusLabel;

    bool sending  = false;
    int  sweepStep = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
